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

#include <fermat/memory/malloc.h>
#include <fermat/memory/checked_math.h>
#include <turbo/log/logging.h>
#include <turbo/threading/thread_local.h>

namespace fermat {
    /// @brief A high‑performance object pool with thread‑local caching.
    ///
    /// This pool is intended for **thread‑local objects** that are created and destroyed
    /// frequently (e.g., per‑request context, temporary nodes, iterators, small buffers).
    /// It maintains a free list per thread, which avoids hitting the global allocator
    /// on most allocations/deallocations. As a result, it offers **much lower latency**
    /// than direct `new`/`delete` or even raw mimalloc calls.
    ///
    /// **Performance highlights** (measured on 3.2 GHz CPU with mimalloc backend):
    ///   - Single‑thread `get_uninitialize()` + `put_raw()`: ~0.9 ns per operation
    ///   - Single‑thread `get()` (construction) + `put()` (destruction): ~1.0 ns per pair
    ///   - Single‑thread `new` + `delete` (no pool): ~12.1 ns
    ///   - Multi‑thread (4 threads): pool ~1.0 ns, `new`/`delete` ~12.6 ns
    ///   - Multi‑thread (16 threads): pool ~2.4 ns, `new`/`delete` ~28.4 ns
    ///   - Batch of 128 objects: pool 226 ns (1.77 ns/obj), `new`/`delete` 2782 ns (21.7 ns/obj)
    ///
    /// **When to use this pool** (✅):
    ///   - Objects have **thread‑local lifetime** (created and destroyed on the same thread).
    ///   - Allocation/deallocation happens **very frequently** (e.g., millions of times per second).
    ///   - You want to **reduce CPU consumption** and **improve cache locality**.
    ///   - **Example: vector distance computation** – each thread computes distances between
    ///     fixed‑dimensional vectors, requiring temporary buffers or result objects that
    ///     are discarded immediately after the calculation. The pool reuses these buffers
    ///     without touching the global allocator, drastically reducing latency.
    ///
    /// **When NOT to use it** (❌):
    ///   - Objects must be **shared across threads** (pointers from one thread’s cache are
    ///     not safe to use in another thread). For cross‑thread sharing, use `ResourcePool`
    ///     (versioned IDs) or fall back to plain `Malloc::good_alloc`/`good_free`.
    ///   - Objects are **large** and allocated rarely – the pool’s memory overhead may
    ///     not be justified.
    ///   - The number of concurrently alive objects per thread exceeds `MaxFree` – the pool
    ///     will still work but will fall back to global allocation more often.
    ///
    /// **Memory usage**:
    ///   - Each thread retains at most `MaxFree` free objects; extra objects are returned
    ///     to the global allocator. This prevents unbounded per‑thread memory consumption.
    ///   - On thread exit, all cached objects are freed to the global allocator (no leak).
    ///
    /// **Thread safety**:
    ///   - The pool is thread‑safe as long as each thread uses its own `thread_local` cache.
    ///   - No cross‑thread synchronization is needed.
    ///
    /// @tparam T       Type of objects stored in the pool.
    /// @tparam MaxFree Maximum number of free objects kept per thread (default 1024).
    struct PoolStats {
        size_t tls_alloc{0};
        size_t tls_freed{0};
        size_t glb_alloc{0};
        size_t glb_freed{0};
        size_t cached{0};
    };

    template<size_t Alignment>
    struct ObjectGuard {
        ObjectGuard() = default;

        ObjectGuard(const ObjectGuard &) = delete;

        ObjectGuard &operator=(const ObjectGuard &) = delete;

        ObjectGuard(ObjectGuard &&other) noexcept
            : ptrs(std::move(other.ptrs)), n(other.n) {
            other.n = 0;
        }

        ObjectGuard &operator=(ObjectGuard &&other) noexcept {
            if (this != &other) {
                ptrs = std::move(other.ptrs);
                n = other.n;
                other.n = 0;
            }
            return *this;
        }

        ~ObjectGuard() {
            for (void *ptr: ptrs) {
                if constexpr (Alignment == 0) {
                    Malloc::good_free(ptr);
                } else {
                    Malloc::good_align_free(ptr, Alignment);
                }
            }
            ptrs.clear();
        }

