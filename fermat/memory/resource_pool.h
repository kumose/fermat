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
#include <turbo/base/nullability.h>

namespace fermat {
    /// @brief Zero-storage proxy that operates on resource metadata.
    /// It derives metadata addresses from the base ID pointer.
    struct ResourceId {
        /// @brief Physical layout: Immutable index + Atomic status.
        struct ControlBlock {
            uint16_t _block; /// Byte 0-1: Physical Block ID
            uint16_t _slot; /// Byte 2-3: Physical Slot ID
            std::atomic<uint16_t> _version; /// Byte 4-5: Atomic Version
            std::atomic<uint16_t> _user; /// Byte 6-7: Atomic User Space
        };

        /// @brief Constructor derives sibling pointers from the provided base.
        /// @param rid Raw pointer (wrapped in Nonnull) to the 64-bit metadata.
        explicit ResourceId(turbo::Nonnull<int64_t *> rid) noexcept {
            /// Since rid is just a pointer, we cast it to our structured ControlBlock.
            auto *base = reinterpret_cast<ControlBlock *>(rid);
            block_ptr = &base->_block;
            slot_ptr = &base->_slot;
            version_ptr = &base->_version;
            user_ptr = &base->_user;
        }

        uint16_t *block_ptr{nullptr};
        uint16_t *slot_ptr{nullptr};
        std::atomic<uint16_t> *version_ptr{nullptr};
        std::atomic<uint16_t> *user_ptr{nullptr};

        // --- Metadata Getters ---

        [[nodiscard]] uint16_t block_id() const noexcept { return *block_ptr; }
        [[nodiscard]] uint16_t slot_id() const noexcept { return *slot_ptr; }

        [[nodiscard]] uint16_t version(std::memory_order order = std::memory_order_relaxed) const noexcept {
            return version_ptr->load(order);
        }

        [[nodiscard]] uint16_t user_space(std::memory_order order = std::memory_order_relaxed) const noexcept {
            return user_ptr->load(order);
        }

        // --- Metadata Setters ---

        /// @brief Set physical Block ID (Non-atomic).
        void set_block_id(uint16_t val) noexcept { *block_ptr = val; }

        /// @brief Set physical Slot ID (Non-atomic).
        void set_slot_id(uint16_t val) noexcept { *slot_ptr = val; }

        /// @brief Set version with custom barrier.
        void set_version(uint16_t val, std::memory_order order = std::memory_order_release) noexcept {
            version_ptr->store(val, order);
        }

        /// @brief Update user space with custom barrier.
        void set_user_space(uint16_t val, std::memory_order order = std::memory_order_release) noexcept {
            user_ptr->store(val, order);
        }

        // --- Lifecycle Management ---

        /// @brief Atomic version bump for ABA protection.
        void next_version(std::memory_order order = std::memory_order_release) noexcept {
            version_ptr->fetch_add(1, order);
        }

        /// @brief Reconstruct the 64-bit ID for the user.
        [[nodiscard]] int64_t encode() const noexcept {
            /// Cast to atomic<int64_t> for a consistent 8-byte snapshot.
            auto *atomic_id = reinterpret_cast<const std::atomic<int64_t> *>(block_ptr);
            return atomic_id->load(std::memory_order_acquire);
        }
    };

    static constexpr uint16_t kDefaultBlockSize = 8096;
    static constexpr uint16_t kDefaultSlotSize = 4096;

    template<typename T, size_t BlockSize = kDefaultBlockSize,
        size_t SlotSize = kDefaultSlotSize, size_t TlsCache = 1024, size_t Batch = 64>
    class ResourcePool {
    public:
        ~ResourcePool() {
            std::unique_lock lock(_blocks_mutex);
            for (size_t i = 0; i < _blocks.size(); ++i) {
                auto *block = _blocks[i];
                auto n = block->n;
                Malloc::good_free(block, n);
            }
            _blocks.clear();
        }

        struct Block {
            size_t n{0};
            T data[SlotSize];
        };

        static ResourcePool &instance() {
            static ResourcePool ins;
            return ins;
        }

        static T *find(const ResourceId &rid) {
            auto &pool = instance();
            uint16_t bid = rid.block_id();
            uint16_t sid = rid.slot_id();

            // Thread-safe access because _blocks only grows
            if (bid >= pool._blocks.size()) return nullptr;
            if (sid >= SlotSize) return nullptr;
            return &pool._blocks[bid]->data[sid];
        }

        static T *find(int64_t id) {
            ResourceId rid(&id);
            return find(rid);
        }

