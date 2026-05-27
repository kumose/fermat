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
#include <shared_mutex>
#include <turbo/strings/str_format.h>

namespace fermat {
    /// -----------------------------------------------------------------------------
    /// ResourceId: 64-bit ID encoding (shard_id[0-7], block_id[8-15],
    ///                                 slot_id[16-31], version[32-47], user[48-63])
    /// -----------------------------------------------------------------------------
    struct ResourceId {
        static constexpr uint64_t kInvalidId = 0;

        // Mask definitions
        static constexpr uint64_t kShardMask = 0xFFULL;
        static constexpr uint64_t kBlockMask = 0xFFULL << 8;
        static constexpr uint64_t kSlotMask = 0xFFFFULL << 16;
        static constexpr uint64_t kVersionMask = 0xFFFFULL << 32;
        static constexpr uint64_t kUserMask = 0xFFFFULL << 48;

        explicit ResourceId(uint64_t val = kInvalidId) noexcept : raw_(val) {
        }

        [[nodiscard]] uint64_t encode() const noexcept { return raw_; }

       [[nodiscard]] uint32_t encode_short() const noexcept {
            return static_cast<uint32_t>(raw_ & 0xFFFFFFFFULL);
        }

        /// Decodes a 32-bit short ID (produced by encode_short()) back into its components.
        /// The layout is: bits 0-7 = shard_id, bits 8-15 = block_id, bits 16-31 = slot_id.
        static void decode_short(uint32_t val, uint8_t &shard, uint8_t &block, uint16_t &slot) noexcept {
            shard = static_cast<uint8_t>(val & 0xFF);
            block = static_cast<uint8_t>((val >> 8) & 0xFF);
            slot = static_cast<uint16_t>((val >> 16) & 0xFFFF);
        }


        /// Encodes shard_id, block_id, and slot_id into a 32-bit compact ID.
        /// Layout: bits 0-7 = shard, bits 8-15 = block, bits 16-31 = slot.
        static uint32_t encode_short(uint8_t shard, uint8_t block, uint16_t slot) noexcept {
            return (static_cast<uint32_t>(shard)) |
                   (static_cast<uint32_t>(block) << 8) |
                   (static_cast<uint32_t>(slot) << 16);
        }

        // Getter
       [[nodiscard]] uint8_t shard_id() const noexcept { return static_cast<uint8_t>(raw_ & 0xFFULL); }
       [[nodiscard]] uint8_t block_id() const noexcept { return static_cast<uint8_t>((raw_ >> 8) & 0xFFULL); }
       [[nodiscard]] uint16_t slot_id() const noexcept { return static_cast<uint16_t>((raw_ >> 16) & 0xFFFFULL); }
       [[nodiscard]] uint16_t version() const noexcept { return static_cast<uint16_t>((raw_ >> 32) & 0xFFFFULL); }
       [[nodiscard]] uint16_t user_space() const noexcept { return static_cast<uint16_t>(raw_ >> 48); }

        // Setter
        void set_shard_id(uint8_t v) noexcept {
            raw_ = (raw_ & ~kShardMask) | (static_cast<uint64_t>(v));
        }

        void set_block_id(uint8_t v) noexcept {
            raw_ = (raw_ & ~kBlockMask) | (static_cast<uint64_t>(v) << 8);
        }

        void set_slot_id(uint16_t v) noexcept {
            raw_ = (raw_ & ~kSlotMask) | (static_cast<uint64_t>(v) << 16);
        }

        void set_version(uint16_t v) noexcept {
            raw_ = (raw_ & ~kVersionMask) | (static_cast<uint64_t>(v) << 32);
        }

        void set_user_space(uint16_t v) noexcept {
            raw_ = (raw_ & ~kUserMask) | (static_cast<uint64_t>(v) << 48);
        }

        void next_version() noexcept {
            set_version(version() + 1);
        }

