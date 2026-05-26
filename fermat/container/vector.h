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
#include <turbo/strings/str_format.h>
#include <fermat/memory/object_pool.h>
#include <fermat/container/construct.h>
#include <fermat/container/utility.h>
#include <fermat/container/traits.h>
#include <fermat/container/compressed_pair.h>
#include <fermat/memory/allocator.h>

namespace fermat {
    /// VectorBase
    ///
    /// The reason we have a VectorBase class is that it makes exception handling
    /// simpler to implement because memory allocation is implemented entirely
    /// in this class. If a user creates a Vector which needs to allocate
    /// memory in the constructor, VectorBase handles it. If an exception is thrown
    /// by the allocator then the exception throw jumps back to the user code and
    /// no try/catch code need be written in the Vector or VectorBase constructor.
    /// If an exception is thrown in the Vector (not VectorBase) constructor, the
    /// destructor for VectorBase will be called automatically (and free the allocated
    /// memory) before the execution jumps back to the user code.
    /// However, if the Vector class were to handle both allocation and initialization
    /// then it would have no choice but to implement an explicit try/catch statement
    /// for all pathways that allocate memory. This increases code size and decreases
    /// performance and makes the code a little harder read and maintain.
    ///
    /// The C++ standard (15.2 paragraph 2) states:
    ///    "An object that is partially constructed or partially destroyed will
    ///     have destructors executed for all its fully constructed subobjects,
    ///     that is, for subobjects for which the constructor has been completed
    ///     execution and the destructor has not yet begun execution."
    ///
    /// The C++ standard (15.3 paragraph 11) states:
    ///    "The fully constructed base classes and members of an object shall
    ///     be destroyed before entering the handler of a function-try-block
    ///     of a constructor or destructor for that block."
    ///
    template<typename T, size_t Alignment, typename Allocator>
    struct VectorBase {
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
        compressed_pair<T *, Allocator> _capacity_end{nullptr, allocator_type{}};

    public:
        VectorBase(const allocator_type &allocator = allocator_type{});

        VectorBase(size_type n, const allocator_type &allocator = allocator_type{});

        virtual ~VectorBase();

    protected:
        T *do_allocate(size_type *n);

        size_type do_allocate_size(size_type n);

        void do_free(T *p, size_type n);

        void uninitialized_n(size_t n);
    }; // VectorBase


    /// Vector
    ///
    /// Implements a dynamic array.
    ///
    template<typename T, size_t Alignment = 0, typename Allocator = BasicAllocator<T, Alignment> >
    class Vector : public VectorBase<T, Alignment, Allocator> {
        typedef VectorBase<T, Alignment, Allocator> base_type;
        typedef Vector<T, Alignment, Allocator> this_type;

        template<class T2, class Allocator2, class U>
        friend typename Vector<T2, Alignment>::size_type erase_unsorted(Vector<T2, Alignment> &c, const U &value);

        template<class T2, class Allocator2, class P>
        friend typename Vector<T2, Alignment>::size_type erase_unsorted_if(Vector<T2, Alignment> &c, P predicate);

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

        static_assert(!std::is_const<value_type>::value, "Vector<T> value_type must be non-const.");
        static_assert(!std::is_volatile<value_type>::value, "Vector<T> value_type must be non-volatile.");

    public:
        Vector() noexcept;

        Vector(const allocator_type &allocator) noexcept;

        explicit Vector(size_type n, const allocator_type &allocator = allocator_type{});

        Vector(size_type n, const value_type &value, const allocator_type &allocator = allocator_type{});

        Vector(const this_type &x, const allocator_type &allocator = allocator_type{});

        Vector(this_type &&x, const allocator_type &allocator = allocator_type{}) noexcept;

        Vector(std::initializer_list<value_type> ilist, const allocator_type &allocator = allocator_type{});

        // note: this has pre-C++11 semantics:
        // this constructor is equivalent to the constructor Vector(static_cast<size_type>(first), static_cast<value_type>(last), allocator) if InputIterator is an integral type.
        template<typename InputIterator>
        Vector(InputIterator first, InputIterator last, const allocator_type &allocator = allocator_type{});

        ~Vector() override;

        this_type &operator=(const this_type &x);

        this_type &operator=(std::initializer_list<value_type> ilist);

        this_type &operator=(this_type &&x) noexcept;

        // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value)

        void swap(this_type &x);

        // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value)

        void assign(size_type n, const value_type &value);

        template<typename InputIterator>
        void assign(InputIterator first, InputIterator last);

        void assign(std::initializer_list<value_type> ilist);

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

        bool empty() const noexcept;

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

        void push_back(const value_type &value);

        reference push_back();

        void *push_back_uninitialized();

        void push_back(value_type &&value);

        void pop_back();

        template<class... Args>
        iterator emplace(const_iterator position, Args &&... args);

        template<class... Args>
        reference emplace_back(Args &&... args);

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

        // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the Vector over the erased position.
        reverse_iterator erase_last(const T &value);

        reverse_iterator erase_last_unsorted(const T &value);

        // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the Vector over the erased position.

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        iterator erase_unsorted(const_iterator position);

        // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the Vector over the erased position.

        reverse_iterator erase(const_reverse_iterator position);

        reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        reverse_iterator erase_unsorted(const_reverse_iterator position);

        void clear() noexcept;

        void reset_lose_memory() noexcept;

        // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        [[nodiscard]] bool validate() const noexcept;

        void bestow(T *data, size_type size, size_type capacity) noexcept;

        T *seize(size_type *size, size_type *capacity) noexcept;

    protected:
        // These functions do the real work of maintaining the Vector. You will notice
        // that many of them have the same name but are specialized on iterator_tag
        // (iterator categories). This is because in these cases there is an optimized
        // implementation that can be had for some cases relative to others. Functions
        // which aren't referenced are neither compiled nor linked into the application.
        template<bool bMove>
        struct should_move_or_copy_tag {
        };

        using should_copy_tag = should_move_or_copy_tag<false>;
        using should_move_tag = should_move_or_copy_tag<true>;

        template<typename ForwardIterator>
        // Allocates a pointer of array count n and copy-constructs it with [first,last).
        pointer do_realloc(size_type *newCapacity, ForwardIterator first, ForwardIterator last, should_copy_tag);

        template<typename ForwardIterator>
        // Allocates a pointer of array count n and copy-constructs it with [first,last).
        pointer do_realloc(size_type *newCapacity, ForwardIterator first, ForwardIterator last, should_move_tag);

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

        void DoInsertValuesEnd(size_type n); // Default constructs n values
        void DoInsertValuesEnd(size_type n, const value_type &value);

        template<typename... Args>
        void DoInsertValue(const_iterator position, Args &&... args);

        template<typename... Args>
        void DoInsertValueEnd(Args &&... args);

        void DoClearCapacity();

        void DoGrow(size_type newCapacity);

        void DoSwap(this_type &x);
    }; // class Vector


