/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// String type.

#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <turbo/strings/str_format.h>
#include <fermat/memory/object_pool.h>

namespace fermat {
    namespace string_internal {
        template<class InIt, class OutIt>
        inline std::pair<InIt, OutIt> copy_n(
            InIt b, typename std::iterator_traits<InIt>::difference_type n, OutIt d) {
            for (; n != 0; --n, ++b, ++d) {
                *d = *b;
            }
            return std::make_pair(b, d);
        }

        template<class Pod, class T>
        inline void podFill(Pod *b, Pod *e, T c) {
            assert(b && e && b <= e);
            constexpr auto kUseMemset = sizeof(T) == 1;
            if constexpr (kUseMemset) {
                memset(b, c, size_t(e - b));
            } else {
                auto const ee = b + ((e - b) & ~7u);
                for (; b != ee; b += 8) {
                    b[0] = c;
                    b[1] = c;
                    b[2] = c;
                    b[3] = c;
                    b[4] = c;
                    b[5] = c;
                    b[6] = c;
                    b[7] = c;
                }
                // Leftovers
                for (; b != e; ++b) {
                    *b = c;
                }
            }
        }

        /*
         * Lightly structured memcpy, simplifies copying PODs and introduces
         * some asserts. Unfortunately using this function may cause
         * measurable overhead (presumably because it adjusts from a begin/end
         * convention to a pointer/size convention, so it does some extra
         * arithmetic even though the caller might have done the inverse
         * adaptation outside).
         */
        template<class Pod>
        inline void podCopy(const Pod *b, const Pod *e, Pod *d) {
            assert(b != nullptr);
            assert(e != nullptr);
            assert(d != nullptr);
            assert(e >= b);
            assert(d >= e || d + (e - b) <= b);
            memcpy(d, b, (e - b) * sizeof(Pod));
        }

        /*
         * Lightly structured memmove, simplifies copying PODs and introduces
         * some asserts
         */
        template<class Pod>
        inline void podMove(const Pod *b, const Pod *e, Pod *d) {
            assert(e >= b);
            memmove(d, b, (e - b) * sizeof(*b));
        }
    } // namespace string_internal

    template<class Char, size_t Alignment = 0>
    class kstring_core {
    public:
        kstring_core() noexcept { reset(); }

        kstring_core(const kstring_core &rhs) {
            assert(&rhs != this);
            copy_rhs(rhs);
        }

        kstring_core &operator=(const kstring_core &rhs) = delete;

        kstring_core(kstring_core &&goner) noexcept {
            // Take goner's guts
            std::swap(data_, goner.data_);
            std::swap(size_, goner.size_);
            std::swap(capacity_, goner.capacity_);
            goner.reset();
        }

        kstring_core(
            const Char *const data,
            const size_t size) {
            init_string(data, size);
        }

        ~kstring_core() noexcept {
            deallocate();
        }

        // swap below doesn't test whether &rhs == this (and instead
        // potentially does extra work) on the premise that the rarity of
        // that situation actually makes the check more expensive than is
        // worth.
        void swap(kstring_core &rhs) noexcept {
            std::swap(data_, rhs.data_);
            std::swap(size_, rhs.size_);
            std::swap(capacity_, rhs.capacity_);
        }

        // In C++11 data() and c_str() are 100% equivalent.
        const Char *data() const { return data_; }

        Char *data() { return data_; }

        const Char *c_str() const {
            if (!data_ || size_ == 0) {
                return &kZero;
            }
            return data_;
        }

        void shrink(const size_t delta) {
            if (delta >= size_) {
                size_ = 0;
                return;
            }
            size_ -= delta;
            data_[size_] = '\0';
        }

        TURBO_NOINLINE
        void reserve(size_t minCapacity) {
            reserve_string(minCapacity);
        }

        Char *expandNoinit(size_t delta) {
            reserve_string((delta + size_));
            auto ptr = &data_[size_];
            size_ += delta;
            data_[size_] = '\0';
            return ptr;
        }

        void push_back(Char c) {
            *expandNoinit(1) = c;
        }

        constexpr bool is_aligned() const {
            return Alignment != 0;
        }

        constexpr size_t alignment() const {
            return Alignment;
        }

        size_t size() const {
            return size_;
        }

        size_t capacity() const {
            return capacity_;
        }

    private:
        Char *c_str() {
            return data_;
        }

        void reset() {
            data_ = nullptr;
            capacity_ = 0;
            size_ = 0;
        }


        void reallocate(
            size_t *newCapacity) {
            size_t capacityBytes;
            if (!checked_add(&capacityBytes, *newCapacity, size_t(1))) {
                throw std::length_error("");
            }
            if (!checked_muladd(
                &capacityBytes, capacityBytes, sizeof(Char), 0ul)) {
                throw std::length_error("");
            }

            Char *ptr = nullptr;
            auto n = *newCapacity + 1;
            if constexpr (Alignment == 0) {
                ptr = reinterpret_cast<Char *>(Malloc::good_alloc(&n));
            } else {
                ptr = reinterpret_cast<Char *>(AlignedMalloc<Alignment>::good_alloc(&n));
            }
            std::memcpy(ptr, data_, (size_ + 1) * sizeof(Char));
            *newCapacity = (capacityBytes) / sizeof(Char) - 1;
        }


        static constexpr size_t kTinySize = 64;
        static constexpr size_t kSmallSize = 256;
        static constexpr size_t kMediumSize = 512;
        static constexpr size_t kBigSize = 1024;

        size_t around_capacity(size_t n) {
            if (n <= 64) {
                return kTinySize;
            } else if (n <= kSmallSize) {
                return kSmallSize;
            } else if (n <= kMediumSize) {
                return kMediumSize;
            } else if (n <= kBigSize) {
                return kBigSize;
            } else {
                return mi_malloc_good_size(n);
            }
        }

        Char *alloc(size_t *n) {
            Char *ptr = nullptr;
            if (*n <= 64) {
                ptr = reinterpret_cast<Char *>(AlignedBytesAllocator<Char, kTinySize, Alignment>::allocate());
                *n = kTinySize;
            } else if (*n <= kSmallSize) {
                ptr = reinterpret_cast<Char *>(AlignedBytesAllocator<Char, kSmallSize, Alignment>::allocate());
                *n = kSmallSize;
            } else if (*n <= kMediumSize) {
                ptr = reinterpret_cast<Char *>(AlignedBytesAllocator<Char, kMediumSize, Alignment>::allocate());
                *n = kMediumSize;
            } else if (*n <= kBigSize) {
                ptr = reinterpret_cast<Char *>(AlignedBytesAllocator<Char, kBigSize, Alignment>::allocate());
                *n = kBigSize;
            } else {
                auto bytes = *n * sizeof(Char);
                if constexpr (Alignment > 0) {
                    ptr = static_cast<Char *>(AlignedMalloc<Alignment>::good_alloc(&bytes));
                } else {
                    ptr = static_cast<Char *>(Malloc::good_alloc(&bytes));
                }
                *n = bytes / sizeof(Char);
            }
            if (!ptr) {
                throw std::length_error("");
            }
            return ptr;
        }

        void deallocate() {
            if (data_ && data_ != &kZero) {
                auto n = capacity_ + 1;
                switch (n) {
                    case 0:
                        break;
                    case kTinySize: {
                        AlignedBytesAllocator<Char, kTinySize, Alignment>::deallocate(data_);
                        break;
                    }
                    case kSmallSize: {
                        AlignedBytesAllocator<Char, kSmallSize, Alignment>::deallocate(data_);
                        break;
                    }
                    case kMediumSize: {
                        AlignedBytesAllocator<Char, kMediumSize, Alignment>::deallocate(data_);
                        break;
                    }
                    case kBigSize: {
                        AlignedBytesAllocator<Char, kBigSize, Alignment>::deallocate(data_);
                        break;
                    }
                    default:
                        if constexpr (Alignment > 0) {
                            AlignedMalloc<Alignment>::good_free(data_);
                        } else {
                            Malloc::good_free(data_);
                        }
                        break;
                }
            }

            data_ = nullptr;
            capacity_ = 0;
            size_ = 0;
        }

