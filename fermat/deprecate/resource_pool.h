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
// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.

#pragma once

#include <fermat/memory/allocator.h>
#include <atomic>
#include <mutex>
#include <turbo/strings/str_format.h>

namespace fermat {

struct SimpleResourceId {
    static constexpr uint64_t kInvalidId = 0;
    static constexpr uint64_t kVersionMask = 0xFFFFULL << 32;
    static constexpr uint64_t kUserMask = 0xFFFFULL << 48;

    explicit SimpleResourceId(uint64_t val = kInvalidId) noexcept : raw_(val) {}

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

template<typename T, size_t BlockSize = 8, size_t SlotSize = 64,
         size_t TlsCache = 1024, size_t Batch = 64>
class SimpleResourcePool {
public:
    struct Block {
        alignas(64) std::atomic<uint16_t> version[SlotSize];
        T data[SlotSize];
    };

    ~SimpleResourcePool() {
        std::lock_guard lock(blocks_mutex_);
        for (Block* blk : blocks_) {
            Malloc::good_free(blk);
        }
    }

    static SimpleResourcePool& instance() {
        static SimpleResourcePool pool;
        return pool;
    }

    static T* find(int64_t id) {
        SimpleResourceId rid(id);
        uint16_t bid = rid.block_id();
        uint16_t sid = rid.slot_id();
        auto& pool = instance();
        if (bid >= pool.blocks_.size() || sid >= SlotSize) return nullptr;
        Block* blk = pool.blocks_[bid];
        uint16_t cur_ver = blk->version[sid].load(std::memory_order_acquire);
        if (cur_ver != rid.version()) return nullptr;
        return &blk->data[sid];
    }

    static T* get_uninitialize(int64_t& rid_out) {
        auto& pool = instance();
        uint32_t idx;
        if (!pool.acquire_free_slot(idx)) return nullptr;

        uint16_t bid = static_cast<uint16_t>(idx >> 16);
        uint16_t sid = static_cast<uint16_t>(idx & 0xFFFF);
        Block* blk = pool.blocks_[bid];
        std::atomic<uint16_t>& ver_atom = blk->version[sid];

        uint16_t old_ver = ver_atom.load(std::memory_order_relaxed);
        uint16_t new_ver = old_ver + 1;
        if (!ver_atom.compare_exchange_strong(old_ver, new_ver,
                                              std::memory_order_acq_rel)) {
            return get_uninitialize(rid_out);
        }

        SimpleResourceId rid;
        rid.set_block_id(bid);
        rid.set_slot_id(sid);
        rid.set_version(new_ver);
        rid_out = rid.encode();
        return &blk->data[sid];
    }

    template<typename... Args>
    static T* get(int64_t& rid, Args&&... args) {
        T* ptr = get_uninitialize(rid);
        if (!ptr) return nullptr;
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    static void put_raw(int64_t rid_val) {
        SimpleResourceId rid(rid_val);
        uint16_t bid = rid.block_id();
        uint16_t sid = rid.slot_id();
        auto& pool = instance();
        if (bid >= pool.blocks_.size() || sid >= SlotSize) return;
        Block* blk = pool.blocks_[bid];
        std::atomic<uint16_t>& ver_atom = blk->version[sid];

        uint16_t old_ver = rid.version();
        uint16_t new_ver = old_ver + 1;
        if (!ver_atom.compare_exchange_strong(old_ver, new_ver,
                                              std::memory_order_acq_rel)) {
            return;
        }
        uint32_t idx = (static_cast<uint32_t>(bid) << 16) | sid;
        pool.release_slot(idx);
    }

    static void put(int64_t rid) {
        T* ptr = find(rid);
        if (!ptr) return;
        ptr->~T();
        put_raw(rid);
    }

private:
    SimpleResourcePool() {
        std::lock_guard lock(blocks_mutex_);
        blocks_.reserve(BlockSize);
        if (!create_block()) throw std::bad_alloc();
    }

    bool create_block() {
        size_t n = sizeof(Block);
        void* mem = Malloc::good_alloc(&n);
        if (!mem) return false;
        Block* blk = new (mem) Block;
        // Initialize versions to 1 (odd number, indicating free)
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

    bool acquire_free_slot(uint32_t& out_idx) {
        // Fast path: thread-local cache
        if (!tls_free_list_.empty()) {
            out_idx = tls_free_list_.back();
            tls_free_list_.pop_back();
            return true;
        }
        // Refill from global free list
        if (!fetch_to_tls()) return false;
        out_idx = tls_free_list_.back();
        tls_free_list_.pop_back();
        return true;
    }

    bool fetch_to_tls() {
        std::lock_guard lock(blocks_mutex_);
        size_t needed = Batch;
        // Ensure global list has enough slots; create new blocks if needed
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
        // Always return the slot directly to the global free list.
        // This avoids holding slots in thread-local caches, which can cause
        // global starvation under heavy concurrency.
        std::lock_guard lock(blocks_mutex_);
        free_list_global_.push_back(idx);
    }

private:
    std::vector<Block*> blocks_;
    mutable std::mutex blocks_mutex_;
    std::vector<uint32_t> free_list_global_;

    static thread_local std::vector<uint32_t> tls_free_list_;
};

template<typename T, size_t B, size_t S, size_t C, size_t BT>
thread_local std::vector<uint32_t> SimpleResourcePool<T, B, S, C, BT>::tls_free_list_;

} // namespace fermat