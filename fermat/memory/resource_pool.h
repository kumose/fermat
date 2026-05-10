// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <fermat/memory/allocator.h>
#include <atomic>
#include <mutex>
#include <turbo/strings/str_format.h>

namespace fermat {
    // -----------------------------------------------------------------------------
    // ResourceId: 64-bit ID encoding (block_id, slot_id, version, user)
    // -----------------------------------------------------------------------------
    struct ResourceId {
        static constexpr uint64_t kInvalidId = 0;
        static constexpr uint64_t kVersionMask = 0xFFFFULL << 32;
        static constexpr uint64_t kUserMask = 0xFFFFULL << 48;

        explicit ResourceId(uint64_t val = kInvalidId) noexcept : raw_(val) {
        }

        uint64_t encode() const noexcept { return raw_; }

        uint16_t block_id() const noexcept { return static_cast<uint16_t>(raw_ >> 0); }
        uint16_t slot_id() const noexcept { return static_cast<uint16_t>(raw_ >> 16); }
        uint16_t version() const noexcept { return static_cast<uint16_t>(raw_ >> 32); }
        uint16_t user_space() const noexcept { return static_cast<uint16_t>(raw_ >> 48); }

        void set_block_id(uint16_t v) noexcept {
            raw_ = (raw_ & ~0xFFFFULL) | v;
        }

        void set_slot_id(uint16_t v) noexcept {
            raw_ = (raw_ & ~(0xFFFFULL << 16)) | (static_cast<uint64_t>(v) << 16);
        }

        void set_version(uint16_t v) noexcept {
            raw_ = (raw_ & ~kVersionMask) | (static_cast<uint64_t>(v) << 32);
        }

        void set_user_space(uint16_t v) noexcept {
            raw_ = (raw_ & ~kUserMask) | (static_cast<uint64_t>(v) << 48);
        }

        void next_version() noexcept { set_version(version() + 1); }

        std::string to_string() const {
            return turbo::str_format("{block:%d, slot:%d, version:%d, user:%d}",
                                     block_id(), slot_id(), version(), user_space());
        }

    private:
        uint64_t raw_;
    };


    /// @brief Fixed-size object pool with versioned slots, CAS, and TLS caching.
    ///
    /// @tparam T          Object type.
    /// @tparam BlockSize  Maximum number of blocks.
    /// @tparam SlotSize   Slots per block.
    /// @tparam TlsCache   Max free indices per thread cache.
    /// @tparam Batch      Number of slots to fetch from global list to TLS.
    ///
    /// ### ABA Problem and Solution
    /// Traditional pools allow stale IDs to access reallocated slots (ABA).
    /// This pool stores an atomic version counter per slot.
    /// - On every `get_uninitialize()` or `put_raw()`, the slot's version is
    ///   incremented atomically (CAS).
    /// - The returned ID contains the current version.
    /// - `find()` checks that the stored version matches the slot's version.
    /// - Old IDs have mismatched versions and become invalid.
    ///
    /// ### Concurrency Model
    /// - Per‑slot version CAS gives lock‑free allocation/free on the slot.
    /// - Global free list and TLS caches use a mutex, but the hot path (TLS hit) is lock‑free.
    /// - Version is 16‑bit. Overflow is harmless; accidental match probability is negligible.
    ///
    /// ### TLS Caching
    /// - Each thread caches free slot indices in `tls_free_list_`.
    /// - Allocation: pop from TLS; if empty, fetch a batch from global list under mutex.
    /// - Release: push to TLS; if TLS size > TlsCache, move half to global list under mutex.
    /// - Reduces global contention and improves locality.
    ///
    /// ### Memory Layout
    /// - `Block` contains `std::atomic<uint16_t> version[SlotSize]` and `T data[SlotSize]`.
    /// - `version` array is cache‑line aligned to avoid false sharing.
    template<typename T, size_t BlockSize = 8, size_t SlotSize = 64,
        size_t TlsCache = 1024, size_t Batch = 64>
    class ResourcePool {
    public:
        struct Block {
            // Version per slot (atomic). Placed in a separate array to avoid false sharing.
            alignas(64) std::atomic<uint16_t> version[SlotSize];
            T data[SlotSize];
        };

        ~ResourcePool() {
            std::lock_guard lock(blocks_mutex_);
            for (Block *blk: blocks_) {
                Malloc::good_free(blk);
            }
        }

        static ResourcePool &instance() {
            static ResourcePool pool;
            return pool;
        }

        // Find object by 64-bit ID. Returns nullptr if ID invalid or version mismatch.
        static T *find(int64_t id) {
            ResourceId rid(id);
            uint16_t bid = rid.block_id();
            uint16_t sid = rid.slot_id();
            auto &pool = instance();
            if (bid >= pool.blocks_.size() || sid >= SlotSize) return nullptr;
            Block *blk = pool.blocks_[bid];
            uint16_t cur_ver = blk->version[sid].load(std::memory_order_acquire);
            if (cur_ver != rid.version()) return nullptr;
            return &blk->data[sid];
        }