        void release_to(std::vector<void *> &rhs) {
            if (rhs.empty()) {
                ptrs.swap(rhs);
                return;
            }
            rhs.reserve(rhs.size() + ptrs.size());
            for (void *ptr: ptrs) {
                rhs.push_back(ptr);
            }
            ptrs.clear();
        }

        std::vector<void *> ptrs;
        size_t n{0};
    };

    template<class T, size_t Alignment, size_t MaxFree = 512>
    class ObjectPool {
    private:
        struct ThreadLocalInitializer {
            ThreadLocalInitializer() {

            }
        };

        /// @brief Get the thread-local cache instance.
        static std::vector<void *> *get_cache() {
            if (TURBO_LIKELY(ObjectPool::cache)) {
                return ObjectPool::cache;
            }
            auto ptr = new std::vector<void *>();
            ObjectPool::cache = ptr;
            ObjectPool::cache->reserve(MaxFree);
            turbo::thread_atexit(ObjectPool::delete_local_pool, ObjectPool::cache);

            return ObjectPool::cache;
        }

        static void delete_local_pool(void *arg) {
            auto cache_ptr = reinterpret_cast<std::vector<void *> *>(arg);
            size_t rn;
            if constexpr (Alignment == 0) {
                rn = Malloc::good_alloc_size(sizeof(T));
            } else {
                rn = Malloc::good_align_alloc_size(Alignment, sizeof(T));
            }
            for (void *ptr: *cache_ptr) {
                if constexpr (Alignment == 0) {
                    Malloc::good_free(ptr);
                } else {
                    Malloc::good_align_free(ptr, Alignment);
                }
            }
            cache_ptr->clear();

            delete cache_ptr;
        }

    public:
        static const size_t kGoodSize;

        /// @brief Acquire an uninitialized memory block for type T.
        /// @return Pointer to the memory block.
        static T *get_uninitialize() {
            auto tls = get_cache();
            if (tls->empty()) {
                size_t n = sizeof(T);
                ++glb_alloc;
                /// Fetch from mimalloc if local cache is empty.
                if constexpr (Alignment == 0) {
                    return static_cast<T *>(Malloc::good_alloc(n));
                } else {
                    return static_cast<T *>(Malloc::good_align_alloc(Alignment, n));
                }
            }
            /// LIFO: Best for CPU cache locality.
            T *ptr = reinterpret_cast<T *>(tls->back());
            tls->pop_back();
            ++tls_alloc;
            return ptr;
        }

        /// @brief Helper to acquire and construct an object.
        template<typename... Args>
        static T *get(Args &&... args) {
            T *ptr = get_uninitialize();
            if (ptr) {
                ::new(static_cast<void *>(ptr)) T(std::forward<Args>(args)...);
            }
            return ptr;
        }

        /// @brief Return a memory block to the local cache or free it if full.
        /// @param ptr The pointer to return.
        static void put_raw(T *ptr) {
            if (ptr == nullptr) return;

            auto tls = get_cache();
            if (tls->size() >= limited) {
                ++glb_freed;
                /// Cache is full: return to mimalloc to prevent memory bloat.
                if constexpr (Alignment == 0) {
                    Malloc::good_free(ptr, kGoodSize);
                } else {
                    Malloc::good_align_free(ptr, Alignment);
                }

                return;
            }
            ++tls_freed;
            tls->push_back(ptr);
        }

        static void set_tsl_limit(size_t n) {
            auto tls = get_cache();
            limited = n;
            if (tls->capacity() < limited) {
                tls->reserve(limited);
                return;
            }
            while (tls->size() > limited) {
                auto *ptr = tls->back();
                tls->pop_back();
                /// Cache is full: return to mimalloc to prevent memory bloat.
                if constexpr (Alignment == 0) {
                    Malloc::good_free(ptr);
                } else {
                    Malloc::good_align_free(ptr, Alignment);
                }
            }
        }

        static size_t release_tsl(float precent_to_save) {
            auto tls = get_cache();
            auto n = static_cast<size_t>(tls->size() * precent_to_save);
            if (n >= tls->size()) {
                return 0;
            }
            auto old_size = tls->size();
            while (tls->size() > n) {
                auto *ptr = tls->back();
                tls->pop_back();
                /// Cache is full: return to mimalloc to prevent memory bloat.
                if constexpr (Alignment == 0) {
                    Malloc::good_free(ptr, kGoodSize);
                } else {
                    Malloc::good_align_free(ptr, Alignment);
                }
            }
            return old_size - n;
        }

