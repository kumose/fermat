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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <memory>
#include <optional>
#include <turbo/strings/str_format.h>
#include <fermat/memory/object_pool.h>
#include <fermat/container/utility.h>
#include <fermat/container/traits.h>
#include <fermat/memory/allocator.h>
#include <fermat/container/compressed_pair.h>

namespace fermat {
    template<typename T, size_t Alignment, typename Allocator>
    struct BufferBase {
        using allocator_type = Allocator;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

        static const size_type npos = (size_type) -1; /// 'npos' means non-valid position or simply non-position.
        static const size_type kMaxSize = (size_type) -2;
        /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.

        size_type GetNewCapacity(size_type currentSize);

    protected:
        T *_begin{nullptr};
        T *_end{nullptr};
        compressed_pair<T *, allocator_type> _capacity_end{nullptr, allocator_type{}};

    public:
        BufferBase(const allocator_type &allocator);

        BufferBase(size_type n, const allocator_type &allocator = allocator_type{});

        virtual ~BufferBase();

        allocator_type &get_allocator() noexcept;

        const allocator_type &get_allocator() const noexcept;

        void set_allocator(const allocator_type &allocator) noexcept;

    protected:
        T *do_allocate(size_type *n);

        size_type do_allocate_size(size_type n);

        void do_free(T *p, size_type n);

        void uninitialized_n(size_t n);
    }; // BufferBase


    /// Buffer
    ///
    /// Implements a dynamic array.
    ///
    template<typename T, size_t Alignment = 0, typename Allocator= BasicAllocator<T, Alignment> >
    class Buffer : public BufferBase<T, Alignment, Allocator> {
        // --- The Safety Lock ---
        // Since we've optimized all paths (append, resize, assign) to use bitwise
        // operations (memcpy/memset) and skipped destructor calls, we MUST
        // ensure T is trivially copyable and has a trivial destructor.
        static_assert(std::is_trivially_copyable_v<T>,
                      "fermat::Buffer only supports trivially copyable types.");
        static_assert(std::is_trivially_destructible_v<T>,
                      "fermat::Buffer only supports types with trivial destructors.");

        typedef BufferBase<T, Alignment, Allocator> base_type;
        typedef Buffer<T, Alignment, Allocator> this_type;

        template<class T2, class Allocator2, class U>
        friend typename Buffer<T2, Alignment>::size_type erase_unsorted(Buffer<T2, Alignment> &c, const U &value);

        template<class T2, class Allocator2, class P>
        friend typename Buffer<T2, Alignment>::size_type erase_unsorted_if(Buffer<T2, Alignment> &c, P predicate);

    protected:
        using base_type::_begin;
        using base_type::_end;
        using base_type::_capacity_end;
        using base_type::do_allocate;
        using base_type::do_allocate_size;
        using base_type::do_free;

    public:
        typedef T value_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
        // Maintainer note: We want to leave iterator defined as T* -- at least in release builds -- as this gives some algorithms an advantage that optimizers cannot get around.
        typedef T *iterator;
        // Note: iterator is simply T* right now, but this will likely change in the future, at least for debug builds.
        typedef const T *const_iterator;
        //       Do not write code that relies on iterator being T*. The reason it will
        typedef std::reverse_iterator<iterator> reverse_iterator;
        //       change in the future is that a debugging iterator system will be created.
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef typename base_type::allocator_type allocator_type;

        using base_type::npos;
        using base_type::GetNewCapacity;

        static_assert(!std::is_const<value_type>::value, "Buffer<T> value_type must be non-const.");
        static_assert(!std::is_volatile<value_type>::value, "Buffer<T> value_type must be non-volatile.");

    public:
        using base_type::get_allocator;
        using base_type::set_allocator;

    public:
        Buffer(const allocator_type &allocator = allocator_type{}) noexcept;

        explicit Buffer(size_type n, const allocator_type &allocator = allocator_type{});

        Buffer(size_type n, const value_type &value, const allocator_type &allocator = allocator_type{});

        Buffer(const this_type &x, const allocator_type &allocator = allocator_type{});

        Buffer(this_type &&x) noexcept;

        Buffer(std::initializer_list<value_type> ilist, const allocator_type &allocator = allocator_type{});

        // note: this has pre-C++11 semantics:
        // this constructor is equivalent to the constructor Buffer(static_cast<size_type>(first), static_cast<value_type>(last), allocator) if InputIterator is an integral type.
        template<typename InputIterator>
        Buffer(InputIterator first, InputIterator last, const allocator_type &allocator = allocator_type{});

        ~Buffer() override = default;

        this_type &operator=(const this_type &x);

        this_type &operator=(std::initializer_list<value_type> ilist);

        this_type &operator=(this_type &&x) noexcept;

        void swap(this_type &x) noexcept;

        void assign(size_type n, const value_type &value);

        template<typename InputIterator>
        void assign(InputIterator first, InputIterator last);

        void assign(std::initializer_list<value_type> ilist);

        void assign(const T *data, size_type n);

        iterator begin() noexcept;

        const_iterator begin() const noexcept;

        const_iterator cbegin() const noexcept;

        iterator end() noexcept;

        const_iterator end() const noexcept;

        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;

        const_reverse_iterator rbegin() const noexcept;

        const_reverse_iterator crbegin() const noexcept;

        reverse_iterator rend() noexcept;

        const_reverse_iterator rend() const noexcept;

        const_reverse_iterator crend() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        size_type size() const noexcept;

        size_type capacity() const noexcept;

        void resize(size_type n, const value_type &value);

        void resize(size_type n);

        void reserve(size_type n);

        void set_capacity(size_type n = base_type::npos);

        // Revises the capacity to the user-specified value. Resizes the container to match the capacity if the requested capacity n is less than the current size. If n == npos then the capacity is reallocated (if necessary) such that capacity == size.
        void shrink_to_fit(); // C++11 function which is the same as set_capacity().

        pointer data() noexcept;

        const_pointer data() const noexcept;

        reference operator[](size_type n);

        const_reference operator[](size_type n) const;

        reference at(size_type n);

        const_reference at(size_type n) const;

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        void pop_back();

        iterator insert(const_iterator position, const value_type &value);

        iterator insert(const_iterator position, size_type n, const value_type &value);

        iterator insert(const_iterator position, value_type &&value);

        iterator insert(const_iterator position, std::initializer_list<value_type> ilist);

        // note: this has pre-C++11 semantics:
        // this function is equivalent to insert(const_iterator position, static_cast<size_type>(first), static_cast<value_type>(last)) if InputIterator is an integral type.
        // ie. same as insert(const_iterator position, size_type n, const value_type& value)
        template<typename InputIterator>
        iterator insert(const_iterator position, InputIterator first, InputIterator last);

        iterator erase_first(const T &value);

        iterator erase_first_unsorted(const T &value);

        // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the Buffer over the erased position.
        reverse_iterator erase_last(const T &value);