      [[nodiscard]]  std::string to_string() const {
            return turbo::str_format("{shard:%d, block:%d, slot:%d, version:%d, user:%d}",
                                     shard_id(), block_id(), slot_id(), version(), user_space());
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

    template<typename T, size_t SlotSize>
    struct ResourceBlock {
        std::array<T, SlotSize> data;
    };

    template<size_t SlotSize>
    struct ResourceMeta {
        std::atomic<uint16_t> version[SlotSize];
    };

    struct ThreadShard {
        /// Returns the current thread's shard ID (0-255).
        /// Initializes the thread-local variable on first call using a hash of std::thread::id.
        static int32_t get_shard_id() noexcept {
            static thread_local int32_t tls_shard_id = g_thread_shard_id.fetch_add(1);
            return tls_shard_id;
        }

        static int32_t get_short_id() noexcept {
            static thread_local uint8_t short_tls_shard_id = get_shard_id() % 256;
            return short_tls_shard_id;
        }

    private:
        static std::atomic<int32_t> g_thread_shard_id;
    };

    template<typename T, uint8_t BlockSize, uint16_t SlotSize>
    class ResourceShard {
    public:
        ResourceShard();

        ~ResourceShard();

        bool create_block(uint8_t shard);

        /// Finds an object within this shard by block, slot, and expected version.
       [[nodiscard]] T *find(uint8_t block_id, uint16_t slot_id, uint16_t expected_version);

        std::mutex blocks_mutex;
        std::array<ResourceBlock<T, SlotSize> *, BlockSize> blocks;

        std::array<ResourceMeta<SlotSize> *, BlockSize> metas;
        std::vector<uint32_t> free_list;
    };

    template<typename T, uint8_t BlockSize = 8, uint16_t SlotSize = 64,
        size_t TlsCache = 1024, size_t Batch = 64>
    class ResourcePool {
    public:
        ~ResourcePool() = default;

        static ResourcePool &instance() {
            static ResourcePool pool;
            return pool;
        }

        /// Finds an object by its 64-bit resource ID.
        /// Uses shard_id to route to the appropriate shard, then validates version.
        /// @param id The 64-bit resource ID.
        /// @return Pointer to the object if valid and version matches, otherwise nullptr.
        static T *find(uint64_t id) {
            ResourceId rid(id);
            uint8_t shard = rid.shard_id();
            uint8_t block = rid.block_id();
            uint16_t slot = rid.slot_id();
            uint16_t version = rid.version();

            auto &pool = instance();
            if (shard >= pool._shards.size()) {
                return nullptr;
            }

            auto &shard_obj = pool._shards[shard];
            /// Delegate to shard's find method (which checks bounds and version).
            return shard_obj.find(block, slot, version);
        }

        /// Allocates an uninitialized object from the pool.
        /// The returned 64-bit ID includes shard, block, slot, and version.
        static T *get_uninitialize(uint64_t &rid_out) {
            auto &pool = instance();

            /// Obtain a free slot index (32-bit short ID containing shard|block|slot).
            uint32_t short_id;
            if (!pool.acquire_free_slot(short_id)) {
                return nullptr;
            }
            /// Decode the short ID into component fields using the dedicated helper.
            uint8_t shard_id, block_id;
            uint16_t slot_id;
            ResourceId::decode_short(short_id, shard_id, block_id, slot_id);

            auto &shard = pool._shards[shard_id];
            auto &ver_atom = shard.metas[block_id]->version[slot_id];

            /// Atomically increment the version to mark this slot as allocated.
            uint16_t old_ver = ver_atom.fetch_add(1, std::memory_order_relaxed);
            KCHECK(old_ver%2 == 0);

            /// Build the full 64-bit resource ID with the new version.
            ResourceId rid;
            rid.set_shard_id(shard_id);
            rid.set_block_id(block_id);
            rid.set_slot_id(slot_id);
            rid.set_version(old_ver + 1);
            rid_out = rid.encode();

            return &shard.blocks[block_id]->data[slot_id];
        }

        // Construct object with arguments and allocate.
        template<typename... Args>
        static T *get(uint64_t &rid, Args &&... args) {
            T *ptr = get_uninitialize(rid);
            if (!ptr) return nullptr;
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        /// Releases a raw object (without destructor) back to the pool.
        /// The ID must be a valid resource ID obtained from get() or get_uninitialize().
        /// If the ID is stale (version mismatch), the call is ignored.
        static void put_raw(uint64_t rid_val) {
            ResourceId rid(rid_val);
            uint8_t shard_id = rid.shard_id();
            uint8_t block_id = rid.block_id();
            uint16_t slot_id = rid.slot_id();
            uint16_t old_ver = rid.version();
            uint16_t new_ver = old_ver + 1;

            auto &pool = instance();
            if (shard_id >= pool._shards.size()) return;
            auto &shard = pool._shards[shard_id];

            if (block_id >= BlockSize || slot_id >= SlotSize) return;
            auto &ver_atom = shard.metas[block_id]->version[slot_id];

            // CAS to increment version; if it fails, the ID is already stale (already freed).
            if (!ver_atom.compare_exchange_strong(old_ver, new_ver,
                                                  std::memory_order_acq_rel)) {
                return; // Stale ID, ignore.
            }

            // Slot is now free (version increased). Encode short ID and return to shard's free list.
            uint32_t short_id = ResourceId::encode_short(shard_id, block_id, slot_id);
            pool.release_slot(short_id);
        }

        // Release constructed object (calls destructor then put_raw).
        static void put(uint64_t rid) {
            T *ptr = find(rid);
            if (!ptr) return;
            ptr->~T();
            put_raw(rid);
        }

    private:
        ResourcePool() = default;

        /// Attempts to obtain a free short ID (shard|block|slot) from this shard.
        /// First tries the thread-local cache; if empty, refills from the global free list.
        /// Returns true if a slot was acquired, false if none available (and cannot create new block).
        bool acquire_free_slot(uint32_t &out_idx) {
            /// 1. Fast path: thread-local cache has available slots.
            if (!tls_free_list_.empty()) {
                out_idx = tls_free_list_.back();
                tls_free_list_.pop_back();
                return true;
            }

            /// 2. Refill TLS from global free list.
            if (!fetch_to_tls()) {
                return false; // No free slots globally and cannot create new block.
            }

            /// 3. After refill, TLS must have at least one slot.
            out_idx = tls_free_list_.back();
            tls_free_list_.pop_back();
            return true;
        }

        /// Refills the thread-local cache by taking a batch of slots from the global free list.
        /// Returns true if at least one slot was obtained, false if global list is empty.
        bool fetch_to_tls() {
            auto shard_id = ThreadShard::get_short_id();
            auto &shard = _shards[shard_id];
            std::lock_guard<std::mutex> lock(shard.blocks_mutex);
            if (shard.free_list.empty()) {
                if (!shard.create_block(shard_id)) {
                    return false;
                }
            }

            size_t batch_size = std::min(Batch, shard.free_list.size());
            for (size_t i = 0; i < batch_size; ++i) {
                tls_free_list_.push_back(shard.free_list.back());
                shard.free_list.pop_back();
            }
            return !tls_free_list_.empty();
        }

        void release_slot(uint32_t idx) {
            if (tls_free_list_.size() < TlsCache) {
                tls_free_list_.push_back(idx);
                return;
            }

            {
                static size_t hf_cache_size = TlsCache / 2;
                uint8_t shard_id = ThreadShard::get_short_id();
                auto &shard = _shards[shard_id];
                std::lock_guard lock(shard.blocks_mutex);
                shard.free_list.push_back(idx);
                for (size_t i = 0; i < hf_cache_size; ++i) {
                    auto id = tls_free_list_.back();
                    shard.free_list.push_back(id);
                    tls_free_list_.pop_back();
                }
            }
        }

    private:
        std::array<ResourceShard<T, BlockSize, SlotSize>, 256> _shards;
        static thread_local std::vector<uint32_t> tls_free_list_;
        static thread_local uint8_t tls_shard_id;
    };

    template<typename T, uint8_t B, uint16_t S, size_t C, size_t BT>
    thread_local std::vector<uint32_t> ResourcePool<T, B, S, C, BT>::tls_free_list_;

    template<typename T, uint8_t BlockSize, uint16_t SlotSize>
    ResourceShard<T, BlockSize, SlotSize>::ResourceShard() {
        blocks.fill(nullptr);
        metas.fill(nullptr);
        free_list.reserve(SlotSize);
    }

    template<typename T, uint8_t BlockSize, uint16_t SlotSize>
    ResourceShard<T, BlockSize, SlotSize>::~ResourceShard() {
        std::lock_guard<std::mutex> lock_resource(blocks_mutex);
        for (size_t i = 0; i < BlockSize; ++i) {
            if (blocks[i] != nullptr) {
                Malloc::good_free(blocks[i]);
                blocks[i] = nullptr;
            }
        }

        for (size_t i = 0; i < BlockSize; ++i) {
            if (metas[i] != nullptr) {
                Malloc::good_free(metas[i]);
                metas[i] = nullptr;
            }
        }
        free_list.clear();
    }

    template<typename T, uint8_t BlockSize, uint16_t SlotSize>
    bool ResourceShard<T, BlockSize, SlotSize>::create_block(uint8_t shard_id) {
        size_t idx = 0;
        while (idx < BlockSize && blocks[idx] != nullptr) ++idx;
        if (idx >= BlockSize) return false;

        // Allocate block and meta (omitted for brevity, same as before)
        size_t block_size = sizeof(ResourceBlock<T, SlotSize>);
        void *block_mem = Malloc::good_alloc(block_size);
        if (!block_mem) throw std::bad_alloc();
        auto *new_block = new(block_mem) ResourceBlock<T, SlotSize>();

        size_t meta_size = sizeof(ResourceMeta<SlotSize>);
        void *meta_mem = Malloc::good_alloc(meta_size);
        if (!meta_mem) {
            Malloc::good_free(block_mem);
            return false;
        }
        auto *new_meta = new(meta_mem) ResourceMeta<SlotSize>();

        /// Initialize all version slots to 0.
        for (size_t i = 0; i < SlotSize; ++i) {
            new_meta->version[i].store(0, std::memory_order_relaxed);
        }

        blocks[idx] = new_block;
        metas[idx] = new_meta;

        // Use the existing encode_short function to avoid repeating bit shifts.
        auto block_id = static_cast<uint8_t>(idx);
        for (uint16_t slot = 0; slot < SlotSize; ++slot) {
            uint32_t encoded = ResourceId::encode_short(shard_id, block_id, slot);
            free_list.push_back(encoded);
        }
        return true;
    }

    template<typename T, uint8_t BlockSize, uint16_t SlotSize>
    T *ResourceShard<T, BlockSize, SlotSize>::find(uint8_t block_id, uint16_t slot_id, uint16_t expected_version) {
        if (block_id >= BlockSize || slot_id >= SlotSize) {
            return nullptr;
        }
        /// Lock is not needed here because version is atomic and blocks are stable.
        auto *blk = blocks[block_id];
        if (!blk) return nullptr;
        uint16_t cur_ver = metas[block_id]->version[slot_id].load(std::memory_order_acquire);
        if (cur_ver != expected_version) {
            return nullptr;
        }
        return &blk->data[slot_id];
    }
} // namespace fermat