    ///////////////////////////////////////////////////////////////////////
    // VectorBase
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline VectorBase<T, Alignment, Allocator>::VectorBase(const allocator_type &allocator) : _capacity_end(
        nullptr, allocator) {
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline VectorBase<T, Alignment,
        Allocator>::VectorBase(size_type n, const allocator_type &allocator) : _capacity_end(nullptr, allocator) {
        uninitialized_n(n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline VectorBase<T, Alignment, Allocator>::~VectorBase() {
        if (_begin) {
            do_free(_begin, _capacity_end.first() - _begin);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline T *VectorBase<T, Alignment, Allocator>::do_allocate(size_type *n) {
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
    inline typename VectorBase<T, Alignment, Allocator>::size_type VectorBase<T, Alignment,
        Allocator>::do_allocate_size(size_type n) {
        if (TURBO_LIKELY(n)) {
            return _capacity_end.second().good_size(n);
        } else {
            return 0;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    void VectorBase<T, Alignment, Allocator>::uninitialized_n(size_t n) {
        _begin = _capacity_end.second().allocate(&n);
        _end = _begin;
        _capacity_end.first() = _begin + n;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void VectorBase<T, Alignment, Allocator>::do_free(T *p, size_type n) {
        if (p) {
            _capacity_end.second().deallocate(p, n);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename VectorBase<T, Alignment, Allocator>::size_type
    VectorBase<T, Alignment, Allocator>::GetNewCapacity(size_type currentSize) {
        // This function must return a value larger than currentSize.
        if (currentSize > 0) {
            if (currentSize < (std::numeric_limits<size_type>::max() / 2)) {
                return 2 * currentSize;
            } else {
                KCHECK(currentSize < std::numeric_limits<size_type>::max()) <<
                                 "Vector growth will overflow the value of the capacity! This is extremely bad!";
                return std::numeric_limits<size_type>::max();
            }
        } else {
            return 1;
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // Vector
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector() noexcept
        : base_type() {
        // Empty
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector(const allocator_type &allocator) noexcept
        : base_type(allocator) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector(size_type n, const allocator_type &allocator)
        : base_type(n, allocator) {
        std::uninitialized_value_construct_n(_begin, n);
        _end = _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector(size_type n, const value_type &value,
                                                   const allocator_type &allocator)
        : base_type(n, allocator) {
        std::uninitialized_fill_n(_begin, n, value);
        _end = _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector(const this_type &x, const allocator_type &allocator)
        : base_type(x.size(), allocator) {
        _end = std::uninitialized_copy(x._begin, x._end, _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector(this_type &&x, const allocator_type &allocator) noexcept
        : base_type(allocator) {
        DoSwap(x);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::Vector(std::initializer_list<value_type> ilist,
                                                   const allocator_type &allocator)
        : base_type(allocator) {
        DoInit(ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline Vector<T, Alignment, Allocator>::Vector(InputIterator first, InputIterator last,
                                                   const allocator_type &allocator)
        : base_type(allocator) {
        DoInit(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline Vector<T, Alignment, Allocator>::~Vector() {
        // Call destructor for the values. Parent class will free the memory.
        std::destroy(_begin, _end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename Vector<T, Alignment, Allocator>::this_type &
    Vector<T, Alignment, Allocator>::operator=(const this_type &x) {
        if (this != &x) // If not assigning to self...
        {
            bool bSlowerPathwayRequired = false;

            if (bSlowerPathwayRequired) {
                DoClearCapacity();
                // Must clear the capacity instead of clear because set_capacity frees our memory, unlike clear.
            }

            DoAssign<const_iterator, false>(x.begin(), x.end(), std::false_type());
        }

        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename Vector<T, Alignment, Allocator>::this_type &
    Vector<T, Alignment, Allocator>::operator=(std::initializer_list<value_type> ilist) {
        typedef typename std::initializer_list<value_type>::iterator InputIterator;
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC());
        // initializer_list has const elements and so we can't move from them.
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename Vector<T, Alignment, Allocator>::this_type &
    Vector<T, Alignment, Allocator>::operator=(this_type &&x) noexcept {
        if (this != &x) {
            DoClearCapacity();
            // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
            swap(x);
            // member swap handles the case that x has a different allocator than our allocator by doing a copy.
        }
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::assign(size_type n, const value_type &value) {
        DoAssignValues(n, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Vector<T, Alignment, Allocator>::assign(InputIterator first, InputIterator last) {
        // It turns out that the C++ std::Vector<int, int> specifies a two argument
        // version of assign that takes (int size, int value). These are not iterators,
        // so we need to do a template compiler trick to do the right thing.
        DoAssign<InputIterator, false>(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::assign(std::initializer_list<value_type> ilist) {
        typedef typename std::initializer_list<value_type>::iterator InputIterator;
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC());
        // initializer_list has const elements and so we can't move from them.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::begin() noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_iterator
    Vector<T, Alignment, Allocator>::begin() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_iterator
    Vector<T, Alignment, Allocator>::cbegin() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::end() noexcept {
        return _end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_iterator
    Vector<T, Alignment, Allocator>::end() const noexcept {
        return _end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_iterator
    Vector<T, Alignment, Allocator>::cend() const noexcept {
        return _end;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::rbegin() noexcept {
        return reverse_iterator(_end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reverse_iterator
    Vector<T, Alignment, Allocator>::rbegin() const noexcept {
        return const_reverse_iterator(_end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reverse_iterator
    Vector<T, Alignment, Allocator>::crbegin() const noexcept {
        return const_reverse_iterator(_end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::rend() noexcept {
        return reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reverse_iterator
    Vector<T, Alignment, Allocator>::rend() const noexcept {
        return const_reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reverse_iterator
    Vector<T, Alignment, Allocator>::crend() const noexcept {
        return const_reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    bool Vector<T, Alignment, Allocator>::empty() const noexcept {
        return (_begin == _end);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::size_type
    Vector<T, Alignment, Allocator>::size() const noexcept {
        return (size_type) (_end - _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::size_type
    Vector<T, Alignment, Allocator>::capacity() const noexcept {
        return (size_type) (_capacity_end.first() - _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::resize(size_type n, const value_type &value) {
        if (n > (size_type) (_end - _begin)) // We expect that more often than not, resizes will be upsizes.
            DoInsertValuesEnd(n - ((size_type) (_end - _begin)), value);
        else {
            std::destroy(_begin + n, _end);
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::resize(size_type n) {
        // Alternative implementation:
        // resize(n, value_type());

        if (n > (size_type) (_end - _begin)) // We expect that more often than not, resizes will be upsizes.
            DoInsertValuesEnd(n - ((size_type) (_end - _begin)));
        else {
            std::destroy(_begin + n, _end);
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::reserve(size_type n) {
        // If the user wants to reduce the reserved memory, there is the set_capacity function.
        if (n > size_type(_capacity_end.first() - _begin)) // If n > capacity ...
            DoGrow(n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::set_capacity(size_type n) {
        if ((n == npos) || (n <= (size_type) (_end - _begin))) // If new capacity <= size...
        {
            if (n == 0) // Very often n will be 0, and clear will be faster than resize and use less stack space.
                clear();
            else if (n < (size_type) (_end - _begin))
                resize(n);

            shrink_to_fit();
        } else {
            // Else new capacity > size.
            auto nn = n;
            pointer const pNewData = do_realloc(&nn, _begin, _end, should_move_tag());
            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end.first() - _begin));

            const ptrdiff_t nPrevSize = _end - _begin;
            _begin = pNewData;
            _end = pNewData + nPrevSize;
            _capacity_end.first() = _begin + nn;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::shrink_to_fit() {
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
    inline typename Vector<T, Alignment, Allocator>::pointer
    Vector<T, Alignment, Allocator>::data() noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_pointer
    Vector<T, Alignment, Allocator>::data() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reference
    Vector<T, Alignment, Allocator>::operator[](size_type n) {
        // We allow the user to use a reference to v[0] of an empty container. But this was merely grandfathered in and ideally we shouldn't allow such access to [0].
        //if (TURBO_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(_end - _begin)))))
        //    KCHECK(false) << "Vector::operator[] -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reference
    Vector<T, Alignment, Allocator>::operator[](size_type n) const {
        //if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
        //    KCHECK(false) << "Vector::operator[] -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reference
    Vector<T, Alignment, Allocator>::at(size_type n) {
        // The difference between at() and operator[] is it signals
        // the requested position is out of range by throwing an
        // out_of_range exception.

        //if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
        //    KCHECK(false) << "Vector::at -- out of range";
        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reference
    Vector<T, Alignment, Allocator>::at(size_type n) const {
        //if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
        //    KCHECK(false) << "Vector::at -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reference
    Vector<T, Alignment, Allocator>::front() {
        //if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            // We don't allow the user to reference an empty container.
        //    KCHECK(false) << "Vector::front -- empty Vector";

        return *_begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reference
    Vector<T, Alignment, Allocator>::front() const {
        //if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            // We don't allow the user to reference an empty container.
       //     KCHECK(false) << "Vector::front -- empty Vector";

        return *_begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reference
    Vector<T, Alignment, Allocator>::back() {
        // if _end is nullptr the expression (_end - 1) is undefined behaviour.
        // any use of back() with an empty Vector is thus conceptually wrong.
        //if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
       //     KCHECK(false) << "Vector::back -- empty Vector";

        return *(_end - 1);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::const_reference
    Vector<T, Alignment, Allocator>::back() const {
        // if _end is nullptr the expression (_end - 1) is undefined behaviour.
        // any use of back() with an empty Vector is thus conceptually wrong.
        //if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
        //    KCHECK(false) << "Vector::back -- empty Vector";

        return *(_end - 1);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::push_back(const value_type &value) {
        if (_end < _capacity_end.first()) {
            construct_at(_end++, value);
        } else {
            DoInsertValueEnd(value);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::push_back(value_type &&value) {
        if (_end < _capacity_end.first())
            construct_at(_end++, std::move(value));
        else
            DoInsertValueEnd(std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reference
    Vector<T, Alignment, Allocator>::push_back() {
        if (_end < _capacity_end.first())
            construct_at(_end++);
        else
            DoInsertValueEnd();

        return *(_end - 1); // Same as return back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void *Vector<T, Alignment, Allocator>::push_back_uninitialized() {
        if (_end == _capacity_end.first()) {
            const size_type nPrevSize = size_type(_end - _begin);
            const size_type nNewCapacity = GetNewCapacity(nPrevSize);
            DoGrow(nNewCapacity);
        }

        return _end++;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::pop_back() {
        //if (TURBO_UNLIKELY(_end <= _begin))
        //    KCHECK(false) << "Vector::pop_back -- empty Vector";


        --_end;
        _end->~value_type();
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<class... Args>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::emplace(const_iterator position, Args &&... args) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.

        if ((_end == _capacity_end.first()) || (position != _end))
            DoInsertValue(position, std::forward<Args>(args)...);
        else {
            construct_at(_end, std::forward<Args>(args)...);
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        }

        return _begin + n;
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<class... Args>
    inline typename Vector<T, Alignment, Allocator>::reference
    Vector<T, Alignment, Allocator>::emplace_back(Args &&... args) {
        if (_end < _capacity_end.first()) {
            construct_at(_end, std::forward<Args>(args)...);
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        } else
            DoInsertValueEnd(std::forward<Args>(args)...);

        return back();
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::insert(const_iterator position, const value_type &value) {
        //if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
        //    KCHECK(false) << "Vector::insert -- invalid position";

        // We implment a quick pathway for the case that the insertion position is at the end and we have free capacity for it.
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.

        if ((_end == _capacity_end.first()) || (position != _end))
            DoInsertValue(position, value);
        else {
            construct_at(_end, value);
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        }

        return _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::insert(const_iterator position, value_type &&value) {
        return emplace(position, std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::insert(const_iterator position, size_type n, const value_type &value) {
        const ptrdiff_t p = position - _begin; // Save this because we might reallocate.
        DoInsertValues(position, n, value);
        return _begin + p;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::insert(const_iterator position, InputIterator first, InputIterator last) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.
        DoInsert(position, first, last, std::is_integral<InputIterator>());
        return _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.
        DoInsert(position, ilist.begin(), ilist.end(), std::false_type());
        return _begin + n;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::erase(const_iterator position) {
        //if (TURBO_UNLIKELY((position < _begin) || (position >= _end)))
        //    KCHECK(false) << "Vector::erase -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if ((position + 1) < _end)
            std::move(destPosition + 1, _end, destPosition);
        --_end;
        _end->~value_type();
        return destPosition;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::erase(const_iterator first, const_iterator last) {
       // if (TURBO_UNLIKELY((first < _begin) || (first > _end) || (last < _begin) || (last > _end) || (last < first)))
      //      KCHECK(false) << "Vector::erase -- invalid position";
        if (first != last) {
            const auto position = const_cast<value_type *>(std::move(
                const_cast<value_type *>(last), const_cast<value_type *>(_end), const_cast<value_type *>(first)));
            std::destroy(position, _end);
            _end -= (last - first);
        }

        return const_cast<value_type *>(first);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::erase_unsorted(const_iterator position) {
        //if (TURBO_UNLIKELY((position < _begin) || (position >= _end)))
        //    KCHECK(false) << "Vector::erase -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);
        *destPosition = std::move(*(_end - 1));

        // pop_back();
        --_end;
        _end->~value_type();

        return destPosition;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator Vector<T, Alignment, Allocator>::erase_first(
        const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        iterator it = std::find(begin(), end(), value);

        if (it != end())
            return erase(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::iterator
    Vector<T, Alignment, Allocator>::erase_first_unsorted(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        iterator it = std::find(begin(), end(), value);

        if (it != end())
            return erase_unsorted(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::erase_last(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        reverse_iterator it = std::find(rbegin(), rend(), value);

        if (it != rend())
            return erase(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::erase_last_unsorted(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        reverse_iterator it = std::find(rbegin(), rend(), value);

        if (it != rend())
            return erase_unsorted(it);
        else
            return it;
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase(last.base(), first.base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename Vector<T, Alignment, Allocator>::reverse_iterator
    Vector<T, Alignment, Allocator>::erase_unsorted(const_reverse_iterator position) {
        return reverse_iterator(erase_unsorted((++position).base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::clear() noexcept {
        std::destroy(_begin, _end);
        _end = _begin;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::reset_lose_memory() noexcept {
        // The reset function is a special extension function which unilaterally
        // resets the container to an empty state without freeing the memory of
        // the contained objects. This is useful for very quickly tearing down a
        // container built into scratch memory.
        _begin = _end = _capacity_end.first() = nullptr;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::swap(this_type &x) {
        DoSwap(x);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename ForwardIterator>
    inline typename Vector<T, Alignment, Allocator>::pointer
    Vector<T, Alignment, Allocator>::do_realloc(size_type *newCapacity, ForwardIterator first, ForwardIterator last,
                                                should_copy_tag) {
        T *const p = do_allocate(newCapacity); // p is of type T* but is not constructed.
        std::uninitialized_copy(first, last, p); // copy-constructs p from [first,last).
        return p;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename ForwardIterator>
    inline typename Vector<T, Alignment, Allocator>::pointer
    Vector<T, Alignment, Allocator>::do_realloc(size_type *newCapacity, ForwardIterator first, ForwardIterator last,
                                                should_move_tag) {
        T *const p = do_allocate(newCapacity); // p is of type T* but is not constructed.
        std::uninitialized_move(first, last, p); // move-constructs p from [first,last).
        return p;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void Vector<T, Alignment, Allocator>::DoInit(Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to initialize a Vector larger than can fit in a size_type!");
        size_type nn = n;
        _begin = do_allocate(&nn);
        _capacity_end.first() = _begin + nn;
        _end = _begin + n;

        std::uninitialized_fill_n(_begin, n, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Vector<T, Alignment, Allocator>::DoInit(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoInitFromIterator(first, last, IC());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Vector<T, Alignment, Allocator>::DoInitFromIterator(InputIterator first, InputIterator last,
                                                                    std::input_iterator_tag) {
        // To do: Use emplace_back instead of push_back(). Our emplace_back will work below without any ifdefs.
        for (; first != last; ++first)
            // InputIterators by definition actually only allow you to iterate through them once.
            push_back(*first); // Thus the standard *requires* that we do this (inefficient) implementation.
    } // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.


    template<typename T, size_t Alignment, typename Allocator>
    template<typename ForwardIterator>
    inline void Vector<T, Alignment, Allocator>::DoInitFromIterator(ForwardIterator first, ForwardIterator last,
                                                                    std::forward_iterator_tag) {
        const auto d = std::distance(first, last);

        container_internal::AssertValueFitsInType<size_type>(
            d, "Attempting to initialize a Vector larger than can fit in a size_type!");

        const size_type n = static_cast<size_type>(d);
        auto nn = n;
        _begin = do_allocate(&nn);
        _capacity_end.first() = _begin + nn;
        _end = _begin + n;

        std::uninitialized_copy(first, last, _begin);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer, bool bMove>
    inline void Vector<T, Alignment, Allocator>::DoAssign(Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to assign more values than can fit in a size_type!");
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator, bool bMove>
    inline void Vector<T, Alignment, Allocator>::DoAssign(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, bMove>(first, last, IC());
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::DoAssignValues(size_type n, const value_type &value) {
        if (n > size_type(_capacity_end.first() - _begin)) // If n > capacity ...
        {
            this_type temp(n, value); // We have little choice but to reallocate with new memory.
            swap(temp);
        } else if (n > size_type(_end - _begin)) // If n > size ...
        {
            std::fill(_begin, _end, value);
            std::uninitialized_fill_n(_end, n - size_type(_end - _begin), value);
            _end += n - size_type(_end - _begin);
        } else // else 0 <= n <= size
        {
            std::fill_n(_begin, n, value);
            erase(_begin + n, _end);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator, bool bMove>
    void Vector<T, Alignment, Allocator>::DoAssignFromIterator(InputIterator first, InputIterator last,
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
    void Vector<T, Alignment, Allocator>::DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last,
                                                               std::random_access_iterator_tag) {
        const auto d = std::distance(first, last);
        container_internal::AssertValueFitsInType<size_type>(
            d, "Attempting to assign more values than can fit in a size_type!");

        const size_type n = static_cast<size_type>(d);
        // If n > capacity ...
        if (n > size_type(_capacity_end.first() - _begin)) {
            auto nn = n;
            pointer const pNewData = do_realloc(&nn, first, last, should_move_or_copy_tag<bMove>());
            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = _begin + n;
            _capacity_end.first() = _begin + nn;
        } else if (n <= size_type(_end - _begin)) // If n <= size ...
        {
            pointer const pNewEnd = std::copy(first, last, _begin);
            // Since we are copying to _begin, we don't have to worry about needing copy_backward or a memmove-like copy (as opposed to memcpy-like copy).
            std::destroy(pNewEnd, _end);
            _end = pNewEnd;
        } else // else size < n <= capacity
        {
            RandomAccessIterator position = first + (_end - _begin);
            std::copy(first, position, _begin);
            // Since we are copying to _begin, we don't have to worry about needing copy_backward or a memmove-like copy (as opposed to memcpy-like copy).
            _end = std::uninitialized_copy(position, last, _end);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void Vector<T, Alignment, Allocator>::DoInsert(const_iterator position, Integer n, Integer value,
                                                          std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to insert more elements than can can fit in size_type!");

        DoInsertValues(position, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Vector<T, Alignment, Allocator>::DoInsert(const_iterator position, InputIterator first,
                                                          InputIterator last,
                                                          std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoInsertFromIterator(position, first, last, IC());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void Vector<T, Alignment, Allocator>::DoInsertFromIterator(const_iterator position, InputIterator first,
                                                                      InputIterator last, std::input_iterator_tag) {
        for (; first != last; ++first, ++position)
            position = insert(position, *first);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename BidirectionalIterator>
    void Vector<T, Alignment, Allocator>::DoInsertFromIterator(const_iterator position, BidirectionalIterator first,
                                                               BidirectionalIterator last,
                                                               std::bidirectional_iterator_tag) {
       // if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
       //     KCHECK(false) << "Vector::insert -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if (first != last) {
            const auto d = std::distance(first, last);
            container_internal::AssertValueFitsInType<size_type>(
                d, "Attempting to insert more elements than can fit in a Vector.");

            const auto n = static_cast<size_type>(d); // n is the number of elements we are inserting.

            if (n <= size_type(_capacity_end.first() - _end)) // If n fits within the existing capacity...
            {
                const auto nExtra = static_cast<size_type>(_end - destPosition);

                if (n < nExtra)
                // If the inserted values are entirely within initialized memory (i.e. are before _end)...
                {
                    std::uninitialized_move(_end - n, _end, _end);
                    std::move_backward(destPosition, _end - n, _end);
                    // We need move_backward because of potential overlap issues.
                    std::copy(first, last, destPosition);
                } else {
                    BidirectionalIterator iTemp = first;
                    std::advance(iTemp, nExtra);
                    std::uninitialized_copy(iTemp, last, _end);
                    std::uninitialized_move(destPosition, _end, _end + n - nExtra);
                    std::copy_backward(first, iTemp, destPosition + nExtra);
                }

                _end += n;
            } else {
                // else we need to expand our capacity.
                const size_type nPrevSize = size_type(_end - _begin);
                const size_type nGrowCapacity = GetNewCapacity(nPrevSize);
                KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) <<
                                 "Size overflow: Attempting to insert more elements than can fit in a Vector.";
                size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
                pointer const pNewData = do_allocate(&nNewCapacity);


                pointer pNewEnd = std::uninitialized_move(_begin, destPosition, pNewData);
                pNewEnd = std::uninitialized_copy(first, last, pNewEnd);
                pNewEnd = std::uninitialized_move(destPosition, _end, pNewEnd);

                std::destroy(_begin, _end);
                do_free(_begin, (size_type) (_capacity_end.first() - _begin));

                _begin = pNewData;
                _end = pNewEnd;
                _capacity_end.first() = pNewData + nNewCapacity;
            }
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment,
        Allocator>::DoInsertValues(const_iterator position, size_type n, const value_type &value) {
        //if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
         //   KCHECK(false) << "Vector::insert -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if (n <= size_type(_capacity_end.first() - _end)) // If n is <= capacity...
        {
            if (n > 0) // To do: See if there is a way we can eliminate this 'if' statement.
            {
                // To consider: Make this algorithm work more like DoInsertValue whereby a pointer to value is used.
                const value_type temp = value;
                const size_type nExtra = static_cast<size_type>(_end - destPosition);

                if (n < nExtra) {
                    std::uninitialized_move(_end - n, _end, _end);
                    std::move_backward(destPosition, _end - n, _end);
                    // We need move_backward because of potential overlap issues.
                    std::fill(destPosition, destPosition + n, temp);
                } else {
                    std::uninitialized_fill_n(_end, n - nExtra, temp);
                    std::uninitialized_move(destPosition, _end, _end + n - nExtra);
                    std::fill(destPosition, _end, temp);
                }

                _end += n;
            }
        } else // else n > capacity
        {
            const size_type nPrevSize = size_type(_end - _begin);
            const size_type nGrowCapacity = GetNewCapacity(nPrevSize);
            KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) <<
                             "Size overflow: Attempting to insert more elements than can fit in a Vector.";
            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            pointer const pNewData = do_allocate(&nNewCapacity);


            pointer pNewEnd = std::uninitialized_move(_begin, destPosition, pNewData);
            std::uninitialized_fill_n(pNewEnd, n, value);
            pNewEnd = std::uninitialized_move(destPosition, _end, pNewEnd + n);

            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end.first() = pNewData + nNewCapacity;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::DoClearCapacity()
    // This function exists because set_capacity() currently indirectly requires value_type to be default-constructible,
    {
        // and some functions that need to clear our capacity (e.g. operator=) aren't supposed to require default-constructibility.
        clear();
        this_type temp(std::move(*this)); // This is the simplest way to accomplish this,
        swap(temp); // and it is as efficient as any other.
    }


    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::DoGrow(size_type newCapacity) {
        pointer const pNewData = do_allocate(&newCapacity);

        pointer pNewEnd = std::uninitialized_move(_begin, _end, pNewData);

        std::destroy(_begin, _end);
        do_free(_begin, (size_type) (_capacity_end.first() - _begin));

        _begin = pNewData;
        _end = pNewEnd;
        _capacity_end.first() = pNewData + newCapacity;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void Vector<T, Alignment, Allocator>::DoSwap(this_type &x) {
        std::swap(_begin, x._begin);
        std::swap(_end, x._end);
        std::swap(_capacity_end.first(), x._capacity_end.first());
    }

    // The code duplication between this and the version that takes no value argument and default constructs the values
    // is unfortunate but not easily resolved without relying on C++11 perfect forwarding.
    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::DoInsertValuesEnd(size_type n, const value_type &value) {
        if (n > size_type(_capacity_end.first() - _end)) {
            const size_type nPrevSize = size_type(_end - _begin);
            const size_type nGrowCapacity = GetNewCapacity(nPrevSize);
            KCHECK(nPrevSize <= std::numeric_limits<size_type>::max() - n) <<
                             "Size overflow: Attempting to insert more elements than can fit in a Vector.";
            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            pointer const pNewData = do_allocate(&nNewCapacity);

            pointer pNewEnd = std::uninitialized_move(_begin, _end, pNewData);

            std::uninitialized_fill_n(pNewEnd, n, value);
            pNewEnd += n;

            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end.first() = pNewData + nNewCapacity;
        } else {
            std::uninitialized_fill_n(_end, n, value);
            _end += n;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::DoInsertValuesEnd(size_type n) {
        if (n > size_type(_capacity_end.first() - _end)) {
            const size_type nPrevSize = size_type(_end - _begin);
            const size_type nGrowCapacity = GetNewCapacity(nPrevSize);
            KCHECK(
                nPrevSize <= std::numeric_limits<size_type>::max() -
                n) << "Size overflow: Attempting to insert more elements than can fit in a Vector.";
            size_type nNewCapacity = std::max(nGrowCapacity, nPrevSize + n);
            pointer const pNewData = do_allocate(&nNewCapacity);


            pointer pNewEnd = std::uninitialized_move(_begin, _end, pNewData);

            std::uninitialized_value_construct_n(pNewEnd, n);
            pNewEnd += n;

            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end.first() = pNewData + nNewCapacity;
        } else {
            std::uninitialized_value_construct_n(_end, n);
            _end += n;
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    void Vector<T, Alignment, Allocator>::DoInsertValue(const_iterator position, Args &&... args) {
       // if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
       //     KCHECK(false) << "Vector::insert/emplace -- invalid position";

        iterator destPosition = const_cast<value_type *>(position);

        if (_end != _capacity_end.first()) {
            /// Insertion with spare capacity
            KCHECK(position < _end);

            /// Create a temporary copy (handles self-referential arguments)
            value_type value(std::forward<Args>(args)...);

            /// Move-construct the last element to the uninitialized `_end` position
            construct_at(_end, std::move(*(_end - 1)));

            /// Shift the range [destPosition, _end-1) one step to the right
            std::move_backward(destPosition, _end - 1, _end);

            /// Destroy the original element at destPosition
            fermat::destroy_at(destPosition);

            /// Move-construct the new value into place
            construct_at(destPosition, std::move(value));

            ++_end;
        } else {
            /// No spare capacity: reallocate
            const size_type nPosSize = destPosition - _begin;
            const size_type nPrevSize = _end - _begin;
            size_type nNewCapacity = GetNewCapacity(nPrevSize);
            pointer pNewData = do_allocate(&nNewCapacity);

            /// Construct the new element directly at the insertion point in the new buffer
            construct_at(pNewData + nPosSize, std::forward<Args>(args)...);

            /// Move existing elements before insertion point
            pointer pNewEnd = std::uninitialized_move(_begin, destPosition, pNewData);
            /// Move existing elements after insertion point, one step further
            pNewEnd = std::uninitialized_move(destPosition, _end, pNewEnd + 1);

            /// Destroy old elements and free old memory
            std::destroy(_begin, _end);
            do_free(_begin, (_capacity_end.first() - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end.first() = pNewData + nNewCapacity;
        }
    }

    // assumes _end == _capacity_end, ie. create a new array and move existing elements into it while inserting the new element at the end.
    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    void Vector<T, Alignment, Allocator>::DoInsertValueEnd(Args &&... args) {
        const size_type nPrevSize = size_type(_end - _begin);
        size_type nNewCapacity = GetNewCapacity(nPrevSize);
        pointer const pNewData = do_allocate(&nNewCapacity);


        // Because args... may potentially reference an element (or its sub-object) of this Vector, we need to construct
        // the new element first, prior to moving it (leaving it in an unspecified state) with the call to uninitialized_move.
        construct_at(pNewData + nPrevSize, std::forward<Args>(args)...);
        pointer pNewEnd = std::uninitialized_move(_begin, _end, pNewData);
        pNewEnd++;

        std::destroy(_begin, _end);
        do_free(_begin, (size_type) (_capacity_end.first() - _begin));

        _begin = pNewData;
        _end = pNewEnd;
        _capacity_end.first() = pNewData + nNewCapacity;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool Vector<T, Alignment, Allocator>::validate() const noexcept {
        if (_end < _begin)
            return false;
        if (_capacity_end.first() < _end)
            return false;
        return true;
    }

    template<typename T, size_t Alignment, typename Allocator>
    void Vector<T, Alignment, Allocator>::bestow(T *data, size_type size, size_type capacity) noexcept {
        // 1. Pre-condition: Buffer must be empty to avoid accidental data loss.
        if (TURBO_UNLIKELY(_begin != nullptr || data == nullptr)) {
            KCHECK(_begin == nullptr) << "Buffer::bestow -- buffer is not empty";
            return;
        }

        if constexpr (Alignment != 0) {
            // 2. Alignment check: Ensure the external pointer meets hardware/SIMD requirements.
            KCHECK(reinterpret_cast<uintptr_t>(data) % Alignment == 0)
                << "Vector::bestow -- pointer is not aligned to " << Alignment;
        }
        // 3. Ownership transfer: Assign external pointers to internal state.
        _begin = data;
        _end = data + size;
        _capacity_end.first() = data + capacity;
    }

    template<typename T, size_t Alignment, typename Allocator>
    T *Vector<T, Alignment, Allocator>::seize(size_type *out_size, size_type *out_capacity) noexcept {
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
    inline bool operator==(const Vector<T, Alignment, Allocator> &a, const Vector<T, Alignment, Allocator> &b) {
        return ((a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin()));
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator!=(const Vector<T, Alignment, Allocator> &a, const Vector<T, Alignment, Allocator> &b) {
        return ((a.size() != b.size()) || !std::equal(a.begin(), a.end(), b.begin()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator<(const Vector<T, Alignment, Allocator> &a, const Vector<T, Alignment, Allocator> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator>(const Vector<T, Alignment, Allocator> &a, const Vector<T, Alignment, Allocator> &b) {
        return b < a;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator<=(const Vector<T, Alignment, Allocator> &a, const Vector<T, Alignment, Allocator> &b) {
        return !(b < a);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator>=(const Vector<T, Alignment, Allocator> &a, const Vector<T, Alignment, Allocator> &b) {
        return !(a < b);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void swap(Vector<T, Alignment, Allocator> &a, Vector<T, Alignment, Allocator> &b) noexcept {
        a.swap(b);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase / erase_if
    //
    // https://en.cppreference.com/w/cpp/container/Vector/erase2
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class U>
    typename Vector<T, Alignment, Allocator>::size_type erase(Vector<T, Alignment, Allocator> &c, const U &value) {
        // Erases all elements that compare equal to value from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove(c.begin(), origEnd, value);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);

        return static_cast<typename Vector<T, Alignment, Allocator>::size_type>(numRemoved);
    }

    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename Vector<T, Alignment, Allocator>::size_type erase_if(Vector<T, Alignment, Allocator> &c,
                                                                 Predicate predicate) {
        // Erases all elements that satisfy the predicate pred from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove_if(c.begin(), origEnd, predicate);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);


        return static_cast<typename Vector<T, Alignment, Allocator>::size_type>(numRemoved);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase_unsorted
    //
    // This serves a similar purpose as erase above but with the difference
    // that it doesn't preserve the relative order of what is left in the
    // Vector.
    //
    // Effects: Removes all elements equal to value from the Vector while
    // optimizing for speed with the potential reordering of elements as a
    // side effect.
    //
    // Complexity: Linear
    //
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class U>
    typename Vector<T, Alignment, Allocator>::size_type erase_unsorted(Vector<T, Alignment, Allocator> &c,
                                                                       const U &value) {
        auto itRemove = c.begin();
        auto ritMove = c.rbegin();

        while (true) {
            itRemove = std::find(itRemove, ritMove.base(), value);
            if (itRemove == ritMove.base()) // any elements to remove?
                break;

            ritMove = std::find_if(ritMove, std::make_reverse_iterator(itRemove),
                                   [&value](const T &elem) { return elem != value; });
            if (itRemove == ritMove.base()) // any elements that can be moved into place?
                break;

            *itRemove = std::move(*ritMove);
            ++itRemove;
            ++ritMove;
        }

        // now all elements in the range [itRemove, c.end()) are either to be removed or have already been moved from.

        auto numRemoved = std::distance(itRemove, c.end());

        std::destroy(itRemove, c.end());
        c._end = itRemove;

        return static_cast<typename Vector<T, Alignment, Allocator>::size_type>(numRemoved);
    }

    ///////////////////////////////////////////////////////////////////////
    // erase_unsorted_if
    //
    // This serves a similar purpose as erase_if above but with the
    // difference that it doesn't preserve the relative order of what is
    // left in the Vector.
    //
    // Effects: Removes all elements that return true for the predicate
    // while optimizing for speed with the potential reordering of elements
    // as a side effect.
    //
    // Complexity: Linear
    //
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename Vector<T, Alignment, Allocator>::size_type erase_unsorted_if(
        Vector<T, Alignment, Allocator> &c, Predicate predicate) {
        // Erases all elements that satisfy predicate from the container.
        auto itRemove = c.begin();
        auto ritMove = c.rbegin();

        while (true) {
            itRemove = std::find_if(itRemove, ritMove.base(), predicate);
            if (itRemove == ritMove.base()) // any elements to remove?
                break;

            ritMove = std::find_if(ritMove, std::make_reverse_iterator(itRemove), not_fn(predicate));
            if (itRemove == ritMove.base()) // any elements that can be moved into place?
                break;

            *itRemove = std::move(*ritMove);
            ++itRemove;
            ++ritMove;
        }

        // now all elements in the range [itRemove, c.end()) are either to be removed or have already been moved from.

        auto numRemoved = std::distance(itRemove, c.end());

        std::destroy(itRemove, c.end());
        c._end = itRemove;

        return static_cast<typename Vector<T, Alignment, Allocator>::size_type>(numRemoved);
    }


    template<size_t Alignment>
    struct is_contiguous_string_visitor<Vector<char, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };
} // namespace fermat