        static T *get_uninitialize(int64_t &rid) {
            auto &pool = instance();
            /// 1. Check thread-local cache first
            if (tls_free_list.empty()) {
                /// 2. If empty, fetch from global or allocate new block
                if (!pool.fetch_to_tls()) return nullptr;
            }

            rid = tls_free_list.back();
            tls_free_list.pop_back();

            return find(rid);
        }

        /// @brief Primary allocation logic using the proxy.
        static T *get_uninitialize(ResourceId &rid_proxy) {
            auto &pool = instance();
            if (tls_free_list.empty()) {
                if (!pool.fetch_to_tls()) return nullptr;
            }

            int64_t raw_id = tls_free_list.back();
            tls_free_list.pop_back();
            ResourceId get_id(&raw_id);
            rid_proxy.set_block_id(get_id.block_id());
            rid_proxy.set_slot_id(get_id.slot_id());
            rid_proxy.set_version(get_id.version());
            rid_proxy.set_user_space(get_id.user_space());

            return &pool._blocks[get_id.block_id()]->data[get_id.slot_id()];
        }

        /// @brief Helper to acquire and construct an object.
        template<typename... Args>
        static T *get(ResourceId &rid, Args &&... args) {
            auto ptr = get_uninitialize(rid);
            if (ptr == nullptr) {
                return nullptr;
            }
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        template<typename... Args>
        static T *get(int64_t &rid, Args &&... args) {
            ResourceId rid_proxy(&rid);
            return get(rid_proxy, std::forward<Args>(args)...);
        }

        static void put_raw(const ResourceId &rid) {
            if (tls_free_list.size() < TlsCache) {
                tls_free_list.push_back(rid.encode());
                return;
            }
            std::vector<int64_t> ids;
            ids.reserve(tls_free_list.size());
            ids.swap(tls_free_list);
            tls_free_list.push_back(rid.encode());
            auto n = ids.size() / 2;
            for (size_t i = 0; i < n; ++i) {
                tls_free_list.push_back(ids.back());
                ids.pop_back();
            }
            instance().return_to_global(ids);
        }

        static void put(const ResourceId &rid) {
            auto ptr = find(rid);
            if (ptr == nullptr) {
                return;
            }
            destroy(ptr);
            put_raw(rid);
        }

        static void put_raw(int64_t rid) {
            ResourceId rid_proxy(&rid);
            put_raw(rid_proxy);
        }

        static void put(int64_t rid) {
            ResourceId rid_proxy(&rid);
            put_raw(rid_proxy);
        }

    private:
        ResourcePool() {
            std::unique_lock lk(_blocks_mutex);
            _blocks.clear();
            _blocks.reserve(BlockSize);
            if (!create_block()) {
                throw std::bad_alloc();
            }
        }

        /// @brief Helper to destroy and return an object.
        static void destroy(T *ptr) {
            if (ptr) {
                ptr->~T();
            }
        }

        bool fetch_to_tls(size_t n = Batch) {
            if (n == 0) {
                return true;
            }
            std::unique_lock lk(_blocks_mutex);

            if (_g_free_list.size() < n && _blocks.size() < BlockSize) {
                if (!create_block()) {
                    return false;
                }
            }

            auto real_size = std::min(_g_free_list.size(), n);
            if (real_size == 0 ) {
                return false;
            }
            for (size_t i = 0; i < real_size; ++i) {
                tls_free_list.push_back(_g_free_list.back());
                _g_free_list.pop_back();
            }
            return true;
        }

        bool return_to_global(const std::vector<int64_t> &ids) {
            std::unique_lock lk(_blocks_mutex);
            for (size_t i = 0; i < ids.size(); ++i) {
                _g_free_list.push_back(ids[i]);
            }
            return true;
        }

        bool create_block() {
            auto n = sizeof(Block);
            auto ptr = reinterpret_cast<Block *>(Malloc::good_alloc(&n));
            if (!ptr) {
                return false;
            }
            ptr->n = n;
            _blocks.push_back(ptr);
            auto bid = _blocks.size() - 1;
            for (size_t i = 0; i < SlotSize; i++) {
                int64_t id;
                ResourceId rid(&id);
                rid.set_block_id(bid);
                rid.set_slot_id(i);
                rid.set_version(1);
                _g_free_list.push_back(id);
            }
            return true;
        }

    private:
        std::vector<Block *> _blocks;
        std::mutex _blocks_mutex;
        std::vector<int64_t> _g_free_list;
        static thread_local std::vector<int64_t> tls_free_list;
    };

    template<typename T, size_t B, size_t S, size_t C, size_t BT>
    thread_local std::vector<int64_t> ResourcePool<T, B, S, C, BT>::tls_free_list = {};
} // namespace fermat