        static constexpr Char kZero = {'\0'};
        Char *data_{nullptr};
        size_t size_{0};
        size_t capacity_{0};

        constexpr static bool kIsSanitize = false;

        void copy_rhs(const kstring_core &);

        void init_string(const Char *data, size_t size);

        void reserve_string(size_t minCapacity);

        Char *mutable_data();
    };

    template<class Char, size_t Alignment>
    inline void kstring_core<Char, Alignment>::copy_rhs(const kstring_core &rhs) {
        deallocate();
        if (rhs.size()) {
            capacity_ = rhs.size() + 1;
            data_ = alloc(&capacity_);
            std::memcpy(data_, rhs.data_, sizeof(Char) * (rhs.size_ + 1));
            size_ = rhs.size_;
        }
    }

    template<class Char, size_t Alignment>
    TURBO_NOINLINE void kstring_core<Char, Alignment>::init_string(
        const Char *const data, const size_t size) {
        if (!data || !size) {
            return;
        }
        // Small strings are allocated normally. Don't forget to
        // allocate one extra Char for the terminating null.
        capacity_ = size + 1;
        size_ = size;
        data_ = alloc(&capacity_);
        std::memcpy(data_, data, size * sizeof(Char));
        --capacity_;
        data_[size_] = '\0';
    }

    template<class Char, size_t Alignment>
    inline Char *kstring_core<Char, Alignment>::mutable_data() {
        return data_;
    }


    template<class Char, size_t Alignment>
    TURBO_NOINLINE void kstring_core<Char, Alignment>::reserve_string(
        const size_t minCapacity) {
        size_t ns;
        if (!checked_add(&ns, minCapacity, 1ul)) {
            throw std::length_error("");
        }
        size_t bytres;
        if (!checked_muladd(
            &bytres, ns, sizeof(Char), 0UL)) {
            throw std::length_error("");
        }

        if (ns <= capacity_) {
            return;
        }
        auto ptr = alloc(&ns);
        if (size_ > 0) {
            std::memcpy(ptr, data_, sizeof(Char) * (size_ + 1));
        }
        auto n = size_;
        deallocate();
        capacity_ = ns - 1;
        data_ = ptr;
        size_ = n;
    }

    /**
     * Dummy KString core that uses an actual std::string. This doesn't
     * make any sense - it's just for testing purposes.
     */
    template<class Char, size_t Alignment>
    class dummy_kstring_core {
    public:
        dummy_kstring_core() {
        }

        dummy_kstring_core(const dummy_kstring_core &another)
            : backend_(another.backend_) {
        }

        dummy_kstring_core(const Char *s, size_t n) : backend_(s, n) {
        }

        void swap(dummy_kstring_core &rhs) { backend_.swap(rhs.backend_); }
        const Char *data() const { return backend_.data(); }
        Char *mutable_data() { return const_cast<Char *>(backend_.data()); }

        void shrink(size_t delta) {
            assert(delta <= size());
            backend_.resize(size() - delta);
        }

        Char *expandNoinit(size_t delta) {
            auto const sz = size();
            backend_.resize(size() + delta);
            return backend_.data() + sz;
        }

        void push_back(Char c) { backend_.push_back(c); }
        size_t size() const { return backend_.size(); }
        size_t capacity() const { return backend_.capacity(); }
        void reserve(size_t minCapacity) { backend_.reserve(minCapacity); }

    private:
        std::basic_string<Char> backend_;
    };

    /**
     * This is the basic_string replacement. For conformity,
     * BasicString takes the same template parameters, plus the last
     * one which is the core.
     */
    template<
        typename E,
        class T = std::char_traits<E>,
        class A = std::allocator<E>,
        class Storage = kstring_core<E> >
    class BasicString {
        static_assert(
            std::is_same<A, std::allocator<E> >::value,
            "KString ignores custom allocators");

        template<typename Ex, typename... Args>
        TURBO_FORCE_INLINE static void enforce(bool condition, Args &&... args) {
            if (!condition) {
                throw Ex(static_cast<Args &&>(args)...);
            }
        }

        bool isSane() const {
            return begin() <= end() && empty() == (size() == 0) &&
                   empty() == (begin() == end()) && size() <= max_size() &&
                   capacity() <= max_size() && size() <= capacity() &&
                   begin()[size()] == '\0';
        }

        struct Invariant {
            Invariant &operator=(const Invariant &) = delete;

            explicit Invariant(const BasicString &s) noexcept : s_(s) {
                assert(s_.isSane());
            }

            ~Invariant() noexcept { assert(s_.isSane()); }

        private:
            const BasicString &s_;
        };

    public:
        // types
        typedef T traits_type;
        typedef typename traits_type::char_type value_type;
        typedef A allocator_type;
        typedef typename std::allocator_traits<A>::size_type size_type;
        typedef typename std::allocator_traits<A>::difference_type difference_type;

        typedef typename std::allocator_traits<A>::value_type &reference;
        typedef typename std::allocator_traits<A>::value_type const &const_reference;
        typedef typename std::allocator_traits<A>::pointer pointer;
        typedef typename std::allocator_traits<A>::const_pointer const_pointer;

        typedef E *iterator;
        typedef const E *const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        static constexpr size_type npos = size_type(-1);
        typedef std::true_type IsRelocatable;

    private:
        using string_view_type = std::basic_string_view<value_type, traits_type>;

        template<typename StringViewLike>
        static inline constexpr bool is_string_view_like_v =
                std::is_convertible_v<StringViewLike const &, string_view_type> &&
                !std::is_convertible_v<StringViewLike const &, const_pointer>;

        template<typename StringViewLike, typename Dummy>
        using if_is_string_view_like_t =
        std::enable_if_t<is_string_view_like_v<StringViewLike>, Dummy>;

        static void procrustes(size_type &n, size_type nmax) {
            if (n > nmax) {
                n = nmax;
            }
        }

        static size_type traitsLength(const value_type *s);

        struct string_view_ctor {
        };

        TURBO_NOINLINE

        BasicString(
            string_view_type view, const A &, string_view_ctor)
            : store_(view.data(), view.size()) {
        }

    public:
        // C++11 21.4.2 construct/copy/destroy

        // Note: while the following two constructors can be (and previously were)
        // collapsed into one constructor written this way:
        //
        //   explicit BasicString(const A& a = A()) noexcept { }
        //
        // This can cause Clang (at least version 3.7) to fail with the error:
        //   "chosen constructor is explicit in copy-initialization ...
        //   in implicit initialization of field '(x)' with omitted initializer"
        //
        // if used in a struct which is default-initialized.  Hence the split into
        // these two separate constructors.

        BasicString() noexcept : BasicString(A()) {
        }

        /* implicit */
        BasicString(std::nullptr_t) = delete;

        explicit BasicString(const A &) noexcept {
        }

        BasicString(const BasicString &str) : store_(str.store_) {
        }

        // Move constructor
        BasicString(BasicString &&goner) noexcept
            : store_(std::move(goner.store_)) {
        }

        // This is defined for compatibility with std::string
        template<typename A2>
        /* implicit */ BasicString(const std::basic_string<E, T, A2> &str)
            : store_(str.data(), str.size()) {
        }

        BasicString(
            const BasicString &str,
            size_type pos,
            size_type n = npos,
            const A & /* a */ = A()) {
            assign(str, pos, n);
        }

        TURBO_NOINLINE
        /* implicit */BasicString(const value_type *s, const A & /*a*/ = A())
            : store_(s, traitsLength(s)) {
        }

        TURBO_NOINLINE
        BasicString(const value_type *s, size_type n, const A & /*a*/ = A())
            : store_(s, n) {
        }

