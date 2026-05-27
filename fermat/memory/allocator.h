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


    template<size_t Numerator, size_t Denominator>
    struct TimesPolicy {
        static_assert(Denominator > 0, "Denominator must be greater than zero");
        static constexpr float Delta = float(Numerator) / float(Denominator);
        size_t get_new_size(size_t current_size, size_t added, size_t align) noexcept {
            return static_cast<size_t>((current_size + added) * Delta);
        }
    };

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
            return operator_type::pooled_alloc(*n);
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
    template<typename T, size_t Alignment = kDefaultAlignedSize, typename GrowPolicy = TimesPolicy<2,1>, typename Operator = TieredAllocator<T, Alignment>>
    class AlignedAllocator : GrowPolicy {
    private:
        static_assert(Malloc::is_valid_alignment<Alignment>(), "alignment not valid");
    public:
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using policy_type = GrowPolicy;
        using operator_type = Operator;

        /// @brief Tell STL containers that all instances of this allocator are interchangeable.
        /// This enables optimizations like O(1) container swap and move operations.
        using is_always_equal = std::true_type;

        static constexpr size_t kAlignment = Alignment;

        [[nodiscard]] size_t alignment() const noexcept {
            return kAlignment;
        }

        [[nodiscard]] size_t good_size(size_t n) const noexcept {
            return operator_type::pooled_alloc_size(n);
        }

        [[nodiscard]] bool is_aligned_size(size_t n) const noexcept {
            return Malloc::is_aligned_size(Alignment, n);
        }

        [[nodiscard]] bool is_aligned(const T *ptr) const noexcept {
            return Malloc::is_aligned(ptr, Alignment);
        }

        size_t get_new_size(size_t current_size, size_t added) noexcept {
            return GrowPolicy::get_new_size(current_size, added, Alignment);
        }

        /// @brief Rebind convenience for STL containers.
        template<typename U>
        struct rebind {
            using other = AlignedAllocator<U, Alignment, GrowPolicy,typename Operator::template rebind<U>::other>;
        };

        AlignedAllocator() noexcept = default;

        template<typename U>
        constexpr AlignedAllocator(const AlignedAllocator<U, Alignment, GrowPolicy,Operator> &) noexcept {
        }

        /// @brief Returns the actual address of x.
        pointer address(reference x) const noexcept { return std::addressof(x); }
        const_pointer address(const_reference x) const noexcept { return std::addressof(x); }

        /// @brief Allocate memory for n elements of type T.
        [[nodiscard]] T *allocate(size_t n, const void *hint = nullptr) {
            return operator_type::pooled_alloc(n);
        }

        /// @brief Deallocate memory previously allocated with allocate.
        void deallocate(T *p, size_t n) noexcept {
            operator_type::pooled_free(p, n);
        }

        /// @brief Maximum number of elements that can be allocated.
       [[nodiscard]] size_type max_size() const noexcept {
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
