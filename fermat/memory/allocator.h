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
#include <memory>

namespace fermat {
    /// @brief STL compatible allocator using mimalloc with default alignment.
    /// @tparam T Element type.
    template<typename T>
    class Allocator {
    public:
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        /// @brief Tell STL containers that all instances of this allocator are interchangeable.
        using is_always_equal = std::true_type;

        /// @brief Rebind convenience for STL containers.
        template<typename U>
        struct rebind {
            using other = Allocator<U>;
        };

        Allocator() noexcept = default;

        template<typename U>
        constexpr Allocator(const Allocator<U> &) noexcept {
        }

        /// @brief Returns the actual address of x.
        pointer address(reference x) const noexcept { return std::addressof(x); }
        const_pointer address(const_reference x) const noexcept { return std::addressof(x); }

        /// @brief Allocate memory for n elements of type T.
        [[nodiscard]] T *allocate(size_t n, const void *hint = nullptr) {
            (void) hint;
            if (n == 0) return nullptr;
            if (n > max_size()) {
                throw std::bad_array_new_length();
            }

            size_t total_bytes = n * sizeof(T);
            /// Request a 'good size' from mimalloc to optimize bucket usage.
            /// This will update total_bytes to the actual allocated physical size.
            void *ptr = Malloc::good_alloc(&total_bytes);

            if (!ptr) throw std::bad_alloc();
            return static_cast<T *>(ptr);
        }

        /// @brief Deallocate memory previously allocated with allocate.
        void deallocate(T *p, size_t n) noexcept {
            if (p == nullptr) return;

            /// We must pass the exact same size used in mi_malloc to mi_free_size.
            size_t total_bytes = n * sizeof(T);
            size_t rn = Malloc::good_alloc_size(total_bytes);
            Malloc::good_free(p, rn);
        }

        /// @brief Maximum number of elements that can be allocated.
        size_type max_size() const noexcept {
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        /// @brief Construct an object in allocated storage.
        template<typename U, typename... Args>
        static void construct(U *p, Args &&... args) {
            ::new(static_cast<void *>(p)) U(std::forward<Args>(args)...);
        }

        /// @brief Destroy an object.
        template<typename U>
        void destroy(U *p) {
            p->~U();
        }

        bool operator==(const Allocator &) const noexcept { return true; }
        bool operator!=(const Allocator &) const noexcept { return false; }
    };

    /// @brief STL compatible allocator using mimalloc with explicit alignment.
    /// @tparam T Element type.
    /// @tparam Alignment Must be a power of 2, defaults to kDefaultAlignedSize (64).
    template<typename T, size_t Alignment = kDefaultAlignedSize>
    class AlignedAllocator {
    public:
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        /// @brief Tell STL containers that all instances of this allocator are interchangeable.
        /// This enables optimizations like O(1) container swap and move operations.
        using is_always_equal = std::true_type;

        static constexpr size_t kAlignment = Alignment;

        [[nodiscard]] size_t alignment() const noexcept {
            return kAlignment;
        }

        [[nodiscard]] bool is_aligned_size(size_t n) const noexcept {
            return AlignedMalloc<Alignment>::is_aligned_size(n);
        }

        [[nodiscard]] bool is_aligned(const T *ptr) const noexcept {
            return AlignedMalloc<Alignment>::is_aligned(ptr);
        }

        /// @brief Rebind convenience for STL containers.
        template<typename U>
        struct rebind {
            using other = AlignedAllocator<U, Alignment>;
        };

        AlignedAllocator() noexcept = default;

        template<typename U>
        constexpr AlignedAllocator(const AlignedAllocator<U, Alignment> &) noexcept {
        }

        /// @brief Returns the actual address of x.
        pointer address(reference x) const noexcept { return std::addressof(x); }
        const_pointer address(const_reference x) const noexcept { return std::addressof(x); }

        /// @brief Allocate memory for n elements of type T.
        [[nodiscard]] T *allocate(size_t n, const void *hint = nullptr) {
            (void) hint; // hint is not used in this implementation
            if (n == 0) return nullptr;
            if (n > max_size()) {
                throw std::bad_array_new_length();
            }

            size_t total_bytes = n * sizeof(T);
            /// Request a 'good size' from mimalloc to optimize bucket usage.
            void *ptr = AlignedMalloc<Alignment>::good_alloc(&total_bytes);

            if (!ptr) throw std::bad_alloc();
            return static_cast<T *>(ptr);
        }

        /// @brief Deallocate memory previously allocated with allocate.
        void deallocate(T *p, size_t n) noexcept {
            if (p == nullptr) return;

            /// We must pass the exact same size used in mi_aligned_alloc to mi_free_size_aligned.
            size_t total_bytes = n * sizeof(T);
            size_t rn = AlignedMalloc<Alignment>::good_alloc_size(total_bytes);
            AlignedMalloc<Alignment>::good_free(p, rn);
        }

        /// @brief Maximum number of elements that can be allocated.
        size_type max_size() const noexcept {
            return std::numeric_limits<size_type>::max() / sizeof(T);
        }

        /// @brief Construct an object of type T in allocated storage.
        template<typename U, typename... Args>
        void construct(U *p, Args &&... args) {
            ::new(static_cast<void *>(p)) U(std::forward<Args>(args)...);
        }

        /// @brief Destroy an object of type T.
        template<typename U>
        void destroy(U *p) {
            p->~U();
        }

        bool operator==(const AlignedAllocator &) const noexcept { return true; }
        bool operator!=(const AlignedAllocator &) const noexcept { return false; }
    };
} // namespace fermat