        TURBO_NOINLINE
        BasicString(size_type n, value_type c, const A & /*a*/ = A()) {
            auto const pData = store_.expandNoinit(n);
            string_internal::podFill(pData, pData + n, c);
        }

        template<class InIt>
        TURBO_NOINLINE

        BasicString(
            InIt begin,
            InIt end,
            typename std::enable_if<
                !std::is_same<InIt, value_type *>::value,
                const A>::type & = A()) {
            assign(begin, end);
        }

        // Specialization for const char*, const char*
        TURBO_NOINLINE
        BasicString(const value_type *b, const value_type *e, const A & /*a*/ = A())
            : store_(b, size_type(e - b)) {
        }


        // Construction from initialization list
        TURBO_NOINLINE
        BasicString(std::initializer_list<value_type> il) {
            assign(il.begin(), il.end());
        }

        template<
            typename StringViewLike,
            if_is_string_view_like_t<StringViewLike, int> = 0>
        explicit BasicString(const StringViewLike &view, const A &a = A())
            : BasicString(string_view_type(view), a, string_view_ctor{}) {
        }

        template<
            typename StringViewLike,
            if_is_string_view_like_t<StringViewLike, int> = 0>
        BasicString(
            const StringViewLike &view, size_type pos, size_type n, const A &a = A())
            : BasicString(
                string_view_type(view).substr(pos, n), a, string_view_ctor{}) {
        }

        ~BasicString() noexcept {
        }

        BasicString &operator=(const BasicString &lhs);

        // Move assignment
        BasicString &operator=(BasicString &&goner) noexcept;

        // Compatibility with std::string
        template<typename A2>
        BasicString &operator=(const std::basic_string<E, T, A2> &rhs) {
            return assign(rhs.data(), rhs.size());
        }

        // Compatibility with std::string
        std::basic_string<E, T, A> toStdString() const {
            return std::basic_string<E, T, A>(data(), size());
        }

        BasicString &operator=(std::nullptr_t) = delete;

        BasicString &operator=(const value_type *s) { return assign(s); }

        BasicString &operator=(value_type c);

        // This actually goes directly against the C++ spec, but the
        // value_type overload is dangerous, so we're explicitly deleting
        // any overloads of operator= that could implicitly convert to
        // value_type.
        // Note that we do need to explicitly specify the template types because
        // otherwise MSVC 2017 will aggressively pre-resolve value_type to
        // traits_type::char_type, which won't compare as equal when determining
        // which overload the implementation is referring to.
        template<typename TP>
        typename std::enable_if<
            std::is_convertible<
                TP,
                typename BasicString<E, T, A, Storage>::value_type>::value &&
            !std::is_same<
                typename std::decay<TP>::type,
                typename BasicString<E, T, A, Storage>::value_type>::value,
            BasicString<E, T, A, Storage> &>::type
        operator=(TP c) = delete;

        BasicString &operator=(std::initializer_list<value_type> il) {
            return assign(il.begin(), il.end());
        }

        operator string_view_type() const noexcept { return {data(), size()}; }

        // C++11 21.4.3 iterators:
        iterator begin() { return store_.mutable_data(); }

        const_iterator begin() const { return store_.data(); }

        const_iterator cbegin() const { return begin(); }

        iterator end() { return store_.mutable_data() + store_.size(); }

        const_iterator end() const { return store_.data() + store_.size(); }

        const_iterator cend() const { return end(); }

        reverse_iterator rbegin() { return reverse_iterator(end()); }

        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator crbegin() const { return rbegin(); }

        reverse_iterator rend() { return reverse_iterator(begin()); }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crend() const { return rend(); }

        // Added by C++11
        // C++11 21.4.5, element access:
        const value_type &front() const { return *begin(); }

        const value_type &back() const {
            assert(!empty());
            // Should be begin()[size() - 1], but that branches twice
            return *(end() - 1);
        }

        value_type &front() { return *begin(); }

        value_type &back() {
            assert(!empty());
            // Should be begin()[size() - 1], but that branches twice
            return *(end() - 1);
        }

        void pop_back() {
            assert(!empty());
            store_.shrink(1);
        }

        // C++11 21.4.4 capacity:
        size_type size() const { return store_.size(); }

        size_type length() const { return size(); }

        size_type max_size() const { return std::numeric_limits<size_type>::max(); }

        void resize(size_type n, value_type c = value_type());

        size_type capacity() const { return store_.capacity(); }

        // Returns the reference count for this string's underlying data.
        // Returns 1 for Tiny (SSO) or Small strings which are not reference
        // counted. For Large strings, returns the actual reference count.
        // Useful for memory accounting with fair-share division.
        size_t use_count() const { return store_.useCount(); }

        // Returns true if the string uses RefCounted (Large) storage mode.
        // Large strings have a RefCounted header (sizeof(size_t) bytes) before
        // the data, which must be accounted for in memory calculations.
        bool is_counted() const { return store_.isCounted(); }

        void reserve(size_type res_arg = 0) {
            enforce<std::length_error>(res_arg <= max_size(), "");
            store_.reserve(res_arg);
        }

        void shrink_to_fit() {
            // Shrink only if slack memory is sufficiently large
            if (capacity() < size() * 3 / 2) {
                return;
            }
            BasicString(cbegin(), cend()).swap(*this);
        }

        void clear() { resize(0); }

        bool empty() const { return size() == 0; }

        // C++11 21.4.5 element access:
        const_reference operator[](size_type pos) const { return *(begin() + pos); }

        reference operator[](size_type pos) { return *(begin() + pos); }

        const_reference at(size_type n) const {
            enforce<std::out_of_range>(n < size(), "");
            return (*this)[n];
        }

        reference at(size_type n) {
            enforce<std::out_of_range>(n < size(), "");
            return (*this)[n];
        }

        // C++11 21.4.6 modifiers:
        BasicString &operator+=(const BasicString &str) { return append(str); }

        BasicString &operator+=(const value_type *s) { return append(s); }

        BasicString &operator+=(const value_type c) {
            push_back(c);
            return *this;
        }

        BasicString &operator+=(std::initializer_list<value_type> il) {
            append(il);
            return *this;
        }

        template<
            typename StringViewLike,
            if_is_string_view_like_t<StringViewLike, int> = 0>
        BasicString &operator+=(const StringViewLike &like) {
            append(like);
            return *this;
        }

        BasicString &append(const BasicString &str);

        BasicString &append(
            const BasicString &str, const size_type pos, size_type n);

        BasicString &append(const value_type *s, size_type n);

        BasicString &append(const value_type *s) {
            return append(s, traitsLength(s));
        }

        BasicString &append(size_type n, value_type c);

        template<class InputIterator>
        BasicString &append(InputIterator first, InputIterator last) {
            insert(end(), first, last);
            return *this;
        }

        BasicString &append(std::initializer_list<value_type> il) {
            return append(il.begin(), il.end());
        }

        template<
            typename StringViewLike,
            if_is_string_view_like_t<StringViewLike, int> = 0>
        BasicString &append(const StringViewLike &like) {
            string_view_type view = like;
            return append(view.begin(), view.end());
        }

        void push_back(const value_type c) {
            // primitive
            store_.push_back(c);
        }

        BasicString &assign(const BasicString &str) {
            if (&str == this) {
                return *this;
            }
            return assign(str.data(), str.size());
        }

        BasicString &assign(BasicString &&str) {
            return *this = std::move(str);
        }

        BasicString &assign(
            const BasicString &str, const size_type pos, size_type n);

        BasicString &assign(const value_type *s, const size_type n);

        BasicString &assign(const value_type *s) {
            return assign(s, traitsLength(s));
        }

        BasicString &assign(std::initializer_list<value_type> il) {
            return assign(il.begin(), il.end());
        }

        template<class ItOrLength, class ItOrChar>
        BasicString &assign(ItOrLength first_or_n, ItOrChar last_or_c) {
            return replace(begin(), end(), first_or_n, last_or_c);
        }

        BasicString &insert(size_type pos1, const BasicString &str) {
            return insert(pos1, str.data(), str.size());
        }

