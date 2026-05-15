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

#include <fermat/memory/object_pool.h>
#include <memory>
#include <turbo/log/logging.h>


namespace fermat {
    template<typename T, size_t Alignment, typename Operator = TieredAllocator<T, Alignment> >
    struct BasicAllocator {
    public:
        using operator_type = Operator;

    public:
        constexpr BasicAllocator() noexcept = default;

        ~BasicAllocator() noexcept = default;

        constexpr BasicAllocator(const BasicAllocator &) noexcept = default;

        constexpr BasicAllocator &operator=(const BasicAllocator &) noexcept = default;

        constexpr BasicAllocator(BasicAllocator &&) noexcept = default;

        constexpr BasicAllocator &operator=(BasicAllocator &&) noexcept = default;


        T *allocate(size_t *n) {
            return operator_type::pooled_alloc(n);
        }

        void deallocate(T *ptr, size_t n) {
            operator_type::pooled_free(ptr, n);
        }


        [[nodiscard]] size_t good_size(size_t n) const {
            return operator_type::pooled_alloc_size(n);
        }


        static std::vector<ObjectGuard<Alignment> > collect_arena() {
            return operator_type::collect_tsl();
        }

        static void apply_arena(std::vector<ObjectGuard<Alignment> > &rhs) {
            operator_type::apply_tsl(rhs);
        }

        [[nodiscard]] const char *name() const noexcept {
            return "default";
        }

        bool operator==(const BasicAllocator &) const noexcept { return true; }
        bool operator!=(const BasicAllocator &) const noexcept { return false; }

        static BasicAllocator get_default_allocator() {
            return BasicAllocator{};
        }
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
            AlignedMalloc<Alignment>::good_free(p);
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
