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

namespace fermat {
    /// @brief A high-performance object pool with thread-local caching.
    /// @tparam T The type of object to pool.
    /// @tparam MaxFree Maximum number of objects to cache per thread.
    template<class T, size_t MaxFree = 1024>
    class ObjectPool {
    private:
        /// @brief Internal manager to handle thread-local storage and cleanup.
        struct ThreadLocalCache {
            std::vector<T*> cache;

            ThreadLocalCache() {
                /// Pre-allocate capacity to avoid reallocations during push_back.
                cache.reserve(MaxFree);
            }

            ~ThreadLocalCache() {
                /// Thread is exiting: return all cached memory to mimalloc.
                /// Using sized-deallocation for maximum performance.
                const size_t rn = Malloc::good_alloc_size(sizeof(T));
                for (T* ptr : cache) {
                    Malloc::good_free(ptr, rn);
                }
                cache.clear();
            }
        };

        /// @brief Get the thread-local cache instance.
        static ThreadLocalCache& get_tls() {
            static thread_local ThreadLocalCache tls;
            return tls;
        }

    public:
        /// @brief Acquire an uninitialized memory block for type T.
        /// @return Pointer to the memory block.
        static T* get_uninitialize() {
            auto& tls = get_tls();
            if (tls.cache.empty()) {
                size_t n = sizeof(T);
                /// Fetch from mimalloc if local cache is empty.
                return static_cast<T*>(Malloc::good_alloc(&n));
            }
            /// LIFO: Best for CPU cache locality.
            T* ptr = tls.cache.back();
            tls.cache.pop_back();
            return ptr;
        }

        /// @brief Helper to acquire and construct an object.
        template<typename... Args>
        static T* get(Args&&... args) {
            T* ptr = get_uninitialize();
            if (ptr) {
                ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
            }
            return ptr;
        }

        /// @brief Return a memory block to the local cache or free it if full.
        /// @param ptr The pointer to return.
        static void put_raw(T* ptr) {
            if (ptr == nullptr) return;

            auto& tls = get_tls();
            if (tls.cache.size() >= MaxFree) {
                /// Cache is full: return to mimalloc to prevent memory bloat.
                const size_t rn = Malloc::good_alloc_size(sizeof(T));
                Malloc::good_free(ptr, rn);
                return;
            }
            tls.cache.push_back(ptr);
        }
        static void put(T* ptr) {
            if (ptr == nullptr) return;
            destroy(ptr);
            put_raw(ptr);
        }

    private:
        /// @brief Helper to destroy and return an object.
        static void destroy(T* ptr) {
            if (ptr) {
                ptr->~T();
            }
        }
    };
} // namespace fermat