        BasicString &insert(
            size_type pos1, const BasicString &str, size_type pos2, size_type n) {
            enforce<std::out_of_range>(pos2 <= str.length(), "");
            procrustes(n, str.length() - pos2);
            return insert(pos1, str.data() + pos2, n);
        }

        BasicString &insert(size_type pos, const value_type *s, size_type n) {
            enforce<std::out_of_range>(pos <= length(), "");
            insert(begin() + pos, s, s + n);
            return *this;
        }

        BasicString &insert(size_type pos, const value_type *s) {
            return insert(pos, s, traitsLength(s));
        }

        BasicString &insert(size_type pos, size_type n, value_type c) {
            enforce<std::out_of_range>(pos <= length(), "");
            insert(begin() + pos, n, c);
            return *this;
        }

        iterator insert(const_iterator p, const value_type c) {
            const size_type pos = p - cbegin();
            insert(p, 1, c);
            return begin() + pos;
        }

    private:
        typedef std::basic_istream<value_type, traits_type> istream_type;

        istream_type &getlineImpl(istream_type &is, value_type delim);

    public:
        friend inline istream_type &getline(
            istream_type &is, BasicString &str, value_type delim) {
            return str.getlineImpl(is, delim);
        }

        friend inline istream_type &getline(istream_type &is, BasicString &str) {
            return getline(is, str, '\n');
        }

    private:
        iterator insertImplDiscr(
            const_iterator i, size_type n, value_type c, std::true_type);

        template<class InputIter>
        iterator insertImplDiscr(
            const_iterator i, InputIter b, InputIter e, std::false_type);

        template<class FwdIterator>
        iterator insertImpl(
            const_iterator i,
            FwdIterator s1,
            FwdIterator s2,
            std::forward_iterator_tag);

        template<class InputIterator>
        iterator insertImpl(
            const_iterator i,
            InputIterator b,
            InputIterator e,
            std::input_iterator_tag);

    public:
        template<class ItOrLength, class ItOrChar>
        iterator insert(const_iterator p, ItOrLength first_or_n, ItOrChar last_or_c) {
            using Sel =
                    std::bool_constant<std::numeric_limits<ItOrLength>::is_specialized>;
            return insertImplDiscr(p, first_or_n, last_or_c, Sel());
        }

        iterator insert(const_iterator p, std::initializer_list<value_type> il) {
            return insert(p, il.begin(), il.end());
        }

        BasicString &erase(size_type pos = 0, size_type n = npos) {
            Invariant checker(*this);

            enforce<std::out_of_range>(pos <= length(), "");
            procrustes(n, length() - pos);
            std::copy(begin() + pos + n, end(), begin() + pos);
            resize(length() - n);
            return *this;
        }

        iterator erase(iterator position) {
            const size_type pos(position - begin());
            enforce<std::out_of_range>(pos <= size(), "");
            erase(pos, 1);
            return begin() + pos;
        }

        iterator erase(iterator first, iterator last) {
            const size_type pos(first - begin());
            erase(pos, last - first);
            return begin() + pos;
        }

        // Replaces at most n1 chars of *this, starting with pos1 with the
        // content of str
        BasicString &replace(
            size_type pos1, size_type n1, const BasicString &str) {
            return replace(pos1, n1, str.data(), str.size());
        }

        // Replaces at most n1 chars of *this, starting with pos1,
        // with at most n2 chars of str starting with pos2
        BasicString &replace(
            size_type pos1,
            size_type n1,
            const BasicString &str,
            size_type pos2,
            size_type n2) {
            enforce<std::out_of_range>(pos2 <= str.length(), "");
            return replace(
                pos1, n1, str.data() + pos2, std::min(n2, str.size() - pos2));
        }

        // Replaces at most n1 chars of *this, starting with pos, with chars from s
        BasicString &replace(size_type pos, size_type n1, const value_type *s) {
            return replace(pos, n1, s, traitsLength(s));
        }

        // Replaces at most n1 chars of *this, starting with pos, with n2
        // occurrences of c
        //
        // consolidated with
        //
        // Replaces at most n1 chars of *this, starting with pos, with at
        // most n2 chars of str.  str must have at least n2 chars.
        template<class StrOrLength, class NumOrChar>
        BasicString &replace(
            size_type pos, size_type n1, StrOrLength s_or_n2, NumOrChar n_or_c) {
            Invariant checker(*this);

            enforce<std::out_of_range>(pos <= size(), "");
            procrustes(n1, length() - pos);
            const iterator b = begin() + pos;
            return replace(b, b + n1, s_or_n2, n_or_c);
        }

        BasicString &replace(iterator i1, iterator i2, const BasicString &str) {
            return replace(i1, i2, str.data(), str.length());
        }

        BasicString &replace(iterator i1, iterator i2, const value_type *s) {
            return replace(i1, i2, s, traitsLength(s));
        }

    private:
        BasicString &replaceImplDiscr(
            iterator i1,
            iterator i2,
            const value_type *s,
            size_type n,
            std::integral_constant<int, 2>);

        BasicString &replaceImplDiscr(
            iterator i1,
            iterator i2,
            size_type n2,
            value_type c,
            std::integral_constant<int, 1>);

        template<class InputIter>
        BasicString &replaceImplDiscr(
            iterator i1,
            iterator i2,
            InputIter b,
            InputIter e,
            std::integral_constant<int, 0>);

    private:
        template<class FwdIterator>
        bool replaceAliased(
            iterator /* i1 */,
            iterator /* i2 */,
            FwdIterator /* s1 */,
            FwdIterator /* s2 */,
            std::false_type) {
            return false;
        }

        template<class FwdIterator>
        bool replaceAliased(
            iterator i1, iterator i2, FwdIterator s1, FwdIterator s2, std::true_type);

        template<class FwdIterator>
        void replaceImpl(
            iterator i1,
            iterator i2,
            FwdIterator s1,
            FwdIterator s2,
            std::forward_iterator_tag);

        template<class InputIterator>
        void replaceImpl(
            iterator i1,
            iterator i2,
            InputIterator b,
            InputIterator e,
            std::input_iterator_tag);

    public:
        template<class T1, class T2>
        BasicString &replace(
            iterator i1, iterator i2, T1 first_or_n_or_s, T2 last_or_c_or_n) {
            constexpr bool num1 = std::numeric_limits<T1>::is_specialized,
                    num2 = std::numeric_limits<T2>::is_specialized;
            using Sel =
                    std::integral_constant<int, num1 ? (num2 ? 1 : -1) : (num2 ? 2 : 0)>;
            return replaceImplDiscr(i1, i2, first_or_n_or_s, last_or_c_or_n, Sel());
        }

        size_type copy(value_type *s, size_type n, size_type pos = 0) const {
            enforce<std::out_of_range>(pos <= size(), "");
            procrustes(n, size() - pos);

            if (n != 0) {
                string_internal::podCopy(data() + pos, data() + pos + n, s);
            }
            return n;
        }

        void swap(BasicString &rhs) { store_.swap(rhs.store_); }

        const value_type *c_str() const { return store_.c_str(); }

        const value_type *data() const { return c_str(); }

        value_type *data() { return store_.data(); }

        allocator_type get_allocator() const { return allocator_type(); }

        size_type find(const BasicString &str, size_type pos = 0) const {
            return find(str.data(), pos, str.length());
        }

        size_type find(
            const value_type *needle, size_type pos, size_type nsize) const;

        size_type find(const value_type *s, size_type pos = 0) const {
            return find(s, pos, traitsLength(s));
        }

        size_type find(value_type c, size_type pos = 0) const {
            return find(&c, pos, 1);
        }

        size_type rfind(const BasicString &str, size_type pos = npos) const {
            return rfind(str.data(), pos, str.length());
        }

        size_type rfind(const value_type *s, size_type pos, size_type n) const;

        size_type rfind(const value_type *s, size_type pos = npos) const {
            return rfind(s, pos, traitsLength(s));
        }