        // Allocate an uninitialized object. Returns pointer and sets `rid_out` to the new ID.
        static T *get_uninitialize(int64_t &rid_out) {
            auto &pool = instance();

            // 1. Obtain a free slot index from TLS or global free list.
            uint32_t idx;
            if (!pool.acquire_free_slot(idx)) return nullptr;

            uint16_t bid = static_cast<uint16_t>(idx >> 16);
            uint16_t sid = static_cast<uint16_t>(idx & 0xFFFF);
            Block *blk = pool.blocks_[bid];
            std::atomic<uint16_t> &ver_atom = blk->version[sid];

            // 2. CAS to increment version (allocate to new user)
            uint16_t old_ver = ver_atom.load(std::memory_order_relaxed);
            uint16_t new_ver = old_ver + 1;
            if (!ver_atom.compare_exchange_strong(old_ver, new_ver,
                                                  std::memory_order_acq_rel)) {
                // Rare: concurrent modification; fallback to retry via a different slot.
                // For simplicity, push the slot back and retry recursively (or loop).
                pool.release_slot(idx);
                return get_uninitialize(rid_out); // recursion, but depth limited
            }

            ResourceId rid;
            rid.set_block_id(bid);
            rid.set_slot_id(sid);
            rid.set_version(new_ver);
            rid_out = rid.encode();
            return &blk->data[sid];
        }

        // Construct object with arguments and allocate.
        template<typename... Args>
        static T *get(int64_t &rid, Args &&... args) {
            T *ptr = get_uninitialize(rid);
            if (!ptr) return nullptr;
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        // Release raw object (without destructor). The ID must be valid.
        static void put_raw(int64_t rid_val) {
            ResourceId rid(rid_val);
            uint16_t bid = rid.block_id();
            uint16_t sid = rid.slot_id();
            auto &pool = instance();
            if (bid >= pool.blocks_.size() || sid >= SlotSize) return;
            Block *blk = pool.blocks_[bid];
            std::atomic<uint16_t> &ver_atom = blk->version[sid];

            uint16_t old_ver = rid.version();
            uint16_t new_ver = old_ver + 1;
            // CAS to increment version; if fails, the ID is already stale.
            if (!ver_atom.compare_exchange_strong(old_ver, new_ver,
                                                  std::memory_order_acq_rel)) {
                // Stale ID, ignore (already freed or reused).
                return;
            }
            // Slot is now free (version increased). Return the index to free list.
            uint32_t idx = (static_cast<uint32_t>(bid) << 16) | sid;
            pool.release_slot(idx);
        }

        // Release constructed object (calls destructor then put_raw).
        static void put(int64_t rid) {
            T *ptr = find(rid);
            if (!ptr) return;
            ptr->~T();
            put_raw(rid);
        }

    private:
        ResourcePool() {
            std::lock_guard lock(blocks_mutex_);
            blocks_.reserve(BlockSize);
            if (!create_block()) throw std::bad_alloc();
        }

        bool create_block() {
            size_t n = sizeof(Block);
            void *mem = Malloc::good_alloc(&n);
            if (!mem) return false;
            Block *blk = new(mem) Block;
            // Initialize versions to 1 (odd number)
            for (size_t i = 0; i < SlotSize; ++i) {
                blk->version[i].store(1, std::memory_order_relaxed);
            }
            blocks_.push_back(blk);
            uint16_t bid = static_cast<uint16_t>(blocks_.size() - 1);
            for (uint16_t sid = 0; sid < SlotSize; ++sid) {
                uint32_t idx = (static_cast<uint32_t>(bid) << 16) | sid;
                free_list_global_.push_back(idx);
            }
            return true;
        }

        bool acquire_free_slot(uint32_t &out_idx) {
            // 1. Try TLS cache
            if (!tls_free_list_.empty()) {
                out_idx = tls_free_list_.back();
                tls_free_list_.pop_back();
                return true;
            }
            // 2. Fetch batch from global
            if (!fetch_to_tls()) return false;
            out_idx = tls_free_list_.back();
            tls_free_list_.pop_back();
            return true;
        }

        bool fetch_to_tls() {
            std::lock_guard lock(blocks_mutex_);
            size_t needed = Batch;
            while (free_list_global_.size() < needed && blocks_.size() < BlockSize) {
                if (!create_block()) return false;
            }
            needed = std::min(free_list_global_.size(), needed);
            if (needed == 0) return false;
            for (size_t i = 0; i < needed; ++i) {
                tls_free_list_.push_back(free_list_global_.back());
                free_list_global_.pop_back();
            }
            return true;
        }

        void release_slot(uint32_t idx) {
            if (tls_free_list_.size() < TlsCache) {
                tls_free_list_.push_back(idx);
                return;
            }
            // Cache full: move half to global (simple strategy)
            std::vector<uint32_t> to_global;
            to_global.reserve(tls_free_list_.size() / 2);
            for (size_t i = 0; i < tls_free_list_.size() / 2; ++i) {
                to_global.push_back(tls_free_list_.back());
                tls_free_list_.pop_back();
            }
            {
                std::lock_guard lock(blocks_mutex_);
                for (uint32_t v: to_global) {
                    free_list_global_.push_back(v);
                }
            }
            tls_free_list_.push_back(idx);
        }

    private:
        std::vector<Block *> blocks_;
        mutable std::mutex blocks_mutex_;
        std::vector<uint32_t> free_list_global_;

        static thread_local std::vector<uint32_t> tls_free_list_;
    };

    template<typename T, size_t B, size_t S, size_t C, size_t BT>
    thread_local std::vector<uint32_t> ResourcePool<T, B, S, C, BT>::tls_free_list_;
} // namespace fermat