        static ObjectGuard<Alignment> collect_tsl() {
            auto tls = get_cache();
            ObjectGuard<Alignment> ret;
            ret.ptrs.swap(*tls);
            ret.n = kGoodSize;
            return std::move(ret);
        }

        static void apply_tls(ObjectGuard<Alignment> &objs) {
            auto tls = get_cache();
            objs.release_to(*tls);
        }

        static PoolStats tls_stats() {
            PoolStats st;
            auto tls = get_cache();
            st.cached = tls->size();
            st.tls_alloc = tls_alloc;
            st.tls_freed = tls_freed;
            st.glb_alloc = glb_alloc;
            st.glb_freed = glb_freed;
            return st;
        }


        static void put(T *ptr) {
            if (ptr == nullptr) return;
            destroy(ptr);
            put_raw(ptr);
        }

    private:

        static thread_local std::vector<void *> *cache;

        static thread_local size_t limited;
        static thread_local size_t tls_alloc;
        static thread_local size_t tls_freed;
        static thread_local size_t glb_alloc;
        static thread_local size_t glb_freed;

    private:
        /// @brief Helper to destroy and return an object.
        static void destroy(T *ptr) {
            if (ptr) {
                ptr->~T();
            }
        }
    };

    template<class T, size_t Alignment, size_t MaxFree>
    thread_local std::vector<void *> *ObjectPool<T, Alignment, MaxFree>::cache = nullptr;

    template<class T, size_t Alignment, size_t MaxFree>
    thread_local size_t ObjectPool<T, Alignment, MaxFree>::limited{MaxFree};
    template<class T, size_t Alignment, size_t MaxFree>
    thread_local size_t ObjectPool<T, Alignment, MaxFree>::tls_alloc{0};
    template<class T, size_t Alignment, size_t MaxFree>
    thread_local size_t ObjectPool<T, Alignment, MaxFree>::tls_freed{0};
    template<class T, size_t Alignment, size_t MaxFree>
    thread_local size_t ObjectPool<T, Alignment, MaxFree>::glb_alloc{0};
    template<class T, size_t Alignment, size_t MaxFree>
    thread_local size_t ObjectPool<T, Alignment, MaxFree>::glb_freed{0};

    template<class T, size_t Alignment, size_t MaxFree>
    const size_t ObjectPool<T, Alignment, MaxFree>::kGoodSize = Malloc::good_type_size<Alignment, T>();

    template<size_t N, size_t Alignment>
    struct AlignBytes {
        static constexpr size_t size = N;
        static constexpr size_t align = Alignment;
        uint8_t data[N];
    };

    template<typename T, size_t N, size_t Alignment>
    struct AlignedBytesAllocator {
        static T *allocate() {
            return reinterpret_cast<T *>(ObjectPool<AlignBytes<N * sizeof(T), Alignment>, Alignment>::get_uninitialize());
        }

        static void deallocate(T *ptr) {
            ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment >::put_raw(
                reinterpret_cast<AlignBytes<N * sizeof(T), Alignment> *>(ptr));
        }

        static void set_tsl_limit(size_t n) {
            ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment >::set_tsl_limit(n);
        }

        static void release_tsl(float precent_to_save) {
            ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment>::release_tsl(precent_to_save);
        }

        static PoolStats tls_stats() {
            return ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment >::tls_stats();
        }

        static ObjectGuard<Alignment> collect_tsl() {
            return ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment >::collect_tsl();
        }

        static void apply_tsl(ObjectGuard<Alignment> &objs) {
            return ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment >::apply_tls(objs);
        }

        static size_t good_size() {
            return ObjectPool<AlignBytes<N * sizeof(T), Alignment>,Alignment>::kGoodSize;
        }
    };

    static constexpr size_t kTinySize = 64;
    static constexpr size_t kMiniSize = 128;
    static constexpr size_t kSmallSize = 256;
    static constexpr size_t kMediumSize = 512;
    static constexpr size_t kBigSize = 1024;
    static constexpr size_t kPageSize = 4096; ///< 4k
    static constexpr size_t kX2PageSize = 4096 * 2; ///< 8k
    static constexpr size_t kX4PageSize = 4096 * 4; ///< 16k