        size_type rfind(value_type c, size_type pos = npos) const {
            return rfind(&c, pos, 1);
        }

        size_type find_first_of(const BasicString &str, size_type pos = 0) const {
            return find_first_of(str.data(), pos, str.length());
        }

        size_type find_first_of(
            const value_type *s, size_type pos, size_type n) const;

        size_type find_first_of(const value_type *s, size_type pos = 0) const {
            return find_first_of(s, pos, traitsLength(s));
        }

        size_type find_first_of(value_type c, size_type pos = 0) const {
            return find_first_of(&c, pos, 1);
        }

        size_type find_last_of(
            const BasicString &str, size_type pos = npos) const {
            return find_last_of(str.data(), pos, str.length());
        }

        size_type find_last_of(const value_type *s, size_type pos, size_type n) const;

        size_type find_last_of(const value_type *s, size_type pos = npos) const {
            return find_last_of(s, pos, traitsLength(s));
        }

        size_type find_last_of(value_type c, size_type pos = npos) const {
            return find_last_of(&c, pos, 1);
        }

        size_type find_first_not_of(
            const BasicString &str, size_type pos = 0) const {
            return find_first_not_of(str.data(), pos, str.size());
        }

        size_type find_first_not_of(
            const value_type *s, size_type pos, size_type n) const;

        size_type find_first_not_of(const value_type *s, size_type pos = 0) const {
            return find_first_not_of(s, pos, traitsLength(s));
        }

        size_type find_first_not_of(value_type c, size_type pos = 0) const {
            return find_first_not_of(&c, pos, 1);
        }

        size_type find_last_not_of(
            const BasicString &str, size_type pos = npos) const {
            return find_last_not_of(str.data(), pos, str.length());
        }

        size_type find_last_not_of(
            const value_type *s, size_type pos, size_type n) const;

        size_type find_last_not_of(const value_type *s, size_type pos = npos) const {
            return find_last_not_of(s, pos, traitsLength(s));
        }

        size_type find_last_not_of(value_type c, size_type pos = npos) const {
            return find_last_not_of(&c, pos, 1);
        }

        BasicString substr(size_type pos = 0, size_type n = npos) const & {
            enforce<std::out_of_range>(pos <= size(), "");
            return BasicString(data() + pos, std::min(n, size() - pos));
        }

        BasicString substr(size_type pos = 0, size_type n = npos) && {
            enforce<std::out_of_range>(pos <= size(), "");
            erase(0, pos);
            if (n < size()) {
                resize(n);
            }
            return std::move(*this);
        }

        int compare(const BasicString &str) const {
            // FIX due to Goncalo N M de Carvalho July 18, 2005
            return compare(0, size(), str);
        }

        int compare(size_type pos1, size_type n1, const BasicString &str) const {
            return compare(pos1, n1, str.data(), str.size());
        }

        int compare(size_type pos1, size_type n1, const value_type *s) const {
            return compare(pos1, n1, s, traitsLength(s));
        }

        int compare(
            size_type pos1, size_type n1, const value_type *s, size_type n2) const {
            enforce<std::out_of_range>(pos1 <= size(), "");
            procrustes(n1, size() - pos1);
            // The line below fixed by Jean-Francois Bastien, 04-23-2007. Thanks!
            const int r = traits_type::compare(pos1 + data(), s, std::min(n1, n2));
            return r != 0 ? r : n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
        }

        int compare(
            size_type pos1,
            size_type n1,
            const BasicString &str,
            size_type pos2,
            size_type n2) const {
            enforce<std::out_of_range>(pos2 <= str.size(), "");
            return compare(
                pos1, n1, str.data() + pos2, std::min(n2, str.size() - pos2));
        }

        // Code from Jean-Francois Bastien (03/26/2007)
        int compare(const value_type *s) const {
            // Could forward to compare(0, size(), s, traitsLength(s))
            // but that does two extra checks
            const size_type n1(size()), n2(traitsLength(s));
            const int r = traits_type::compare(data(), s, std::min(n1, n2));
            return r != 0 ? r : n1 > n2 ? 1 : n1 < n2 ? -1 : 0;
        }

        template<typename H>
        H turbo_hash_value(H h, const BasicString &s) {
            return H::combine(std::move(h), std::basic_string_view<E>(s.data(), s.size()));
        }

    private:
        // Data
        Storage store_;
    };