        reverse_iterator erase_last_unsorted(const T &value);

        // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the Buffer over the erased position.

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        iterator erase_unsorted(const_iterator position);

        // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the Buffer over the erased position.

        reverse_iterator erase(const_reverse_iterator position);

        reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        reverse_iterator erase_unsorted(const_reverse_iterator position);

        void clear() noexcept;

        void reset_lose_memory() noexcept;

        // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        [[nodiscard]] bool validate() const noexcept;

        void bestow(T *data, size_type size, size_type capacity) noexcept;

        T *seize(size_type *size, size_type *capacity) noexcept;

        /// additional api

        void append(value_type value);

        void append_confident(value_type value);

        void append(const value_type *value, size_type size);

        void append_confident(const value_type *TURBO_RESTRICT value, size_type size);

        reference append();

        void *append_uninitialized();

        [[nodiscard]] bool is_stringify() const noexcept;

    protected:
        // These functions do the real work of maintaining the Buffer. You will notice
        // that many of them have the same name but are specialized on iterator_tag
        // (iterator categories). This is because in these cases there is an optimized
        // implementation that can be had for some cases relative to others. Functions
        // which aren't referenced are neither compiled nor linked into the application.
        template<bool bMove>
        struct should_move_or_copy_tag {
        };


        // Allocates a pointer of array count n and copy-constructs it with [first,last).
        pointer do_realloc(size_type *newCapacity, const T *first, const T *last);

        template<typename Integer>
        void DoInit(Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void DoInit(InputIterator first, InputIterator last, std::false_type);

        template<typename InputIterator>
        void DoInitFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag);

        template<typename ForwardIterator>
        void DoInitFromIterator(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);

        template<typename Integer, bool bMove>
        void DoAssign(Integer n, Integer value, std::true_type);

        template<typename InputIterator, bool bMove>
        void DoAssign(InputIterator first, InputIterator last, std::false_type);

        void DoAssignValues(size_type n, const value_type &value);

        template<typename InputIterator, bool bMove>
        void DoAssignFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag);