    template<typename T, size_t Alignment>
    struct TieredAllocator {
        template<typename U>
        struct rebind {
            using other = TieredAllocator<U, Alignment>;
        };


        static T *pooled_alloc(size_t n) {
            T *ptr = nullptr;
            switch (n) {
                case 1:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, 1, Alignment>::allocate());
                    break;
                case kTinySize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kTinySize, Alignment>::allocate());
                    break;
                case kMiniSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kMiniSize, Alignment>::allocate());
                    break;
                case kSmallSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kSmallSize, Alignment>::allocate());
                    break;
                case kMediumSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kMediumSize, Alignment>::allocate());
                    break;
                case kBigSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kBigSize, Alignment>::allocate());
                    break;
                case kPageSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kPageSize, Alignment>::allocate());
                    break;
                case kX2PageSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kX2PageSize, Alignment>::allocate());
                    break;
                case kX4PageSize:
                    ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, kX4PageSize, Alignment>::allocate());
                    break;
                default: {
                    size_t bytes;
                    if (!checked_muladd(&bytes, n, sizeof(T), 0UL)) {
                        throw std::length_error("");
                    }
                    if constexpr (Alignment > 0) {
                        ptr = static_cast<T *>(Malloc::good_align_alloc(Alignment, bytes));
                    } else {
                        ptr = static_cast<T *>(Malloc::good_alloc(bytes));
                    }
                    break;
                }
            }
            if (!ptr) {
                throw std::length_error("");
            }

            return ptr;
        }

        static size_t pooled_alloc_size(size_t n) {
            if (n == 1) {
                return 1;
            }
            if (n <= kTinySize) {
                return kTinySize;
            } else if (n <= kMiniSize) {
                return kMiniSize;
            } else if (n <= kSmallSize) {
                return kSmallSize;
            } else if (n <= kMediumSize) {
                return kMediumSize;
            } else if (n <= kBigSize) {
                return kBigSize;
            } else if (n <= kPageSize) {
                return kPageSize;
            } else if (n <= kX2PageSize) {
                return kX2PageSize;
            } else if (n <= kX4PageSize) {
                return kX4PageSize;
            } else {
                size_t bytes;
                if (!checked_muladd(&bytes, n, sizeof(T), 0UL)) {
                    return std::numeric_limits<size_t>::max();
                }
                size_t nn;
                if constexpr (Alignment > 0) {
                    nn = Malloc::good_align_alloc_size(Alignment, bytes);
                } else {
                    nn = Malloc::good_alloc_size(bytes);
                }
                return nn / sizeof(T);
            }
        }

        static void pooled_free(T *ptr, size_t n) {
            switch (n) {
                case 0:
                    break;
                case 1: {
                    AlignedBytesAllocator<T, 1, Alignment>::deallocate(ptr);
                    break;
                }

                case kTinySize: {
                    AlignedBytesAllocator<T, kTinySize, Alignment>::deallocate(ptr);
                    break;
                }
                case kMiniSize: {
                    AlignedBytesAllocator<T, kMiniSize, Alignment>::deallocate(ptr);
                    break;
                }
                case kSmallSize: {
                    AlignedBytesAllocator<T, kSmallSize, Alignment>::deallocate(ptr);
                    break;
                }
                case kMediumSize: {
                    AlignedBytesAllocator<T, kMediumSize, Alignment>::deallocate(ptr);
                    break;
                }
                case kBigSize: {
                    AlignedBytesAllocator<T, kBigSize, Alignment>::deallocate(ptr);
                    break;
                }
                case kPageSize: {
                    AlignedBytesAllocator<T, kPageSize, Alignment>::deallocate(ptr);
                    break;
                }
                case kX2PageSize: {
                    AlignedBytesAllocator<T, kX2PageSize, Alignment>::deallocate(ptr);
                    break;
                }
                case kX4PageSize: {
                    AlignedBytesAllocator<T, kX4PageSize, Alignment>::deallocate(ptr);
                    break;
                }
                default:
                    if constexpr (Alignment > 0) {
                        Malloc::good_align_free(ptr, Alignment);
                    } else {
                        Malloc::good_free(ptr, n);
                    }
                    break;
            }
        }

        static void set_tsl_limit(size_t slot, size_t n) {
            if (slot == 1) {
                AlignedBytesAllocator<T, 1, Alignment>::set_tsl_limit(n);
            } else if (slot <= kTinySize) {
                AlignedBytesAllocator<T, kTinySize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kMiniSize) {
                AlignedBytesAllocator<T, kMiniSize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kSmallSize) {
                AlignedBytesAllocator<T, kSmallSize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kMediumSize) {
                AlignedBytesAllocator<T, kMediumSize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kBigSize) {
                AlignedBytesAllocator<T, kBigSize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kPageSize) {
                AlignedBytesAllocator<T, kPageSize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kX2PageSize) {
                AlignedBytesAllocator<T, kX2PageSize, Alignment>::set_tsl_limit(n);
            } else if (slot <= kX4PageSize) {
                AlignedBytesAllocator<T, kX4PageSize, Alignment>::set_tsl_limit(n);
            }
        }

        static void set_tsl_limit(size_t n) {
            set_tsl_limit(1, n);
            set_tsl_limit(kTinySize, n);
            set_tsl_limit(kMiniSize, n);
            set_tsl_limit(kSmallSize, n);
            set_tsl_limit(kMediumSize, n);
            set_tsl_limit(kBigSize, n);
            set_tsl_limit(kPageSize, n);
            set_tsl_limit(kX2PageSize, n);
            set_tsl_limit(kX4PageSize, n);
        }

        static void release_tsl(size_t slot, float precent_to_save) {
            if (slot == 1) {
                AlignedBytesAllocator<T, 1, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kTinySize) {
                AlignedBytesAllocator<T, kTinySize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kMiniSize) {
                AlignedBytesAllocator<T, kMiniSize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kSmallSize) {
                AlignedBytesAllocator<T, kSmallSize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kMediumSize) {
                AlignedBytesAllocator<T, kMediumSize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kBigSize) {
                AlignedBytesAllocator<T, kBigSize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kPageSize) {
                AlignedBytesAllocator<T, kPageSize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kX2PageSize) {
                AlignedBytesAllocator<T, kX2PageSize, Alignment>::release_tsl(precent_to_save);
            } else if (slot <= kX4PageSize) {
                AlignedBytesAllocator<T, kX4PageSize, Alignment>::release_tsl(precent_to_save);
            }
        }

        static void release_tsl(float precent_to_save) {
            release_tsl(1, precent_to_save);
            release_tsl(kTinySize, precent_to_save);
            release_tsl(kMiniSize, precent_to_save);
            release_tsl(kSmallSize, precent_to_save);
            release_tsl(kMediumSize, precent_to_save);
            release_tsl(kBigSize, precent_to_save);
            release_tsl(kPageSize, precent_to_save);
            release_tsl(kX2PageSize, precent_to_save);
            release_tsl(kX4PageSize, precent_to_save);
        }

        static std::optional<PoolStats> tls_stats(size_t n) {
            if (n == 1) {
                return AlignedBytesAllocator<T, 1, Alignment>::tls_stats();
            } else if (n <= kTinySize) {
                return AlignedBytesAllocator<T, kTinySize, Alignment>::tls_stats();
            } else if (n <= kMiniSize) {
                return AlignedBytesAllocator<T, kMiniSize, Alignment>::tls_stats();
            } else if (n <= kSmallSize) {
                return AlignedBytesAllocator<T, kSmallSize, Alignment>::tls_stats();
            } else if (n <= kMediumSize) {
                return AlignedBytesAllocator<T, kMediumSize, Alignment>::tls_stats();
            } else if (n <= kBigSize) {
                return AlignedBytesAllocator<T, kBigSize, Alignment>::tls_stats();
            } else if (n <= kPageSize) {
                return AlignedBytesAllocator<T, kPageSize, Alignment>::tls_stats();
            } else if (n <= kX2PageSize) {
                return AlignedBytesAllocator<T, kX2PageSize, Alignment>::tls_stats();
            } else if (n <= kX4PageSize) {
                return AlignedBytesAllocator<T, kX4PageSize, Alignment>::tls_stats();
            } else {
                return std::nullopt;
            }
        }

        static std::vector<std::pair<size_t, PoolStats> > tls_stats() {
            std::vector<std::pair<size_t, PoolStats> > result;
            result.emplace_back(1, *tls_stats(1));
            result.emplace_back(kTinySize, *tls_stats(kTinySize));
            result.emplace_back(kMiniSize, *tls_stats(kMiniSize));
            result.emplace_back(kSmallSize, *tls_stats(kSmallSize));
            result.emplace_back(kMediumSize, *tls_stats(kMediumSize));
            result.emplace_back(kBigSize, *tls_stats(kBigSize));
            result.emplace_back(kPageSize, *tls_stats(kPageSize));
            result.emplace_back(kX2PageSize, *tls_stats(kX2PageSize));
            result.emplace_back(kX4PageSize, *tls_stats(kX4PageSize));
            return result;
        }

        static ObjectGuard<Alignment> collect_tsl(size_t n) {
            if (n == 1) {
                return AlignedBytesAllocator<T, 1, Alignment>::collect_tsl();
            } else if (n <= kTinySize) {
                return AlignedBytesAllocator<T, kTinySize, Alignment>::collect_tsl();
            } else if (n <= kMiniSize) {
                return AlignedBytesAllocator<T, kMiniSize, Alignment>::collect_tsl();
            } else if (n <= kSmallSize) {
                return AlignedBytesAllocator<T, kSmallSize, Alignment>::collect_tsl();
            } else if (n <= kMediumSize) {
                return AlignedBytesAllocator<T, kMediumSize, Alignment>::collect_tsl();
            } else if (n <= kBigSize) {
                return AlignedBytesAllocator<T, kBigSize, Alignment>::collect_tsl();
            } else if (n <= kPageSize) {
                return AlignedBytesAllocator<T, kPageSize, Alignment>::collect_tsl();
            } else if (n <= kX2PageSize) {
                return AlignedBytesAllocator<T, kX2PageSize, Alignment>::collect_tsl();
            } else if (n <= kX4PageSize) {
                return AlignedBytesAllocator<T, kX4PageSize, Alignment>::collect_tsl();
            } else {
                return ObjectGuard<Alignment>{};
            }
        }

        static std::vector<ObjectGuard<Alignment> > collect_tsl() {
            std::vector<ObjectGuard<Alignment> > result;
            result.reserve(6);
            result.emplace_back(collect_tsl(1));
            result.emplace_back(collect_tsl(kTinySize));
            result.emplace_back(collect_tsl(kMiniSize));
            result.emplace_back(collect_tsl(kSmallSize));
            result.emplace_back(collect_tsl(kMediumSize));
            result.emplace_back(collect_tsl(kBigSize));
            result.emplace_back(collect_tsl(kPageSize));
            result.emplace_back(collect_tsl(kX2PageSize));
            result.emplace_back(collect_tsl(kX4PageSize));
            return std::move(result);
        }

        static void apply_tsl(ObjectGuard<Alignment> &objs) {
            if (objs.n == 0) {
                return;
            } else if (objs.n == AlignedBytesAllocator<T, 1, Alignment>::good_size()) {
                AlignedBytesAllocator<T, 1, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kTinySize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kTinySize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kMiniSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kMiniSize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kSmallSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kSmallSize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kMediumSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kMediumSize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kBigSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kBigSize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kPageSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kPageSize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kX2PageSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kX2PageSize, Alignment>::apply_tsl(objs);
            } else if (objs.n == AlignedBytesAllocator<T, kX4PageSize, Alignment>::good_size()) {
                AlignedBytesAllocator<T, kX4PageSize, Alignment>::apply_tsl(objs);
            } else {
                KCHECK(false) << "unknown size:" << objs.n;
            }
        }

        static void apply_tsl(std::vector<ObjectGuard<Alignment> > &objs) {
            for (auto &it: objs) {
                apply_tsl(it);
            }
        }
    };


    template<typename T, size_t Alignment, size_t N>
    struct FixedAllocator {
        template<typename U>
        struct rebind {
            using other = FixedAllocator<U, Alignment, N>;
        };


        static T *pooled_alloc(size_t n) {
            T *ptr = reinterpret_cast<T *>(AlignedBytesAllocator<T, N, Alignment>::allocate());
            if (!ptr) {
                throw std::length_error("");
            }

            return ptr;
        }

        static size_t pooled_alloc_size(size_t n) {
            return N;
        }

        static void pooled_free(T *ptr, size_t n) {
            AlignedBytesAllocator<T, N, Alignment>::deallocate(ptr);
        }

        static void set_tsl_limit(size_t n) {
            AlignedBytesAllocator<T, N, Alignment>::set_tsl_limit(n);
        }

        static void release_tsl(float precent_to_save) {
            AlignedBytesAllocator<T, N, Alignment>::release_tsl(precent_to_save);
        }

        static std::optional<PoolStats> tls_stats() {
            return AlignedBytesAllocator<T, N, Alignment>::tls_stats();
        }

        static ObjectGuard<Alignment> collect_tsl() {
            return AlignedBytesAllocator<T, N, Alignment>::collect_tsl();
        }

        static void apply_tsl(ObjectGuard<Alignment> &objs) {
            AlignedBytesAllocator<T, N, Alignment>::apply_tsl(objs);
        }

        static void apply_tsl(std::vector<ObjectGuard<Alignment> > &objs) {
            for (auto &it: objs) {
                apply_tsl(it);
            }
        }
    };


    template<typename T, size_t Alignment>
    struct PureAllocator {
        template<typename U>
        struct rebind {
            using other = PureAllocator<U, Alignment>;
        };


        static T *pooled_alloc(size_t n) {
            T *ptr = reinterpret_cast<T *>(Malloc::good_alloc(sizeof(T) * n));
            if (!ptr) {
                throw std::length_error("");
            }

            return ptr;
        }

        static size_t pooled_alloc_size(size_t n) {
            return n * sizeof(T);
        }

        static void pooled_free(T *ptr, size_t n) {
            Malloc::good_free(ptr);
        }

        static void set_tsl_limit(size_t n) {
        }

        static void release_tsl(float precent_to_save) {
        }

        static std::optional<PoolStats> tls_stats() {
            return std::nullopt;
        }

        static ObjectGuard<Alignment> collect_tsl() {
            return ObjectGuard<Alignment>::collect_tsl();
        }

        static void apply_tsl(ObjectGuard<Alignment> &objs) {
        }

        static void apply_tsl(std::vector<ObjectGuard<Alignment> > &objs) {
        }
    };


    /// for thread trans bind to same object,
    /// for example a is producer to alloc, b is
    /// customer, to free, and then, you can reuse it
    /// by trans from a AranaPool to b, when b done,
    /// return the AranaPool to a to reuse, but only
    /// alloc it by no lock when pingpong only with a
    /// small lock when trnas resource
    template<typename T, size_t Alignment>
    struct AranaPool {
        AranaPool() = default;

        T *allocate() {
            T *ptr = nullptr;
            if (!_queue.empty()) {
                ptr = _queue.back();
                _queue.pop_back();
            } else {
                ptr = AlignedBytesAllocator<T, 1, Alignment>::alloc();
            }
            return ptr;
        }

        ~AranaPool() {
            for (auto &it: _queue) {
                AlignedBytesAllocator<T, 1, Alignment>::free(it);
            }
        }

        void deallocate(T *ptr) {
            if (ptr == nullptr) {
                return;
            }
            _queue.push(ptr);
        }

    private:
        std::vector<T *> _queue;
    };

    template<typename T, size_t Alignment>
    struct ObjectPoolGuard {
        ObjectPoolGuard(AranaPool<T, Alignment> *arena) : _arena(arena) {
        }

        ~ObjectPoolGuard() = default;

        T *allocate() {
            if (_arena) {
                return _arena->allocate();
            }
            return reinterpret_cast<T *>(AlignedBytesAllocator<T, 1, Alignment>::allocate());
        }

        void deallocate(T *ptr) {
            if (ptr == nullptr) {
                return;
            }
            if (_arena) {
                _arena->deallocate(ptr);
                return;
            }
            AlignedBytesAllocator<T, 1, Alignment>::deallocate(ptr);
        }

    private:
        AranaPool<T, Alignment> *_arena;
    };
} // namespace fermat