    template<typename E, class T, class A, class S>
    TURBO_NOINLINE typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::traitsLength(const value_type *s) {
        return s ? traits_type::length(s) : 0;
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::operator=(
        const BasicString &lhs) {
        Invariant checker(*this);

        if (TURBO_UNLIKELY(&lhs == this)) {
            return *this;
        }

        return assign(lhs.data(), lhs.size());
    }

    // Move assignment
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::operator=(
        BasicString &&goner) noexcept {
        if (TURBO_UNLIKELY(&goner == this)) {
            // Compatibility with std::basic_string<>,
            // C++11 21.4.2 [string.cons] / 23 requires self-move-assignment support.
            return *this;
        }
        // No need of this anymore
        this->~BasicString();
        // Move the goner into this

        new(&store_) S(std::move(goner.store_));
        return *this;
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::operator=(
        value_type c) {
        Invariant checker(*this);

        if (empty()) {
            store_.expandNoinit(1);
        } else {
            store_.shrink(size() - 1);
        }
        front() = c;
        return *this;
    }

    template<typename E, class T, class A, class S>
    inline void BasicString<E, T, A, S>::resize(
        const size_type n, const value_type c /*= value_type()*/) {
        Invariant checker(*this);

        auto size = this->size();
        if (n <= size) {
            store_.shrink(size - n);
        } else {
            auto const delta = n - size;
            auto pData = store_.expandNoinit(delta);
            string_internal::podFill(pData, pData + delta, c);
        }
        assert(this->size() == n);
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::append(
        const BasicString &str) {
#ifndef NDEBUG
        auto desiredSize = size() + str.size();
#endif
        append(str.data(), str.size());
        assert(size() == desiredSize);
        return *this;
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::append(
        const BasicString &str, const size_type pos, size_type n) {
        const size_type sz = str.size();
        enforce<std::out_of_range>(pos <= sz, "");
        procrustes(n, sz - pos);
        return append(str.data() + pos, n);
    }

    template<typename E, class T, class A, class S>
    TURBO_NOINLINE BasicString<E, T, A, S> &BasicString<E, T, A, S>::append(
        const value_type *s, size_type n) {
        Invariant checker(*this);

        if (TURBO_UNLIKELY(!n)) {
            // Unlikely but must be done
            return *this;
        }
        auto const oldSize = size();
        auto const oldData = data();
        auto pData = store_.expandNoinit(n);

        // Check for aliasing (rare). We could use "<=" here but in theory
        // those do not work for pointers unless the pointers point to
        // elements in the same array. For that reason we use
        // std::less_equal, which is guaranteed to offer a total order
        // over pointers. See discussion at http://goo.gl/Cy2ya for more
        // info.
        std::less_equal<const value_type *> le;
        if (TURBO_UNLIKELY(le(oldData, s) && !le(oldData + oldSize, s))) {
            assert(le(s + n, oldData + oldSize));
            // expandNoinit() could have moved the storage, restore the source.
            s = data() + (s - oldData);
            string_internal::podMove(s, s + n, pData);
        } else {
            string_internal::podCopy(s, s + n, pData);
        }

        assert(size() == oldSize + n);
        return *this;
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::append(
        size_type n, value_type c) {
        Invariant checker(*this);
        auto pData = store_.expandNoinit(n, /* expGrowth = */ true);
        string_internal::podFill(pData, pData + n, c);
        return *this;
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::assign(
        const BasicString &str, const size_type pos, size_type n) {
        const size_type sz = str.size();
        enforce<std::out_of_range>(pos <= sz, "");
        procrustes(n, sz - pos);
        return assign(str.data() + pos, n);
    }

    template<typename E, class T, class A, class S>
    TURBO_NOINLINE BasicString<E, T, A, S> &BasicString<E, T, A, S>::assign(
        const value_type *s, const size_type n) {
        if (n < capacity()) {
            std::memcpy(store_.data_, s, n);
            store_.size_ = n;
            store_.data_[store_.size_] = '\0';
        } else {
            store_.deallocate();
            store_.init_string(s, n);
        }
        return *this;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::istream_type &
    BasicString<E, T, A, S>::getlineImpl(istream_type &is, value_type delim) {
        Invariant checker(*this);

        clear();
        size_t size = 0;
        while (true) {
            size_t avail = capacity() - size;
            // KString has 1 byte extra capacity for the null terminator,
            // and getline null-terminates the read string.
            is.getline(store_.expandNoinit(avail), avail + 1, delim);
            size += is.gcount();

            if (is.bad() || is.eof() || !is.fail()) {
                // Done by either failure, end of file, or normal read.
                if (!is.bad() && !is.eof()) {
                    --size; // gcount() also accounts for the delimiter.
                }
                resize(size);
                break;
            }

            assert(size == this->size());
            assert(size == capacity());
            // Start at minimum allocation 63 + terminator = 64.
            reserve(std::max<size_t>(63, 3 * size / 2));
            // Clear the error so we can continue reading.
            is.clear();
        }
        return is;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::find(
        const value_type *needle,
        const size_type pos,
        const size_type nsize) const {
        auto const size = this->size();
        // nsize + pos can overflow (eg pos == npos), guard against that by checking
        // that nsize + pos does not wrap around.
        if (nsize + pos > size || nsize + pos < pos) {
            return npos;
        }

        if (nsize == 0) {
            return pos;
        }
        // Don't use std::search, use a Boyer-Moore-like trick by comparing
        // the last characters first
        auto const haystack = data();
        auto const nsize_1 = nsize - 1;
        auto const lastNeedle = needle[nsize_1];

        // Boyer-Moore skip value for the last char in the needle. Zero is
        // not a valid value; skip will be computed the first time it's
        // needed.
        size_type skip = 0;

        const E *i = haystack + pos;
        auto iEnd = haystack + size - nsize_1;

        while (i < iEnd) {
            // Boyer-Moore: match the last element in the needle
            while (i[nsize_1] != lastNeedle) {
                if (++i == iEnd) {
                    // not found
                    return npos;
                }
            }
            // Here we know that the last char matches
            // Continue in pedestrian mode
            for (size_t j = 0;;) {
                assert(j < nsize);
                if (i[j] != needle[j]) {
                    // Not found, we can skip
                    // Compute the skip value lazily
                    if (skip == 0) {
                        skip = 1;
                        while (skip <= nsize_1 && needle[nsize_1 - skip] != lastNeedle) {
                            ++skip;
                        }
                    }
                    i += skip;
                    break;
                }
                // Check if done searching
                if (++j == nsize) {
                    // Yay
                    return i - haystack;
                }
            }
        }
        return npos;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::iterator
    BasicString<E, T, A, S>::insertImplDiscr(
        const_iterator i, size_type n, value_type c, std::true_type) {
        Invariant checker(*this);
        assert(i >= cbegin() && i <= cend());
        const size_type pos = i - cbegin();

        auto oldSize = size();
        store_.expandNoinit(n);
        auto b = begin();

        string_internal::podMove(b + pos, b + oldSize, b + pos + n);
        string_internal::podFill(b + pos, b + pos + n, c);

        return b + pos;
    }

    template<typename E, class T, class A, class S>
    template<class InputIter>
    inline typename BasicString<E, T, A, S>::iterator
    BasicString<E, T, A, S>::insertImplDiscr(
        const_iterator i, InputIter b, InputIter e, std::false_type) {
        return insertImpl(
            i, b, e, typename std::iterator_traits<InputIter>::iterator_category());
    }

    template<typename E, class T, class A, class S>
    template<class FwdIterator>
    inline typename BasicString<E, T, A, S>::iterator
    BasicString<E, T, A, S>::insertImpl(
        const_iterator i,
        FwdIterator s1,
        FwdIterator s2,
        std::forward_iterator_tag) {
        Invariant checker(*this);

        assert(i >= cbegin() && i <= cend());
        const size_type pos = i - cbegin();
        auto n = std::distance(s1, s2);
        assert(n >= 0);

        auto oldSize = size();
        store_.expandNoinit(n);
        auto b = begin();
        string_internal::podMove(b + pos, b + oldSize, b + pos + n);
        std::copy(s1, s2, b + pos);

        return b + pos;
    }

    template<typename E, class T, class A, class S>
    template<class InputIterator>
    inline typename BasicString<E, T, A, S>::iterator
    BasicString<E, T, A, S>::insertImpl(
        const_iterator i,
        InputIterator b,
        InputIterator e,
        std::input_iterator_tag) {
        const auto pos = i - cbegin();
        BasicString temp(cbegin(), i);
        for (; b != e; ++b) {
            temp.push_back(*b);
        }
        temp.append(i, cend());
        swap(temp);
        return begin() + pos;
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::replaceImplDiscr(
        iterator i1,
        iterator i2,
        const value_type *s,
        size_type n,
        std::integral_constant<int, 2>) {
        assert(i1 <= i2);
        assert(begin() <= i1 && i1 <= end());
        assert(begin() <= i2 && i2 <= end());
        return replace(i1, i2, s, s + n);
    }

    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::replaceImplDiscr(
        iterator i1,
        iterator i2,
        size_type n2,
        value_type c,
        std::integral_constant<int, 1>) {
        const size_type n1 = i2 - i1;
        if (n1 > n2) {
            std::fill(i1, i1 + n2, c);
            erase(i1 + n2, i2);
        } else {
            std::fill(i1, i2, c);
            insert(i2, n2 - n1, c);
        }
        assert(isSane());
        return *this;
    }

    template<typename E, class T, class A, class S>
    template<class InputIter>
    inline BasicString<E, T, A, S> &BasicString<E, T, A, S>::replaceImplDiscr(
        iterator i1,
        iterator i2,
        InputIter b,
        InputIter e,
        std::integral_constant<int, 0>) {
        using Cat = typename std::iterator_traits<InputIter>::iterator_category;
        replaceImpl(i1, i2, b, e, Cat());
        return *this;
    }

    template<typename E, class T, class A, class S>
    template<class FwdIterator>
    inline bool BasicString<E, T, A, S>::replaceAliased(
        iterator i1, iterator i2, FwdIterator s1, FwdIterator s2, std::true_type) {
        std::less_equal<const value_type *> le{};
        const bool aliased = le(&*begin(), &*s1) && le(&*s1, &*end());
        if (!aliased) {
            return false;
        }
        // Aliased replace, copy to new string
        BasicString temp;
        temp.reserve(size() - (i2 - i1) + std::distance(s1, s2));
        temp.append(begin(), i1).append(s1, s2).append(i2, end());
        swap(temp);
        return true;
    }

    template<typename E, class T, class A, class S>
    template<class FwdIterator>
    inline void BasicString<E, T, A, S>::replaceImpl(
        iterator i1,
        iterator i2,
        FwdIterator s1,
        FwdIterator s2,
        std::forward_iterator_tag) {
        Invariant checker(*this);

        // Handle aliased replace
        using Sel = std::bool_constant<
            std::is_same<FwdIterator, iterator>::value ||
            std::is_same<FwdIterator, const_iterator>::value>;
        if (replaceAliased(i1, i2, s1, s2, Sel())) {
            return;
        }

        auto const n1 = i2 - i1;
        assert(n1 >= 0);
        auto const n2 = std::distance(s1, s2);
        DKCHECK(n2 >= 0);

        if (n1 > n2) {
            // shrinks
            std::copy(s1, s2, i1);
            erase(i1 + n2, i2);
        } else {
            // grows
            s1 = string_internal::copy_n(s1, n1, i1).first;
            insert(i2, s1, s2);
        }
        assert(isSane());
    }

    template<typename E, class T, class A, class S>
    template<class InputIterator>
    inline void BasicString<E, T, A, S>::replaceImpl(
        iterator i1,
        iterator i2,
        InputIterator b,
        InputIterator e,
        std::input_iterator_tag) {
        BasicString temp(begin(), i1);
        temp.append(b, e).append(i2, end());
        swap(temp);
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::rfind(
        const value_type *s, size_type pos, size_type n) const {
        if (n > length()) {
            return npos;
        }
        pos = std::min(pos, length() - n);
        if (n == 0) {
            return pos;
        }

        const_iterator i(begin() + pos);
        for (;; --i) {
            if (traits_type::eq(*i, *s) && traits_type::compare(&*i, s, n) == 0) {
                return i - begin();
            }
            if (i == begin()) {
                break;
            }
        }
        return npos;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::find_first_of(
        const value_type *s, size_type pos, size_type n) const {
        if (pos > length() || n == 0) {
            return npos;
        }
        const_iterator i(begin() + pos), finish(end());
        for (; i != finish; ++i) {
            if (traits_type::find(s, n, *i) != nullptr) {
                return i - begin();
            }
        }
        return npos;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::find_last_of(
        const value_type *s, size_type pos, size_type n) const {
        if (!empty() && n > 0) {
            pos = std::min(pos, length() - 1);
            const_iterator i(begin() + pos);
            for (;; --i) {
                if (traits_type::find(s, n, *i) != nullptr) {
                    return i - begin();
                }
                if (i == begin()) {
                    break;
                }
            }
        }
        return npos;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::find_first_not_of(
        const value_type *s, size_type pos, size_type n) const {
        if (pos < length()) {
            const_iterator i(begin() + pos), finish(end());
            for (; i != finish; ++i) {
                if (traits_type::find(s, n, *i) == nullptr) {
                    return i - begin();
                }
            }
        }
        return npos;
    }

    template<typename E, class T, class A, class S>
    inline typename BasicString<E, T, A, S>::size_type
    BasicString<E, T, A, S>::find_last_not_of(
        const value_type *s, size_type pos, size_type n) const {
        if (!this->empty()) {
            pos = std::min(pos, size() - 1);
            const_iterator i(begin() + pos);
            for (;; --i) {
                if (traits_type::find(s, n, *i) == nullptr) {
                    return i - begin();
                }
                if (i == begin()) {
                    break;
                }
            }
        }
        return npos;
    }

    // non-member functions
    // C++11 21.4.8.1/1
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        BasicString<E, T, A, S> result;
        result.reserve(lhs.size() + rhs.size());
        result.append(lhs).append(rhs);
        return result;
    }

    // C++11 21.4.8.1/2
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        BasicString<E, T, A, S> &&lhs, const BasicString<E, T, A, S> &rhs) {
        return std::move(lhs.append(rhs));
    }

    // C++11 21.4.8.1/3
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        const BasicString<E, T, A, S> &lhs, BasicString<E, T, A, S> &&rhs) {
        if (rhs.capacity() >= lhs.size() + rhs.size()) {
            // Good, at least we don't need to reallocate
            return std::move(rhs.insert(0, lhs));
        }
        // Meh, no go. Forward to operator+(const&, const&).
        auto const &rhsC = rhs;
        return lhs + rhsC;
    }

    // C++11 21.4.8.1/4
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        BasicString<E, T, A, S> &&lhs, BasicString<E, T, A, S> &&rhs) {
        return std::move(lhs.append(rhs));
    }

    // C++11 21.4.8.1/5
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        const E *lhs, const BasicString<E, T, A, S> &rhs) {
        //
        BasicString<E, T, A, S> result;
        const auto len = BasicString<E, T, A, S>::traits_type::length(lhs);
        result.reserve(len + rhs.size());
        result.append(lhs, len).append(rhs);
        return result;
    }

    // C++11 21.4.8.1/6
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        const E *lhs, BasicString<E, T, A, S> &&rhs) {
        //
        const auto len = BasicString<E, T, A, S>::traits_type::length(lhs);
        if (rhs.capacity() >= len + rhs.size()) {
            // Good, at least we don't need to reallocate
            rhs.insert(rhs.begin(), lhs, lhs + len);
            return std::move(rhs);
        }
        // Meh, no go. Do it by hand since we have len already.
        BasicString<E, T, A, S> result;
        result.reserve(len + rhs.size());
        result.append(lhs, len).append(rhs);
        return result;
    }

    // C++11 21.4.8.1/7
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        E lhs, const BasicString<E, T, A, S> &rhs) {
        BasicString<E, T, A, S> result;
        result.reserve(1 + rhs.size());
        result.push_back(lhs);
        result.append(rhs);
        return result;
    }

    // C++11 21.4.8.1/8
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        E lhs, BasicString<E, T, A, S> &&rhs) {
        //
        if (rhs.capacity() > rhs.size()) {
            // Good, at least we don't need to reallocate

            rhs.insert(rhs.begin(), lhs);
            return std::move(rhs);
        }
        // Meh, no go. Forward to operator+(E, const&).
        auto const &rhsC = rhs;
        return lhs + rhsC;
    }

    // C++11 21.4.8.1/9
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        const BasicString<E, T, A, S> &lhs, const E *rhs) {
        typedef typename BasicString<E, T, A, S>::size_type size_type;
        typedef typename BasicString<E, T, A, S>::traits_type traits_type;

        BasicString<E, T, A, S> result;
        const size_type len = traits_type::length(rhs);
        result.reserve(lhs.size() + len);
        result.append(lhs).append(rhs, len);
        return result;
    }

    // C++11 21.4.8.1/10
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        BasicString<E, T, A, S> &&lhs, const E *rhs) {
        //
        return std::move(lhs += rhs);
    }

    // C++11 21.4.8.1/11
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        const BasicString<E, T, A, S> &lhs, E rhs) {
        BasicString<E, T, A, S> result;
        result.reserve(lhs.size() + 1);
        result.append(lhs);
        result.push_back(rhs);
        return result;
    }

    // C++11 21.4.8.1/12
    template<typename E, class T, class A, class S>
    inline BasicString<E, T, A, S> operator+(
        BasicString<E, T, A, S> &&lhs, E rhs) {
        //
        return std::move(lhs += rhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator==(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
    }

    template<typename E, class T, class A, class S>
    inline bool operator==(std::nullptr_t, const BasicString<E, T, A, S> &) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator==(
        const typename BasicString<E, T, A, S>::value_type *lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs == lhs;
    }

    template<typename E, class T, class A, class S>
    inline bool operator==(const BasicString<E, T, A, S> &, std::nullptr_t) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator==(
        const BasicString<E, T, A, S> &lhs,
        const typename BasicString<E, T, A, S>::value_type *rhs) {
        return lhs.compare(rhs) == 0;
    }

    template<typename E, class T, class A, class S>
    inline bool operator!=(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs == rhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator!=(std::nullptr_t, const BasicString<E, T, A, S> &) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator!=(
        const typename BasicString<E, T, A, S>::value_type *lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs == rhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator!=(const BasicString<E, T, A, S> &, std::nullptr_t) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator!=(
        const BasicString<E, T, A, S> &lhs,
        const typename BasicString<E, T, A, S>::value_type *rhs) {
        return !(lhs == rhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator<(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return lhs.compare(rhs) < 0;
    }

    template<typename E, class T, class A, class S>
    inline bool operator<(const BasicString<E, T, A, S> &, std::nullptr_t) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator<(
        const BasicString<E, T, A, S> &lhs,
        const typename BasicString<E, T, A, S>::value_type *rhs) {
        return lhs.compare(rhs) < 0;
    }

    template<typename E, class T, class A, class S>
    inline bool operator<(std::nullptr_t, const BasicString<E, T, A, S> &) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator<(
        const typename BasicString<E, T, A, S>::value_type *lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs.compare(lhs) > 0;
    }

    template<typename E, class T, class A, class S>
    inline bool operator>(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs < lhs;
    }

    template<typename E, class T, class A, class S>
    inline bool operator>(const BasicString<E, T, A, S> &, std::nullptr_t) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator>(
        const BasicString<E, T, A, S> &lhs,
        const typename BasicString<E, T, A, S>::value_type *rhs) {
        return rhs < lhs;
    }

    template<typename E, class T, class A, class S>
    inline bool operator>(std::nullptr_t, const BasicString<E, T, A, S> &) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator>(
        const typename BasicString<E, T, A, S>::value_type *lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs < lhs;
    }

    template<typename E, class T, class A, class S>
    inline bool operator<=(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(rhs < lhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator<=(const BasicString<E, T, A, S> &, std::nullptr_t) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator<=(
        const BasicString<E, T, A, S> &lhs,
        const typename BasicString<E, T, A, S>::value_type *rhs) {
        return !(rhs < lhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator<=(std::nullptr_t, const BasicString<E, T, A, S> &) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator<=(
        const typename BasicString<E, T, A, S>::value_type *lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(rhs < lhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator>=(
        const BasicString<E, T, A, S> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs < rhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator>=(const BasicString<E, T, A, S> &, std::nullptr_t) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator>=(
        const BasicString<E, T, A, S> &lhs,
        const typename BasicString<E, T, A, S>::value_type *rhs) {
        return !(lhs < rhs);
    }

    template<typename E, class T, class A, class S>
    inline bool operator>=(std::nullptr_t, const BasicString<E, T, A, S> &) = delete;

    template<typename E, class T, class A, class S>
    inline bool operator>=(
        const typename BasicString<E, T, A, S>::value_type *lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs < rhs);
    }

    // C++11 21.4.8.8
    template<typename E, class T, class A, class S>
    void swap(BasicString<E, T, A, S> &lhs, BasicString<E, T, A, S> &rhs) {
        lhs.swap(rhs);
    }

    // TODO: make this faster.
    template<typename E, class T, class A, class S>
    inline std::basic_istream<
        typename BasicString<E, T, A, S>::value_type,
        typename BasicString<E, T, A, S>::traits_type> &
    operator>>(
        std::basic_istream<
            typename BasicString<E, T, A, S>::value_type,
            typename BasicString<E, T, A, S>::traits_type> &is,
        BasicString<E, T, A, S> &str) {
        typedef std::basic_istream<
                    typename BasicString<E, T, A, S>::value_type,
                    typename BasicString<E, T, A, S>::traits_type>
                _istream_type;
        typename _istream_type::sentry sentry(is);
        size_t extracted = 0;
        typename _istream_type::iostate err = _istream_type::goodbit;
        if (sentry) {
            auto n = is.width();
            if (n <= 0) {
                n = str.max_size();
            }
            str.erase();
            for (auto got = is.rdbuf()->sgetc(); extracted != size_t(n); ++extracted) {
                if (got == T::eof()) {
                    err |= _istream_type::eofbit;
                    is.width(0);
                    break;
                }
                if (isspace(got)) {
                    break;
                }
                str.push_back(got);
                got = is.rdbuf()->snextc();
            }
        }
        if (!extracted) {
            err |= _istream_type::failbit;
        }
        if (err) {
            is.setstate(err);
        }
        return is;
    }

    template<typename E, class T, class A, class S>
    inline std::basic_ostream<
        typename BasicString<E, T, A, S>::value_type,
        typename BasicString<E, T, A, S>::traits_type> &
    operator<<(
        std::basic_ostream<
            typename BasicString<E, T, A, S>::value_type,
            typename BasicString<E, T, A, S>::traits_type> &os,
        const BasicString<E, T, A, S> &str) {
#ifdef _LIBCPP_VERSION
        typedef std::basic_ostream<
                    typename BasicString<E, T, A, S>::value_type,
                    typename BasicString<E, T, A, S>::traits_type>
                _ostream_type;
        typename _ostream_type::sentry _s(os);
        if (_s) {
            typedef std::ostreambuf_iterator<
                        typename BasicString<E, T, A, S>::value_type,
                        typename BasicString<E, T, A, S>::traits_type>
                    _Ip;
            size_t __len = str.size();
            bool __left =
                    (os.flags() & _ostream_type::adjustfield) == _ostream_type::left;
            if (__pad_and_output(
                    _Ip(os),
                    str.data(),
                    __left ? str.data() + __len : str.data(),
                    str.data() + __len,
                    os,
                    os.fill())
                .failed()) {
                os.setstate(_ostream_type::badbit | _ostream_type::failbit);
            }
        }
#elif defined(_MSC_VER)
        typedef decltype(os.precision()) streamsize;
        // MSVC doesn't define __ostream_insert
        os.write(str.data(), static_cast<streamsize>(str.size()));
#else
        std::__ostream_insert(os, str.data(), str.size());
#endif
        return os;
    }

    // basic_string compatibility routines

    template<typename E, class T, class A, class S, class A2>
    inline bool operator==(
        const BasicString<E, T, A, S> &lhs,
        const std::basic_string<E, T, A2> &rhs) {
        auto r = lhs.compare(0, lhs.size(), rhs.data(), rhs.size());
        return r == 0;
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator==(
        const std::basic_string<E, T, A2> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs == lhs;
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator!=(
        const BasicString<E, T, A, S> &lhs,
        const std::basic_string<E, T, A2> &rhs) {
        return !(lhs == rhs);
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator!=(
        const std::basic_string<E, T, A2> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs == rhs);
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator<(
        const BasicString<E, T, A, S> &lhs,
        const std::basic_string<E, T, A2> &rhs) {
        return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) < 0;
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator>(
        const BasicString<E, T, A, S> &lhs,
        const std::basic_string<E, T, A2> &rhs) {
        return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) > 0;
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator<(
        const std::basic_string<E, T, A2> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs > lhs;
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator>(
        const std::basic_string<E, T, A2> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return rhs < lhs;
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator<=(
        const BasicString<E, T, A, S> &lhs,
        const std::basic_string<E, T, A2> &rhs) {
        return !(lhs > rhs);
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator>=(
        const BasicString<E, T, A, S> &lhs,
        const std::basic_string<E, T, A2> &rhs) {
        return !(lhs < rhs);
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator<=(
        const std::basic_string<E, T, A2> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs > rhs);
    }

    template<typename E, class T, class A, class S, class A2>
    inline bool operator>=(
        const std::basic_string<E, T, A2> &lhs,
        const BasicString<E, T, A, S> &rhs) {
        return !(lhs < rhs);
    }

    typedef BasicString<char> KString;

    // KString is relocatable
    //template<class T, class R, class A, class S>
    //TURBO_ASSUME_RELOCATABLE(BasicString<T, R, A, S>);

    // Compatibility function, to make sure toStdString(s) can be called
    // to convert a std::string or KString variable s into type std::string
    // with very little overhead if s was already std::string
    inline std::string toStdString(const fermat::KString &s) {
        return std::string(s.data(), s.size());
    }

    inline const std::string &toStdString(const std::string &s) {
        return s;
    }

    // If called with a temporary, the compiler will select this overload instead
    // of the above, so we don't return a (lvalue) reference to a temporary.
    inline std::string &&toStdString(std::string &&s) {
        return std::move(s);
    }

    template<typename Sink>
    void turbo_stringify(Sink &sink, const KString &s) {
        sink.Append(std::string_view(s.data(), s.size()));
    }
} // namespace fermat


namespace std {
    template<typename E, class T, class A, class S>
    struct hash<fermat::BasicString<E, T, A, S> > {
        size_t operator()(const fermat::BasicString<E, T, A, S> &s) const noexcept {
            return hash<basic_string_view<E, T> >()(basic_string_view<E, T>(s.data(), s.size()));
        }
    };
}