        template<typename RandomAccessIterator, bool bMove>
        void DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last,
                                  std::random_access_iterator_tag);

        template<typename Integer>
        void DoInsert(const_iterator position, Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void DoInsert(const_iterator position, InputIterator first, InputIterator last, std::false_type);

        template<typename InputIterator>
        void DoInsertFromIterator(const_iterator position, InputIterator first, InputIterator last,
                                  std::input_iterator_tag);

        template<typename BidirectionalIterator>
        void DoInsertFromIterator(const_iterator position, BidirectionalIterator first, BidirectionalIterator last,
                                  std::bidirectional_iterator_tag);

        void DoInsertValues(const_iterator position, size_type n, const value_type &value);

        void do_insert_values_end(size_type n); // Default constructs n values
        void do_insert_values_end(size_type n, const value_type &value);

        template<typename... Args>
        void do_insert_value(const_iterator position, Args &&... args);

        template<typename... Args>
        void do_insert_value_end(Args &&... args);

        void DoClearCapacity();

        void DoGrow(size_type newCapacity);

        void DoSwap(this_type &x) noexcept;
    }; // class Buffer


    ///////////////////////////////////////////////////////////////////////
    // BufferBase
    ///////////////////////////////////////////////////////////////////////
    template<typename T, size_t Alignment, typename Allocator>
    inline BufferBase<T, Alignment, Allocator>::BufferBase(const allocator_type &allocator) : _capacity_end(
        nullptr, allocator) {
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline BufferBase<T, Alignment,
        Allocator>::BufferBase(size_type n, const allocator_type &allocator) : _capacity_end(nullptr, allocator) {
        uninitialized_n(n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline BufferBase<T, Alignment, Allocator>::~BufferBase() {
        if (_begin) {
            do_free(_begin, _capacity_end.first() - _begin);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline T *BufferBase<T, Alignment, Allocator>::do_allocate(size_type *n) {
        // If n is zero, then we allocate no memory and just return nullptr.
        // This is fine, as our default ctor initializes with NULL pointers.
        if (TURBO_LIKELY(n)) {
            auto ptr = _capacity_end.second().allocate(n);
            return ptr;
        } else {
            return nullptr;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename BufferBase<T, Alignment, Allocator>::allocator_type &BufferBase<T, Alignment,
        Allocator>::get_allocator() noexcept {
        return _capacity_end.second();
    }

    template<typename T, size_t Alignment, typename Allocator>
    const typename BufferBase<T, Alignment, Allocator>::allocator_type &BufferBase<T, Alignment,
        Allocator>::get_allocator() const noexcept {
        return _capacity_end.second();
    }

    template<typename T, size_t Alignment, typename Allocator>
    void BufferBase<T, Alignment, Allocator>::set_allocator(const allocator_type &allocator) noexcept {
        _capacity_end.second() = allocator;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename BufferBase<T, Alignment, Allocator>::size_type BufferBase<T, Alignment,
        Allocator>::do_allocate_size(size_type n) {
        if (TURBO_LIKELY(n)) {
            return _capacity_end.second().good_size(n);
        } else {
            return 0;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    void BufferBase<T, Alignment, Allocator>::uninitialized_n(size_t n) {
        _begin = _capacity_end.second().allocate(&n);
        _end = _begin;
        _capacity_end.first() = _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void BufferBase<T, Alignment, Allocator>::do_free(T *p, size_type n) {
        if (p) {
            _capacity_end.second().deallocate(p, n);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename BufferBase<T, Alignment, Allocator>::size_type
    BufferBase<T, Alignment, Allocator>::GetNewCapacity(size_type currentSize) {
        // This function must return a value larger than currentSize.
        if (currentSize > 0) {
            if (currentSize < (std::numeric_limits<size_type>::max() / 2)) {
                return 2 * currentSize;
            } else {
                KCHECK(currentSize < std::numeric_limits<size_type>::max()) <<
                                 "Buffer growth will overflow the value of the capacity! This is extremely bad!";
                return std::numeric_limits<size_type>::max();
            }
        } else {
            return 1;
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // Buffer
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline Buffer<T, Alignment, Allocator>::Buffer(const allocator_type &allocator) noexcept
        : base_type(allocator) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Buffer<T, Alignment, Allocator>::Buffer(size_type n, const allocator_type &allocator)
        : base_type(n, allocator) {
        if (n > 0) {
            // For trivial types, value-initialization is equivalent to zero-initialization.
            // memset is highly optimized by libc and often uses SIMD/vector instructions.
            std::memset(static_cast<void *>(_begin), 0, n * sizeof(T));
        }
        _end = _begin + n;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline Buffer<T, Alignment, Allocator>::Buffer(size_type n, const value_type &value,
                                                   const allocator_type &allocator)
        : base_type(n, allocator) {
        if (n > 0) {
            // For trivial types, simple assignment is sufficient and highly optimizable.
            // Modern compilers will vectorize this loop into SIMD instructions.
            for (size_type i = 0; i < n; ++i) {
                _begin[i] = value;
            }
        }
        _end = _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Buffer<T, Alignment, Allocator>::Buffer(const this_type &x, const allocator_type &allocator)
        : base_type(x.size(), allocator) {
        const size_type n = x.size();
        if (n > 0) {
            std::memcpy(static_cast<void *>(_begin),
                        static_cast<const void *>(x._begin),
                        n * sizeof(T));
            _end = _begin + n;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline Buffer<T, Alignment, Allocator>::Buffer(this_type &&x) noexcept : base_type(allocator_type{}) {
        DoSwap(x);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Buffer<T, Alignment, Allocator>::Buffer(std::initializer_list<value_type> ilist,
                                                   const allocator_type &allocator)
        : base_type(allocator) {
        DoInit(ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline Buffer<T, Alignment, Allocator>::Buffer(InputIterator first, InputIterator last,
                                                   const allocator_type &allocator)
        : base_type(allocator) {
        DoInit(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename Buffer<T, Alignment, Allocator>::this_type &
    Buffer<T, Alignment, Allocator>::operator=(const this_type &x) {
        if (this != &x) // If not assigning to self...
        {
            DoAssign<const_iterator, false>(x.begin(), x.end(), std::false_type());
        }

        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename Buffer<T, Alignment, Allocator>::this_type &
    Buffer<T, Alignment, Allocator>::operator=(std::initializer_list<value_type> ilist) {
        typedef typename std::initializer_list<value_type>::iterator InputIterator;
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC());
        // initializer_list has const elements and so we can't move from them.
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename Buffer<T, Alignment, Allocator>::this_type &
    Buffer<T, Alignment, Allocator>::operator=(this_type &&x) noexcept {
        if (this != &x) {
            DoClearCapacity();
            // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
            swap(x);
            // member swap handles the case that x has a different allocator than our allocator by doing a copy.
        }
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::assign(size_type n, const value_type &value) {
        DoAssignValues(n, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Buffer<T, Alignment, Allocator>::assign(InputIterator first, InputIterator last) {
        // It turns out that the C++ std::Buffer<int, int> specifies a two argument
        // version of assign that takes (int size, int value). These are not iterators,
        // so we need to do a template compiler trick to do the right thing.
        DoAssign<InputIterator, false>(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::assign(std::initializer_list<value_type> ilist) {
        typedef typename std::initializer_list<value_type>::iterator InputIterator;
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC());
        // initializer_list has const elements and so we can't move from them.
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::assign(const T *data, size_type n) {
        if (n > static_cast<size_type>(_capacity_end.first() - _begin)) {
            // Optimization: For assign, we don't need to preserve old data,
            // so we can just reallocate or use a specialized path.
            set_capacity(n);
        }

        if (n > 0) {
            std::memcpy(static_cast<void *>(_begin), static_cast<const void *>(data), n * sizeof(T));
        }
        _end = _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::begin() noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_iterator
    Buffer<T, Alignment, Allocator>::begin() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_iterator
    Buffer<T, Alignment, Allocator>::cbegin() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::end() noexcept {
        return _end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_iterator
    Buffer<T, Alignment, Allocator>::end() const noexcept {
        return _end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_iterator
    Buffer<T, Alignment, Allocator>::cend() const noexcept {
        return _end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::rbegin() noexcept {
        return reverse_iterator(_end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reverse_iterator
    Buffer<T, Alignment, Allocator>::rbegin() const noexcept {
        return const_reverse_iterator(_end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reverse_iterator
    Buffer<T, Alignment, Allocator>::crbegin() const noexcept {
        return const_reverse_iterator(_end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::rend() noexcept {
        return reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reverse_iterator
    Buffer<T, Alignment, Allocator>::rend() const noexcept {
        return const_reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reverse_iterator
    Buffer<T, Alignment, Allocator>::crend() const noexcept {
        return const_reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    bool Buffer<T, Alignment, Allocator>::empty() const noexcept {
        return (_begin == _end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::size_type
    Buffer<T, Alignment, Allocator>::size() const noexcept {
        return (size_type) (_end - _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::size_type
    Buffer<T, Alignment, Allocator>::capacity() const noexcept {
        return (size_type) (_capacity_end.first() - _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::resize(size_type n, const value_type &value) {
        const auto nPrevSize = static_cast<size_type>(_end - _begin);

        if (n > nPrevSize) {
            // Upsize: Use the optimized trivial fill path.
            do_insert_values_end(n - nPrevSize, value);
        } else {
            // Downsize: Trivial elements require no formal destruction.
            // This reduces the operation to a simple pointer assignment (O(1)).
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::resize(size_type n) {
        const auto nPrevSize = static_cast<size_type>(_end - _begin);
        if (n > nPrevSize) {
            // Upsize: Insert (n - nPrevSize) default-initialized elements.
            // For trivial types, this is optimized to std::memset(..., 0, ...).
            do_insert_values_end(n - nPrevSize);
        } else {
            // Downsize: No destruction needed for trivial types.
            // We simply move the end pointer, making it O(1).
            _end = _begin + n;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::reserve(size_type n) {
        // If the user wants to reduce the reserved memory, there is the set_capacity function.
        if (n > size_type(_capacity_end.first() - _begin)) {
            // If n > capacity ...
            DoGrow(n);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::set_capacity(size_type n) {
        const auto nPrevSize = static_cast<size_type>(_end - _begin);

        if (n == npos || n <= nPrevSize) {
            // --- Case 1: Shrink or same size ---
            if (n == 0) {
                clear();
            } else if (n < nPrevSize) {
                // For trivial types, resize(n) just moves the _end pointer.
                _end = _begin + n;
            }
            shrink_to_fit();
        } else {
            // --- Case 2: Expand capacity ---
            auto nNewCapacity = n;
            // Note: For trivial types, should_move_tag() is equivalent to bitwise copy.
            const auto pNewData = do_realloc(&nNewCapacity, _begin, _end);


            do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewData + nPrevSize;
            _capacity_end.first() = _begin + nNewCapacity;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::shrink_to_fit() {
        // This is the simplest way to accomplish this, and it is as efficient as any other.
        auto n = do_allocate_size(size());
        if (n < capacity()) {
            this_type temp = this_type(std::move_iterator<iterator>(begin()), std::move_iterator<iterator>(end()));

            // Call DoSwap() rather than swap() as we know our allocators match and we don't want to invoke the code path
            // handling non matching allocators as it imposes additional restrictions on the type of T to be copyable
            DoSwap(temp);
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::pointer
    Buffer<T, Alignment, Allocator>::data() noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_pointer
    Buffer<T, Alignment, Allocator>::data() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reference
    Buffer<T, Alignment, Allocator>::operator[](size_type n) {
        // We allow the user to use a reference to v[0] of an empty container. But this was merely grandfathered in and ideally we shouldn't allow such access to [0].
        //if (TURBO_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(_end - _begin)))))
        //    KCHECK(false) << "Buffer::operator[] -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reference
    Buffer<T, Alignment, Allocator>::operator[](size_type n) const {
        //if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
        //   KCHECK(false) << "Buffer::operator[] -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reference
    Buffer<T, Alignment, Allocator>::at(size_type n) {
        // The difference between at() and operator[] is it signals
        // the requested position is out of range by throwing an
        // out_of_range exception.

        if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
            KCHECK(false) << "Buffer::at -- out of range";
        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reference
    Buffer<T, Alignment, Allocator>::at(size_type n) const {
        if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
            KCHECK(false) << "Buffer::at -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reference
    Buffer<T, Alignment, Allocator>::front() {
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            // We don't allow the user to reference an empty container.
            KCHECK(false) << "Buffer::front -- empty Buffer";

        return *_begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reference
    Buffer<T, Alignment, Allocator>::front() const {
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            // We don't allow the user to reference an empty container.
            KCHECK(false) << "Buffer::front -- empty Buffer";

        return *_begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reference
    Buffer<T, Alignment, Allocator>::back() {
        // if _end is nullptr the expression (_end - 1) is undefined behaviour.
        // any use of back() with an empty Buffer is thus conceptually wrong.
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            KCHECK(false) << "Buffer::back -- empty Buffer";

        return *(_end - 1);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::const_reference
    Buffer<T, Alignment, Allocator>::back() const {
        // if _end is nullptr the expression (_end - 1) is undefined behaviour.
        // any use of back() with an empty Buffer is thus conceptually wrong.
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            KCHECK(false) << "Buffer::back -- empty Buffer";

        return *(_end - 1);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::append(value_type value) {
        if (TURBO_LIKELY(_end < _capacity_end.first())) {
            *_end++ = value;
        } else {
            do_insert_value_end(value);
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::append_confident(value_type value) {
        *_end++ = value;
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::append(const value_type *value, size_type size) {
        if (TURBO_UNLIKELY(size == 0)) return;

        // Check if we need to expand the buffer
        if (TURBO_UNLIKELY(size > static_cast<size_type>(_capacity_end.first() - _end))) {
            auto n = GetNewCapacity(this->size() + size);
            reserve(n);
        }

        // Perform bitwise copy for trivial types
        std::memcpy(static_cast<void *>(_end), static_cast<const void *>(value), size * sizeof(T));
        _end += size;
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::append_confident(const value_type * TURBO_RESTRICT value, size_type size) {
        if (TURBO_UNLIKELY(size == 0)) return;

        // Direct memcpy without any bounds or capacity logic.
        // Maximize throughput for pre-reserved ingestion.
        std::memcpy(static_cast<void *>(_end), static_cast<const void *>(value), size * sizeof(T));
        _end += size;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reference
    Buffer<T, Alignment, Allocator>::append() {
        if (_end < _capacity_end.first())
            *_end++ = T{};
        else
            do_insert_value_end();

        return *(_end - 1); // Same as return back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void *Buffer<T, Alignment, Allocator>::append_uninitialized() {
        if (_end == _capacity_end.first()) {
            const size_type nPrevSize = size_type(_end - _begin);
            const size_type nNewCapacity = GetNewCapacity(nPrevSize);
            DoGrow(nNewCapacity);
        }

        return _end++;
    }

    template<typename T, size_t Alignment, typename Allocator>
    [[nodiscard]] inline bool Buffer<T, Alignment, Allocator>::is_stringify() const noexcept {
        // Condition 1: Type size check. Supports char, char16_t, char32_t, etc.
        // Condition 2: Capacity check. Ensures there's a slot for '\0' beyond _end.
        if constexpr (sizeof(T) <= 4) {
            return (_capacity_end.first() > _end);
        }
        return false;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::pop_back() {
        if (TURBO_UNLIKELY(_end <= _begin))
            KCHECK(false) << "Buffer::pop_back -- empty Buffer";


        --_end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::insert(const_iterator position, const value_type &value) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Buffer::insert -- invalid position";

        // We implment a quick pathway for the case that the insertion position is at the end and we have free capacity for it.
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.

        if ((_end == _capacity_end.first()) || (position != _end))
            do_insert_value(position, value);
        else {
            *_end = value;
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        }

        return _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::insert(const_iterator position, value_type &&value) {
        return emplace(position, std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::insert(const_iterator position, size_type n, const value_type &value) {
        const ptrdiff_t p = position - _begin; // Save this because we might reallocate.
        DoInsertValues(position, n, value);
        return _begin + p;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::insert(const_iterator position, InputIterator first, InputIterator last) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.
        DoInsert(position, first, last, std::is_integral<InputIterator>());
        return _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.
        DoInsert(position, ilist.begin(), ilist.end(), std::false_type());
        return _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::erase(const_iterator position) {
        if (TURBO_UNLIKELY((position < _begin) || (position >= _end)))
            KCHECK(false) << "Buffer::erase -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        auto destPosition = const_cast<value_type *>(position);

        if ((position + 1) < _end)
            std::move(destPosition + 1, _end, destPosition);
        --_end;
        _end->~value_type();
        return destPosition;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::erase(const_iterator first, const_iterator last) {
        if (TURBO_UNLIKELY((first < _begin) || (first > _end) || (last < _begin) || (last > _end) || (last < first)))
            KCHECK(false) << "Buffer::erase -- invalid position";

        if (first != last) {
            const auto dest = const_cast<value_type *>(first);
            const auto src = const_cast<value_type *>(last);
            const auto num_to_move = static_cast<size_type>(_end - src);

            if (num_to_move > 0) {
                // Use memmove to safely handle potential (though unlikely here) overlap
                // and perform high-speed bitwise copy for trivial types.
                std::memmove(static_cast<void *>(dest), static_cast<const void *>(src), num_to_move * sizeof(T));
            }

            // T is trivial, so we simply adjust the end pointer without calling destroy.
            _end -= (last - first);
        }

        return const_cast<value_type *>(first);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::erase_unsorted(const_iterator position) {
        if (TURBO_UNLIKELY((position < _begin) || (position >= _end)))
            KCHECK(false) << "Buffer::erase -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        auto destPosition = const_cast<value_type *>(position);
        *destPosition = std::move(*(_end - 1));

        // pop_back();
        --_end;
        _end->~value_type();

        return destPosition;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator Buffer<T, Alignment, Allocator>::erase_first(
        const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        iterator it = std::find(begin(), end(), value);

        if (it != end())
            return erase(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::iterator
    Buffer<T, Alignment, Allocator>::erase_first_unsorted(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        iterator it = std::find(begin(), end(), value);

        if (it != end())
            return erase_unsorted(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::erase_last(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        reverse_iterator it = std::find(rbegin(), rend(), value);

        if (it != rend())
            return erase(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::erase_last_unsorted(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        reverse_iterator it = std::find(rbegin(), rend(), value);

        if (it != rend())
            return erase_unsorted(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase(last.base(), first.base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::reverse_iterator
    Buffer<T, Alignment, Allocator>::erase_unsorted(const_reverse_iterator position) {
        return reverse_iterator(erase_unsorted((++position).base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::clear() noexcept {
        _end = _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::reset_lose_memory() noexcept {
        // The reset function is a special extension function which unilaterally
        // resets the container to an empty state without freeing the memory of
        // the contained objects. This is useful for very quickly tearing down a
        // container built into scratch memory.
        _begin = _end = _capacity_end.first() = nullptr;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::swap(this_type &x) noexcept {
        DoSwap(x);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Buffer<T, Alignment, Allocator>::pointer
    Buffer<T, Alignment, Allocator>::do_realloc(size_type *newCapacity, const T *first, const T *last) {
        // Allocate the new memory block. The actual capacity assigned is returned in newCapacity.
        T *const p = do_allocate(newCapacity);

        std::memcpy(static_cast<void *>(p),
                    static_cast<const void *>(first),
                    static_cast<size_type>(last - first) * sizeof(T));

        return p;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void Buffer<T, Alignment, Allocator>::DoInit(Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to initialize a Buffer larger than can fit in a size_type!");

        auto count = static_cast<size_type>(n);
        size_type nAllocatedCapacity = count;

        // Allocate raw memory. do_allocate updates nAllocatedCapacity with actual size.
        _begin = do_allocate(&nAllocatedCapacity);
        _capacity_end.first() = _begin + nAllocatedCapacity;
        _end = _begin + count;

        if (count > 0) {
            // For trivial types, value-initialization is just a plain assignment.
            // The compiler will splat 'value' into SIMD registers for high-speed filling.
            const T val = static_cast<T>(value);
            for (size_type i = 0; i < count; ++i) {
                _begin[i] = val;
            }
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Buffer<T, Alignment, Allocator>::DoInit(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoInitFromIterator(first, last, IC());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Buffer<T, Alignment, Allocator>::DoInitFromIterator(InputIterator first, InputIterator last,
                                                                    std::input_iterator_tag) {
        // To do: Use emplace_back instead of append(). Our emplace_back will work below without any ifdefs.
        for (; first != last; ++first)
            // InputIterators by definition actually only allow you to iterate through them once.
            append(*first); // Thus the standard *requires* that we do this (inefficient) implementation.
    } // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.


    template<typename T, size_t Alignment, typename Allocator>
    template<typename ForwardIterator>
    inline void Buffer<T, Alignment, Allocator>::DoInitFromIterator(ForwardIterator first, ForwardIterator last,
                                                                    std::forward_iterator_tag) {
        const auto d = std::distance(first, last);

        container_internal::AssertValueFitsInType<size_type>(
            d, "Attempting to initialize a Buffer larger than can fit in a size_type!");

        const auto n = static_cast<size_type>(d);
        auto nn = n;
        _begin = do_allocate(&nn);
        _capacity_end.first() = _begin + nn;
        _end = _begin + n;

        std::copy(first, last, _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer, bool bMove>
    inline void Buffer<T, Alignment, Allocator>::DoAssign(Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to assign more values than can fit in a size_type!");
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator, bool bMove>
    inline void Buffer<T, Alignment, Allocator>::DoAssign(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, bMove>(first, last, IC());
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::DoAssignValues(size_type n, const value_type &value) {
        const auto nCapacity = static_cast<size_type>(_capacity_end.first() - _begin);

        if (n > nCapacity) {
            // --- Path 1: Insufficient capacity ---
            // For trivial types, we don't need to preserve old data.
            // Just free and reallocate to the exact or grown size.
            size_type nNewCapacity = n;
            const auto pNewData = do_allocate(&nNewCapacity);

            // Skip std::destroy for old elements as they are trivial
            do_free(_begin, nCapacity);

            _begin = pNewData;
            _end = _begin + n;
            _capacity_end.first() = _begin + nNewCapacity;

            // Perform bulk fill on the new memory
            for (size_type i = 0; i < n; ++i) {
                _begin[i] = value;
            }
        } else {
            // --- Path 2: Fits in current capacity ---
            // No need to distinguish between std::fill and uninitialized_fill.
            // Direct assignment is sufficient for the entire range [0, n).
            for (size_type i = 0; i < n; ++i) {
                _begin[i] = value;
            }

            // Simply update the end pointer. No destruction needed for the tail.
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator, bool bMove>
    void Buffer<T, Alignment, Allocator>::DoAssignFromIterator(InputIterator first, InputIterator last,
                                                               std::input_iterator_tag) {
        iterator position(_begin);

        while ((position != _end) && (first != last)) {
            *position = *first;
            ++first;
            ++position;
        }
        if (first == last)
            erase(position, _end);
        else
            insert(_end, first, last);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename RandomAccessIterator, bool bMove>
    void Buffer<T, Alignment, Allocator>::DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last,
                                                               std::random_access_iterator_tag) {
        const auto d = std::distance(first, last);
        container_internal::AssertValueFitsInType<size_type>(d, "Size overflow");
        const auto n = static_cast<size_type>(d);

        const auto nCapacity = static_cast<size_type>(_capacity_end.first() - _begin);

        if (n > nCapacity) {
            // --- Path 1: Expansion needed ---
            auto nNewCapacity = n;
            // For trivial types, we don't need to preserve old data during assign,
            // so we can just allocate new and free old.
            // Or use do_realloc if it's optimized for pure allocation + copy.
            const auto pNewData = do_allocate(&nNewCapacity);

            // Copy new data directly into the fresh buffer
            std::copy(first, last, pNewData);

            // No need to destroy trivial elements in old buffer
            do_free(_begin, nCapacity);

            _begin = pNewData;
            _end = _begin + n;
            _capacity_end.first() = _begin + nNewCapacity;
        } else {
            // --- Path 2: Fits in current capacity ---
            // For trivial types, "uninitialized" memory is just raw memory.
            // We can treat [begin, end) and [end, capacity_end) as the same.
            std::copy(first, last, _begin);

            // Simply update the end pointer. No destruction needed for the "tail".
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void Buffer<T, Alignment, Allocator>::DoInsert(const_iterator position, Integer n, Integer value,
                                                          std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to insert more elements than can can fit in size_type!");

        DoInsertValues(position, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Buffer<T, Alignment, Allocator>::DoInsert(const_iterator position, InputIterator first,
                                                          InputIterator last,
                                                          std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoInsertFromIterator(position, first, last, IC());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Buffer<T, Alignment, Allocator>::DoInsertFromIterator(const_iterator position, InputIterator first,
                                                                      InputIterator last, std::input_iterator_tag) {
        for (; first != last; ++first, ++position)
            position = insert(position, *first);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename BidirectionalIterator>
    void Buffer<T, Alignment, Allocator>::DoInsertFromIterator(const_iterator position, BidirectionalIterator first,
                                                               BidirectionalIterator last,
                                                               std::bidirectional_iterator_tag) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Buffer::insert -- invalid position";

        if (first == last) return;

        auto destPosition = const_cast<value_type *>(position);
        const auto d = std::distance(first, last);
        container_internal::AssertValueFitsInType<size_type>(d, "Size overflow");
        const auto n = static_cast<size_type>(d);

        const auto nPosIndex = static_cast<size_type>(destPosition - _begin);
        const auto nFreeSpace = static_cast<size_type>(_capacity_end.first() - _end);

        if (n <= nFreeSpace) {
            // --- In-place Path ---
            const auto nSuffixSize = static_cast<size_type>(_end - destPosition);

            if (nSuffixSize > 0) {
                // Shift suffix to the right by n positions.
                // Use memmove because source and destination overlap.
                std::memmove(static_cast<void *>(destPosition + n),
                             static_cast<const void *>(destPosition),
                             nSuffixSize * sizeof(T));
            }

            // Copy new data into the created gap.
            std::copy(first, last, destPosition);
            _end += n;
        } else {
            // --- Reallocation Path ---
            const auto nPrevSize = static_cast<size_type>(_end - _begin);
            const auto nGrowCapacity = GetNewCapacity(nPrevSize);
            KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) << "Size overflow";

            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            const auto pNewData = do_allocate(&nNewCapacity);

            // 1. Relocate prefix: [begin, position)
            if (nPosIndex > 0) {
                std::memcpy(static_cast<void *>(pNewData),
                            static_cast<const void *>(_begin),
                            nPosIndex * sizeof(T));
            }

            // 2. Copy source range into the middle: [first, last)
            std::copy(first, last, pNewData + nPosIndex);

            // 3. Relocate suffix: [position, end)
            const size_type nSuffixSize = nPrevSize - nPosIndex;
            if (nSuffixSize > 0) {
                std::memcpy(static_cast<void *>(pNewData + nPosIndex + n),
                            static_cast<const void *>(_begin + nPosIndex),
                            nSuffixSize * sizeof(T));
            }

            // 4. Cleanup old memory without calling destructors.
            do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewData + nPrevSize + n;
            _capacity_end.first() = pNewData + nNewCapacity;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment,
        Allocator>::DoInsertValues(const_iterator position, size_type n, const value_type &value) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Buffer::insert -- invalid position";

        if (n == 0) return;

        auto destPosition = const_cast<value_type *>(position);
        const auto nPosIndex = static_cast<size_type>(destPosition - _begin);
        const auto nFreeSpace = static_cast<size_type>(_capacity_end.first() - _end);

        if (n <= nFreeSpace) {
            // --- In-place Path ---
            const value_type temp = value; // Capture value in case it's from within the buffer
            const auto num_to_move = static_cast<size_type>(_end - destPosition);

            if (num_to_move > 0) {
                // Shift existing elements to the right to make room for n new elements
                std::memmove(static_cast<void *>(destPosition + n),
                             static_cast<const void *>(destPosition),
                             num_to_move * sizeof(T));
            }

            // Fill the created gap with the value
            for (size_type i = 0; i < n; ++i) {
                destPosition[i] = temp;
            }
            _end += n;
        } else {
            // --- Reallocation Path ---
            const auto nPrevSize = static_cast<size_type>(_end - _begin);
            const size_type nGrowCapacity = GetNewCapacity(nPrevSize);

            KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) << "Size overflow";

            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            const auto pNewData = do_allocate(&nNewCapacity);

            // 1. Relocate the prefix [begin, position)
            if (nPosIndex > 0) {
                std::memcpy(static_cast<void *>(pNewData),
                            static_cast<const void *>(_begin),
                            nPosIndex * sizeof(T));
            }

            // 2. Fill the new n elements at pNewData + nPosIndex
            pointer pInsertPos = pNewData + nPosIndex;
            for (size_type i = 0; i < n; ++i) {
                pInsertPos[i] = value;
            }

            // 3. Relocate the suffix [position, end)
            const size_type nSuffixSize = nPrevSize - nPosIndex;
            if (nSuffixSize > 0) {
                std::memcpy(static_cast<void *>(pInsertPos + n),
                            static_cast<const void *>(_begin + nPosIndex),
                            nSuffixSize * sizeof(T));
            }

            // 4. Free old storage. No destruction required for trivial types.
            do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewData + nPrevSize + n;
            _capacity_end.first() = pNewData + nNewCapacity;
        }
    }

    // This function exists because set_capacity() currently indirectly requires value_type to be default-constructible,
    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::DoClearCapacity() {
        // and some functions that need to clear our capacity (e.g. operator=) aren't supposed to require default-constructibility.
        clear();
        this_type temp(std::move(*this)); // This is the simplest way to accomplish this,
        swap(temp); // and it is as efficient as any other.
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::DoGrow(size_type newCapacity) {
        // Allocate new memory block
        const auto pNewData = do_allocate(&newCapacity);
        const auto nPrevSize = static_cast<size_type>(_end - _begin);

        if (nPrevSize > 0) {
            // For trivial types, relocation is a simple bitwise copy.
            std::memcpy(static_cast<void *>(pNewData),
                        static_cast<const void *>(_begin),
                        nPrevSize * sizeof(T));
        }

        // Free the old memory block.
        do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

        // Update container_internal state pointers
        _begin = pNewData;
        _end = pNewData + nPrevSize;
        _capacity_end.first() = pNewData + newCapacity;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Buffer<T, Alignment, Allocator>::DoSwap(this_type &x) noexcept {
        std::swap(_begin, x._begin);
        std::swap(_end, x._end);
        std::swap(_capacity_end.first(), x._capacity_end.first());
    }

    // The code duplication between this and the version that takes no value argument and default constructs the values
    // is unfortunate but not easily resolved without relying on C++11 perfect forwarding.
    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::do_insert_values_end(size_type n, const value_type &value) {
        const auto nPrevSize = static_cast<size_type>(_end - _begin);
        const auto nFreeSpace = static_cast<size_type>(_capacity_end.first() - _end);

        if (n > nFreeSpace) {
            // --- Reallocation Path ---
            const size_type nGrowCapacity = GetNewCapacity(nPrevSize);
            KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) << "Size overflow";

            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            const auto pNewData = do_allocate(&nNewCapacity);

            // 1. Move existing data: Trivial move is just a memcpy
            if (nPrevSize > 0) {
                std::memcpy(static_cast<void *>(pNewData), static_cast<const void *>(_begin), nPrevSize * sizeof(T));
            }

            // 2. Fill new data: Use a simple loop for value fill (compiler will optimize this)
            pointer pInsertPos = pNewData + nPrevSize;
            for (size_type i = 0; i < n; ++i) {
                pInsertPos[i] = value;
            }

            // 3. Free old memory
            do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

            // 4. Update pointers
            _begin = pNewData;
            _end = pNewData + nPrevSize + n;
            _capacity_end.first() = pNewData + nNewCapacity;
        } else {
            // --- In-place Path ---
            // Just fill the existing free space
            for (size_type i = 0; i < n; ++i) {
                _end[i] = value;
            }
            _end += n;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::do_insert_values_end(size_type n) {
        const auto nPrevSize = static_cast<size_type>(_end - _begin);
        const auto nFreeSpace = static_cast<size_type>(_capacity_end.first() - _end);
        if (n > nFreeSpace) {
            // --- Reallocation Path ---
            const size_type nGrowCapacity = GetNewCapacity(nPrevSize);
            KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) << "Size overflow";

            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            const auto pNewData = do_allocate(&nNewCapacity);

            // 1. Relocate existing data using memcpy (Trivial move is a bitwise copy)
            if (nPrevSize > 0) {
                std::memcpy(static_cast<void *>(pNewData), static_cast<const void *>(_begin), nPrevSize * sizeof(T));
            }

            // 2. Zero-initialize the newly added elements
            std::memset(static_cast<void *>(pNewData + nPrevSize), 0, n * sizeof(T));

            // 3. Free old memory without calling destructors (T is trivial)
            do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

            // 4. Update buffer state
            _begin = pNewData;
            _end = pNewData + nPrevSize + n;
            _capacity_end.first() = pNewData + nNewCapacity;
        } else {
            // --- In-place Path ---
            // Efficiently zero-initialize trailing memory
            std::memset(static_cast<void *>(_end), 0, n * sizeof(T));
            _end += n;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    void Buffer<T, Alignment, Allocator>::do_insert_value(const_iterator position, Args &&... args) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Buffer::insert/emplace -- invalid position";

        auto destPosition = const_cast<value_type *>(position);
        const auto nPosIndex = static_cast<size_type>(destPosition - _begin);

        if (_end != _capacity_end.first()) {
            // --- In-place Path ---
            // 1. Capture the value first in case it's a reference to an element inside this buffer.
            // For trivial types, this is just a stack copy.
            value_type value(std::forward<Args>(args)...);

            // 2. Shift elements to the right. Use memmove because ranges overlap.
            const auto num_to_move = static_cast<size_type>(_end - destPosition);
            if (num_to_move > 0) {
                std::memmove(static_cast<void *>(destPosition + 1),
                             static_cast<const void *>(destPosition),
                             num_to_move * sizeof(T));
            }

            // 3. Simple assignment is sufficient for trivial types.
            *destPosition = value;
            ++_end;
        } else {
            // --- Reallocation Path ---
            const auto nPrevSize = static_cast<size_type>(_end - _begin);
            size_type nNewCapacity = GetNewCapacity(nPrevSize);
            const auto pNewData = do_allocate(&nNewCapacity);

            // 1. Construct the new value at the target index in the new buffer.
            // Using placement new here to handle the variadic args correctly.
            ::new(static_cast<void *>(pNewData + nPosIndex)) T(std::forward<Args>(args)...);

            // 2. Copy the prefix: [begin, position)
            if (nPosIndex > 0) {
                std::memcpy(static_cast<void *>(pNewData),
                            static_cast<const void *>(_begin),
                            nPosIndex * sizeof(T));
            }

            // 3. Copy the suffix: [position, end)
            const size_type nSuffixSize = nPrevSize - nPosIndex;
            if (nSuffixSize > 0) {
                std::memcpy(static_cast<void *>(pNewData + nPosIndex + 1),
                            static_cast<const void *>(_begin + nPosIndex),
                            nSuffixSize * sizeof(T));
            }

            // 4. Free old memory. No destruction needed for trivial types.
            do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewData + nPrevSize + 1;
            _capacity_end.first() = pNewData + nNewCapacity;
        }
    }

    // assumes _end == _capacity_end.first(), ie. create a new array and move existing elements into it while inserting the new element at the end.
    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    void Buffer<T, Alignment, Allocator>::do_insert_value_end(Args &&... args) {
        const auto nPrevSize = static_cast<size_type>(_end - _begin);
        size_type nNewCapacity = GetNewCapacity(nPrevSize);
        const auto pNewData = do_allocate(&nNewCapacity);

        // 1. Construct the new element first at the new tail.
        // This is safe even if 'args' references elements in the old [_begin, _end) range.
        ::new(static_cast<void *>(pNewData + nPrevSize)) T(std::forward<Args>(args)...);

        // 2. Relocate existing trivial data using memcpy.
        if (nPrevSize > 0) {
            std::memcpy(static_cast<void *>(pNewData),
                        static_cast<const void *>(_begin),
                        nPrevSize * sizeof(T));
        }

        // 3. Free old storage. Destructors are skipped for trivial types.
        do_free(_begin, static_cast<size_type>(_capacity_end.first() - _begin));

        // 4. Update container_internal pointers.
        _begin = pNewData;
        _end = pNewData + nPrevSize + 1;
        _capacity_end.first() = pNewData + nNewCapacity;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool Buffer<T, Alignment, Allocator>::validate() const noexcept {
        if (_end < _begin)
            return false;
        if (_capacity_end.first() < _end)
            return false;
        return true;
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Buffer<T, Alignment, Allocator>::bestow(T *data, size_type size, size_type capacity) noexcept {
        // 1. Pre-condition: Buffer must be empty to avoid accidental data loss.
        if (TURBO_UNLIKELY(_begin != nullptr || data == nullptr)) {
            KCHECK(_begin == nullptr) << "Buffer::bestow -- buffer is not empty";
            return;
        }
        if constexpr (Alignment != 0) {
            // 2. Alignment check: Ensure the external pointer meets hardware/SIMD requirements.
            KCHECK(reinterpret_cast<uintptr_t>(data) % Alignment == 0)
                << "Buffer::bestow -- pointer is not aligned to " << Alignment;
        }

        // 3. Ownership transfer: Assign external pointers to internal state.
        _begin = data;
        _end = data + size;
        _capacity_end.first() = data + capacity;
    }

    template<typename T, size_t Alignment, typename Allocator>
    T *Buffer<T, Alignment, Allocator>::seize(size_type *out_size, size_type *out_capacity) noexcept {
        if (_begin == nullptr) {
            return nullptr;
        }
        T *raw_ptr = _begin;

        if (out_size) *out_size = static_cast<size_type>(_end - _begin);
        if (out_capacity) *out_capacity = static_cast<size_type>(_capacity_end.first() - _begin);

        // Reset buffer to a safe, empty state without freeing memory
        _begin = nullptr;
        _end = nullptr;
        _capacity_end.first() = nullptr;

        return raw_ptr;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator==(const Buffer<T, Alignment, Allocator> &a, const Buffer<T, Alignment, Allocator> &b) {
        return ((a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin()));
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator!=(const Buffer<T, Alignment, Allocator> &a, const Buffer<T, Alignment, Allocator> &b) {
        return ((a.size() != b.size()) || !std::equal(a.begin(), a.end(), b.begin()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator<(const Buffer<T, Alignment, Allocator> &a, const Buffer<T, Alignment, Allocator> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator>(const Buffer<T, Alignment, Allocator> &a, const Buffer<T, Alignment, Allocator> &b) {
        return b < a;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator<=(const Buffer<T, Alignment, Allocator> &a, const Buffer<T, Alignment, Allocator> &b) {
        return !(b < a);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator>=(const Buffer<T, Alignment, Allocator> &a, const Buffer<T, Alignment, Allocator> &b) {
        return !(a < b);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void swap(Buffer<T, Alignment, Allocator> &a, Buffer<T, Alignment, Allocator> &b) noexcept {
        a.swap(b);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase / erase_if
    //
    // https://en.cppreference.com/w/cpp/container/Buffer/erase2
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class U>
    typename Buffer<T, Alignment, Allocator>::size_type erase(Buffer<T, Alignment, Allocator> &c, const U &value) {
        // Erases all elements that compare equal to value from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove(c.begin(), origEnd, value);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);

        return static_cast<typename Buffer<T, Alignment, Allocator>::size_type>(numRemoved);
    }

    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename Buffer<T, Alignment, Allocator>::size_type erase_if(Buffer<T, Alignment, Allocator> &c,
                                                                 Predicate predicate) {
        // Erases all elements that satisfy the predicate pred from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove_if(c.begin(), origEnd, predicate);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);


        return static_cast<typename Buffer<T, Alignment, Allocator>::size_type>(numRemoved);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase_unsorted
    //
    // This serves a similar purpose as erase above but with the difference
    // that it doesn't preserve the relative order of what is left in the
    // Buffer.
    //
    // Effects: Removes all elements equal to value from the Buffer while
    // optimizing for speed with the potential reordering of elements as a
    // side effect.
    //
    // Complexity: Linear
    //
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class U>
    typename Buffer<T, Alignment, Allocator>::size_type erase_unsorted(Buffer<T, Alignment, Allocator> &c,
                                                                       const U &value) {
        auto itRemove = c.begin();
        auto ritMove = c.rbegin();

        while (true) {
            // Find the next element that needs to be removed from the front
            itRemove = std::find(itRemove, ritMove.base(), value);
            if (itRemove == ritMove.base())
                break;

            // Find the next element that should NOT be removed from the back
            ritMove = std::find_if(ritMove, std::make_reverse_iterator(itRemove),
                                   [&value](const T &elem) { return !(elem == value); });

            if (itRemove == ritMove.base())
                break;

            // For trivial types, simple assignment is as fast as move.
            // We overwrite the 'to-be-removed' slot with a 'valid' element from the tail.
            *itRemove = *ritMove;

            ++itRemove;
            ++ritMove;
        }

        // Number of elements to be removed is the distance from the new end to the old end.
        auto numRemoved = std::distance(itRemove, c.end());

        // Destructors are no-ops for trivial types, so we simply adjust the tail pointer.
        c._end = itRemove;

        return static_cast<typename Buffer<T, Alignment, Allocator>::size_type>(numRemoved);
    }

    ///////////////////////////////////////////////////////////////////////
    // erase_unsorted_if
    //
    // This serves a similar purpose as erase_if above but with the
    // difference that it doesn't preserve the relative order of what is
    // left in the Buffer.
    //
    // Effects: Removes all elements that return true for the predicate
    // while optimizing for speed with the potential reordering of elements
    // as a side effect.
    //
    // Complexity: Linear
    //
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename Buffer<T, Alignment, Allocator>::size_type erase_unsorted_if(
        Buffer<T, Alignment, Allocator> &c, Predicate predicate) {
        auto itRemove = c.begin();
        auto ritMove = c.rbegin();

        while (true) {
            // Find the first element from the front that matches the predicate
            itRemove = std::find_if(itRemove, ritMove.base(), predicate);
            if (itRemove == ritMove.base())
                break;

            // Find the first element from the back that does NOT match the predicate
            ritMove = std::find_if(ritMove, std::make_reverse_iterator(itRemove),
                                   [&predicate](const T &elem) { return !predicate(elem); });

            if (itRemove == ritMove.base())
                break;

            // Overwrite the element to be removed with a valid one from the tail
            *itRemove = *ritMove;

            ++itRemove;
            ++ritMove;
        }

        // Calculate how many elements were logically removed
        auto numRemoved = std::distance(itRemove, c.end());

        // Update the end of the buffer. No destruction is necessary for trivial types.
        c._end = itRemove;

        return static_cast<typename Buffer<T, Alignment, Allocator>::size_type>(numRemoved);
    }

    template<size_t Alignment>
    struct is_contiguous_string_visitor<Buffer<char, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };


    template<typename T, size_t Alignment, typename SizeType>
    struct BufferReference {
        struct Range {
            SizeType offset{0};
            SizeType length{0};
        };

        /// non nullptr, must setting
        std::shared_ptr<Buffer<T, Alignment> > buffer;
        Range view;

        [[nodiscard]] bool is_full_view() const {
            return view.offset == 0 && view.length == buffer->size();
        }

        [[nodiscard]] SizeType offset() const {
            return view.offset;
        }

        [[nodiscard]] SizeType size() const {
            return view.length;
        }

        [[nodiscard]] SizeType capacity() const {
            return buffer->size();
        }

        [[nodiscard]] bool is_unique() const {
            return buffer.use_count() == 1;
        }

        [[nodiscard]] bool is_mutable() const {
            return buffer.use_count() == 1;
        }

        [[nodiscard]] bool next(void **out, int64_t *size) {
            DKCHECK(buffer.use_count() == 1);
            if (view.length == buffer->size()) {
                return false;
            }
            auto bs = buffer->size();
            *out = buffer->data() + view.length;
            *size = bs - view.length;
            view.length = bs;
            return true;
        }

       [[nodiscard]] int64_t backup(int64_t n) {
            DKCHECK(buffer.use_count() == 1);
            if (n >= view.length) {
                auto r = n - view.length;
                view.length = 0;
                return r;
            }
            view.length -= n;
            return 0;
        }

        [[nodiscard]] std::pair<size_t, size_t> append(const void *data, size_t size) {
            DKCHECK(buffer.use_count() == 1);
            auto bs = buffer->size();

            auto av = bs - view.length;
            if (size < av) {
                std::memcpy(view.data() + view.length, data, size);
                view.length += size;
                return {size, av - size};
            }
            std::memcpy(view.data() + view.length, data, av);
            view.length += av;
            return {av, 0};
        }

        void tidy_write_able() {
            DKCHECK(buffer.use_count() == 1);
            auto bs = buffer->size();
            if (view.offset != 0) {
                std::memcpy(view.data(), buffer->data() + view.offset, view.length);
                view.offset = 0;
            }
        }
    };
} // namespace fermat
