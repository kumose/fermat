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

#include <fermat/container/construct.h>
#include <fermat/memory/allocator.h>
#include <fermat/container/compressed_pair.h>
#include <algorithm>
#include <type_traits>
#include <iterator>
#include <memory>
#include <initializer_list>
#include <new>
#include <cstddef>


namespace fermat {
    /// DequeIterator
    ///
    /// The DequeIterator provides both const and non-const iterators for Deque.
    /// It also is used for the tracking of the begin and end for the Deque.
    ///
    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    struct DequeIterator {
        typedef DequeIterator<T, Pointer, Reference, kDequeSubarraySize> this_type;
        typedef DequeIterator<T, T *, T &, kDequeSubarraySize> iterator;
        typedef DequeIterator<T, const T *, const T &, kDequeSubarraySize> const_iterator;
        typedef ptrdiff_t difference_type;
        typedef std::random_access_iterator_tag iterator_category;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

    public:
        DequeIterator();

        DequeIterator(const iterator &x);

        DequeIterator &operator=(const iterator &x);

        pointer operator->() const;

        reference operator*() const;

        this_type &operator++();

        this_type operator++(int);

        this_type &operator--();

        this_type operator--(int);

        this_type &operator+=(difference_type n);

        this_type &operator-=(difference_type n);

        this_type operator+(difference_type n) const;

        this_type operator-(difference_type n) const;

    protected:
        template<typename, typename, typename, unsigned>
        friend struct DequeIterator;

        template<typename, typename, typename, unsigned>
        friend struct DequeBase;

        template<typename, size_t, typename, typename, typename, unsigned>
        friend class Deque;

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend bool operator==(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &,
                               const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &);

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend bool operator!=(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &,
                               const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &);

        template<typename U, typename PointerU, typename ReferenceU, unsigned kDequeSubarraySizeU>
        friend bool operator!=(const DequeIterator<U, PointerU, ReferenceU, kDequeSubarraySizeU> &a,
                               const DequeIterator<U, PointerU, ReferenceU, kDequeSubarraySizeU> &b);

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend bool operator<(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &,
                              const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &);

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend bool operator>(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &,
                              const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &);

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend bool operator<=(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &,
                               const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &);

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend bool operator>=(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &,
                               const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &);

        template<typename U, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
            kDequeSubarraySizeU>
        friend typename DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU>::difference_type
        operator-(const DequeIterator<U, PointerA, ReferenceA, kDequeSubarraySizeU> &a,
                  const DequeIterator<U, PointerB, ReferenceB, kDequeSubarraySizeU> &b);

    protected:
        T *_current; // Where we currently point. Declared first because it's used most often.
        T *_begin; // The beginning of the current subarray.
        T **_current_array_ptr;
        // Pointer to current subarray. We could alternatively implement this as a list node iterator if the Deque used a linked list.

        struct Increment {
        };

        struct Decrement {
        };

        struct FromConst {
        };

        DequeIterator(T **pCurrentArrayPtr, T *pCurrent);

        DequeIterator(const const_iterator &x, FromConst) : _current(x._current), _begin(x._begin),
                                                            _current_array_ptr(x._current_array_ptr) {
        }

        DequeIterator(const iterator &x, Increment);

        DequeIterator(const iterator &x, Decrement);

        this_type move(const iterator &first, const iterator &last, std::true_type);

        // true means that value_type has the type_trait is_trivially_copyable,
        this_type move(const iterator &first, const iterator &last, std::false_type); // false means it does not.

        void move_backward(const iterator &first, const iterator &last, std::true_type);

        // true means that value_type has the type_trait is_trivially_copyable,
        void move_backward(const iterator &first, const iterator &last, std::false_type); // false means it does not.

        void set_subarray(T **pCurrentArrayPtr);
    };


    /// DequeBase
    ///
    /// The DequeBase implements memory allocation for Deque.
    /// See VectorBase (class vector) for an explanation of why we
    /// create this separate base class.
    ///
    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    struct DequeBase {
        typedef T value_type;
        typedef Allocator allocator_type;
        typedef ArrayAllocator array_allocator_type;
        typedef size_t size_type; // See config.h for the definition of size_t, which defaults to size_t.
        typedef ptrdiff_t difference_type;
        typedef DequeIterator<T, T *, T &, kDequeSubarraySize> iterator;
        typedef DequeIterator<T, const T *, const T &, kDequeSubarraySize> const_iterator;

        static const size_type npos = (size_type) -1; /// 'npos' means non-valid position or simply non-position.
        static const size_type kMaxSize = (size_type) -2;
        /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.

        enum {
            kMinPtrArraySize = 8,
            /// A new empty Deque has a ptrArraySize of 0, but any allocated ptrArrays use this min size.
            kSubarraySize = kDequeSubarraySize ///
            //kNodeSize        = kDequeSubarraySize * sizeof(T)
        };

    protected:
        /// Defines the side of the Deque: front or back.
        enum Side {
            kSideFront, /// Identifies the front side of the Deque.
            kSideBack /// Identifies the back side of the Deque.
        };

        T **_ptr_array; // Array of pointers to subarrays.
        size_type _ptr_array_size; // Possibly we should store this as T** mpArrayEnd.
        iterator _it_begin; // Where within the subarrays is our beginning.
        iterator _it_end; // Where within the subarrays is our end.
        allocator_type _allocator; // To do: Use base class optimization to make this go away.
        array_allocator_type _array_allocator;

    public:
        DequeBase(const allocator_type &allocator);

        DequeBase(size_type n);

        DequeBase(size_type n, const allocator_type &allocator);

        ~DequeBase();

        const allocator_type &get_allocator() const noexcept;

        allocator_type &get_allocator() noexcept;

        void set_allocator(const allocator_type &allocator);

    protected:
        T *do_allocate_subarray();

        void DoFreeSubarray(T *p);

        void DoFreeSubarrays(T **pBegin, T **pEnd);

        T **do_allocate_ptr_array(size_type n);

        void do_free_ptr_array(T **p, size_type n);

        iterator do_realloc_subarray(size_type nAdditionalCapacity, Side allocationSide);

        void DoReallocPtrArray(size_type nAdditionalCapacity, Side allocationSide);

        void do_init(size_type n);
    }; // DequeBase


    /// Deque
    ///
    /// Implements a conventional C++ double-ended queue. The implementation used here
    /// is very much like any other Deque implementations you may have seen, as it
    /// follows the standard algorithm for Deque design.
    ///
    /// Note:
    /// As of this writing, Deque does not support zero-allocation initial emptiness.
    /// A newly created Deque with zero elements will still allocate a subarray
    /// pointer set. We are looking for efficient and clean ways to get around this,
    /// but current efforts have resulted in less efficient and more fragile code.
    /// The logic of this class doesn't lend itself to a clean implementation.
    /// It turns out that deques are one of the least likely classes you'd want this
    /// behaviour in, so until this functionality becomes very important to somebody,
    /// we will leave it as-is. It can probably be solved by adding some extra code to
    /// the Do* functions and adding good comments explaining the situation.
    ///
#define DEQUE_DEFAULT_SUBARRAY_SIZE(T) ((sizeof(T) <= 4) ? 64 : ((sizeof(T) <= 8) ? 32 : ((sizeof(T) <= 16) ? 16 : ((sizeof(T) <= 32) ? 8 : 4))))

    template<typename T, size_t Alignment = 64, typename Policy = TimesPolicy<2, 1>,
        typename Allocator = AlignedAllocator<T, Alignment, Policy>,
        typename ArrayAllocator = AlignedAllocator<T *, Alignment, Policy>,
        unsigned kDequeSubarraySize = 512>
    class Deque : public DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize> {
    public:
        typedef DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize> base_type;
        typedef Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> this_type;
        typedef T value_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
        typedef DequeIterator<T, T *, T &, kDequeSubarraySize> iterator;
        typedef DequeIterator<T, const T *, const T &, kDequeSubarraySize> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef typename base_type::allocator_type allocator_type;

        using base_type::npos;

        static_assert(!std::is_const<value_type>::value, "Deque<T>::value_type must be non-const.");
        static_assert(!std::is_volatile<value_type>::value, "Deque<T>::value_type must be non-volatile.");

    protected:
        using base_type::kSideFront;
        using base_type::kSideBack;
        using base_type::_ptr_array;
        using base_type::_ptr_array_size;
        using base_type::_it_begin;
        using base_type::_it_end;
        using base_type::_allocator;
        using base_type::_array_allocator;
        using base_type::do_allocate_subarray;
        using base_type::DoFreeSubarray;
        using base_type::DoFreeSubarrays;
        using base_type::do_allocate_ptr_array;
        using base_type::do_free_ptr_array;
        using base_type::do_realloc_subarray;
        using base_type::DoReallocPtrArray;

    public:
        Deque();

        explicit Deque(const allocator_type &allocator);

        explicit Deque(size_type n, const allocator_type &allocator = allocator_type{});

        Deque(size_type n, const value_type &value, const allocator_type &allocator = allocator_type{});

        Deque(const this_type &x);

        Deque(this_type &&x) noexcept;

        Deque(this_type &&x, const allocator_type &allocator);

        Deque(std::initializer_list<value_type> ilist, const allocator_type &allocator = allocator_type{});

        // note: this has pre-C++11 semantics:
        // this constructor is equivalent to the constructor Deque(static_cast<size_type>(first), static_cast<value_type>(last)) if InputIterator is an integral type.
        template<typename InputIterator>
        Deque(InputIterator first, InputIterator last);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        ~Deque();

        this_type &operator=(const this_type &x);

        this_type &operator=(std::initializer_list<value_type> ilist);

        this_type &operator=(this_type &&x) noexcept;

        void swap(this_type &x) noexcept;

        void assign(size_type n, const value_type &value);

        void assign(std::initializer_list<value_type> ilist);

        template<typename InputIterator> // It turns out that the C++ std::Deque<int, int> specifies a two argument
        void assign(InputIterator first, InputIterator last);

        // version of assign that takes (int size, int value). These are not
        // iterators, so we need to do a template compiler trick to do the right thing.

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

        void resize(size_type n, const value_type &value);

        void resize(size_type n);

        void shrink_to_fit();

        void set_capacity(size_type n = base_type::npos);

        reference operator[](size_type n);

        const_reference operator[](size_type n) const;

        reference at(size_type n);

        const_reference at(size_type n) const;

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        void push_front(const value_type &value);

        reference push_front();

        void push_front(value_type &&value);

        void push_back(const value_type &value);

        reference push_back();

        void push_back(value_type &&value);

        void pop_front();

        void pop_back();

        template<class... Args>
        iterator emplace(const_iterator position, Args &&... args);

        template<class... Args>
        reference emplace_front(Args &&... args);

        template<class... Args>
        reference emplace_back(Args &&... args);

        iterator insert(const_iterator position, const value_type &value);

        iterator insert(const_iterator position, value_type &&value);

        iterator insert(const_iterator position, size_type n, const value_type &value);

        iterator insert(const_iterator position, std::initializer_list<value_type> ilist);

        // note: this has pre-C++11 semantics:
        // this function is equivalent to insert(const_iterator position, static_cast<size_type>(first), static_cast<value_type>(last)) if InputIterator is an integral type.
        // ie. same as insert(const_iterator position, size_type n, const value_type& value)
        template<typename InputIterator>
        iterator insert(const_iterator position, InputIterator first, InputIterator last);

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        reverse_iterator erase(reverse_iterator position);

        reverse_iterator erase(reverse_iterator first, reverse_iterator last);

        void clear();

        //void reset_lose_memory(); // Disabled until it can be implemented efficiently and cleanly.  // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        bool validate() const;

    protected:
        template<typename Integer>
        void do_init(Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void do_init(InputIterator first, InputIterator last, std::false_type);

        template<typename InputIterator>
        void do_init_from_iterator(InputIterator first, InputIterator last, std::input_iterator_tag);

        template<typename ForwardIterator>
        void do_init_from_iterator(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);

        void do_fill_init(const value_type &value);

        template<typename Integer>
        void do_assign(Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void do_assign(InputIterator first, InputIterator last, std::false_type);

        void do_assign_values(size_type n, const value_type &value);

        template<typename Integer>
        iterator do_insert(const const_iterator &position, Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        iterator do_insert(const const_iterator &position, const InputIterator &first, const InputIterator &last,
                           std::false_type);

        template<typename InputIterator>
        iterator do_insert_from_iterator(const_iterator position, const InputIterator &first, const InputIterator &last,
                                         std::input_iterator_tag);

        template<typename ForwardIterator>
        iterator do_insert_from_iterator(const_iterator position, const ForwardIterator &first,
                                         const ForwardIterator &last, std::forward_iterator_tag);

        iterator do_insert_values(const_iterator position, size_type n, const value_type &value);

        void do_swap(this_type &x);
    }; // class Deque


    ///////////////////////////////////////////////////////////////////////
    // DequeBase
    ///////////////////////////////////////////////////////////////////////

    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::DequeBase(const allocator_type &allocator)
        : _ptr_array(nullptr),
          _ptr_array_size(0),
          _it_begin(),
          _it_end(),
          _allocator(allocator) {
        // It is assumed here that the Deque subclass will init us when/as needed.
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::DequeBase(size_type n)
        : _ptr_array(nullptr),
          _ptr_array_size(0),
          _it_begin(),
          _it_end(),
          _allocator() {
        // It's important to note that do_init creates space for elements and assigns
        // _it_begin/_it_end to point to them, but these elements are not constructed.
        // You need to immediately follow this constructor with code that constructs the values.
        do_init(n);
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::DequeBase(size_type n, const allocator_type &allocator)
        : _ptr_array(nullptr),
          _ptr_array_size(0),
          _it_begin(),
          _it_end(),
          _allocator(allocator) {
        // It's important to note that do_init creates space for elements and assigns
        // _it_begin/_it_end to point to them, but these elements are not constructed.
        // You need to immediately follow this constructor with code that constructs the values.
        do_init(n);
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::~DequeBase() {
        if (_ptr_array) {
            DoFreeSubarrays(_it_begin._current_array_ptr, _it_end._current_array_ptr + 1);
            do_free_ptr_array(_ptr_array, _ptr_array_size);
            _ptr_array = nullptr;
        }
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    const typename DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::allocator_type &
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::get_allocator() const noexcept {
        return _allocator;
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::allocator_type &
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::get_allocator() noexcept {
        return _allocator;
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::set_allocator(const allocator_type &allocator) {
        // The only time you can set an allocator is with an empty unused container, such as right after construction.
        if (TURBO_LIKELY(_allocator != allocator)) {
            // our Deque implementation always has allocations for _ptr_array. this set_allocator() is unlike other container's set_allocator() member function
            // in that it actually frees allocations when assigning the allocator. this lack of consistency is unfortunate.
            if (TURBO_LIKELY(_ptr_array && (_it_end - _it_begin) == 0)) // is the container empty?
            {
                DoFreeSubarrays(_it_begin._current_array_ptr, _it_end._current_array_ptr + 1);
                do_free_ptr_array(_ptr_array, _ptr_array_size);

                _allocator = allocator;
                do_init(0);
            } else {
                throw std::logic_error("Deque::set_allocator -- attempt to change allocator after inserting elements.");
            }
        }
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    T *DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::do_allocate_subarray() {
        T *p = _allocator.allocate(kDequeSubarraySize);
        KCHECK(p != nullptr) << "the behaviour of std::allocators that return nullptr is not defined.";

#if FERMAT_DEBUG
        memset((void *) p, 0, kDequeSubarraySize * sizeof(T));
#endif

        return (T *) p;
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::DoFreeSubarray(T *p) {
        if (p)
            _allocator.deallocate(p, kDequeSubarraySize);
    }

    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::DoFreeSubarrays(T **pBegin, T **pEnd) {
        while (pBegin < pEnd)
            DoFreeSubarray(*pBegin++);
    }

    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    T **DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::do_allocate_ptr_array(size_type n) {
        T **pp = _array_allocator.allocate(n);
        KCHECK(pp != nullptr) << "the behaviour of std::allocators that return nullptr is not defined.";

#if FERMAT_DEBUG
        memset((void *) pp, 0, n * sizeof(T *));
#endif

        return pp;
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::do_free_ptr_array(T **pp, size_type n) {
        if (pp)
            _array_allocator.deallocate(pp, n);
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::do_realloc_subarray(size_type nAdditionalCapacity, Side allocationSide) {
        // nAdditionalCapacity refers to the amount of additional space we need to be
        // able to store in this Deque. Typically this function is called as part of
        // an insert or append operation. This is the function that makes sure there
        // is enough capacity for the new elements to be copied into the Deque.
        // The new capacity here is always at the front or back of the Deque.
        // This function returns an iterator to that points to the new begin or
        // the new end of the Deque space, depending on allocationSide.

        if (allocationSide == kSideFront) {
            // There might be some free space (nCurrentAdditionalCapacity) at the front of the existing subarray.
            const size_type nCurrentAdditionalCapacity = (size_type) (_it_begin._current - _it_begin._begin);

            if (TURBO_UNLIKELY(nCurrentAdditionalCapacity < nAdditionalCapacity)) {
                // If we need to grow downward into a new subarray...

                const difference_type nSubarrayIncrease = (difference_type) (
                    ((nAdditionalCapacity - nCurrentAdditionalCapacity) + kDequeSubarraySize - 1) / kDequeSubarraySize);
                difference_type i;

                if (nSubarrayIncrease > (_it_begin._current_array_ptr - _ptr_array))
                    // If there are not enough pointers in front of the current (first) one...
                    DoReallocPtrArray((size_type) (nSubarrayIncrease - (_it_begin._current_array_ptr - _ptr_array)),
                                      kSideFront);

                for (i = 1; i <= nSubarrayIncrease; ++i)
                    _it_begin._current_array_ptr[-i] = do_allocate_subarray();
            }

            return _it_begin - (difference_type) nAdditionalCapacity;
        } else {
            auto const nCurrentAdditionalCapacity = (size_type) ((_it_end._begin + kDequeSubarraySize - 1) - _it_end._current);

            if (TURBO_UNLIKELY(nCurrentAdditionalCapacity < nAdditionalCapacity)) {
                // If we need to grow forward into a new subarray...

                auto const nSubarrayIncrease = (difference_type) (
                    ((nAdditionalCapacity - nCurrentAdditionalCapacity) + kDequeSubarraySize - 1) / kDequeSubarraySize);
                difference_type i;

                if (nSubarrayIncrease > ((_ptr_array + _ptr_array_size) - _it_end._current_array_ptr) - 1)
                    // If there are not enough pointers after the current (last) one...
                    DoReallocPtrArray(
                        (size_type) (nSubarrayIncrease - (
                                         ((_ptr_array + _ptr_array_size) - _it_end._current_array_ptr) - 1)),
                        kSideBack);

                for (i = 1; i <= nSubarrayIncrease; ++i)
                    _it_end._current_array_ptr[i] = do_allocate_subarray();
            }

            return _it_end + (difference_type) nAdditionalCapacity;
        }
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::DoReallocPtrArray(size_type nAdditionalCapacity,
                                                                        Side allocationSide) {
        // This function is not called unless the capacity is known to require a resize.
        //
        // We have an array of pointers (_ptr_array), of which a segment of them are in use and
        // at either end of the array are zero or more unused pointers. This function is being
        // called because we need to extend the capacity on either side of this array by
        // nAdditionalCapacity pointers. However, it's possible that if the user is continually
        // using push_back and pop_front then the pointer array will continue to be extended
        // on the back side and unused on the front side. So while we are doing this resizing
        // here we also take the opportunity to recenter the pointers and thus be balanced.
        // It man turn out that we don't even need to reallocate the pointer array in order
        // to increase capacity on one side, as simply moving the pointers to the center may
        // be enough to open up the requires space.
        //
        // Balanced pointer array     Unbalanced pointer array (unused space at front, no free space at back)
        // ----++++++++++++----        ---------+++++++++++

        const size_type nUnusedPtrCountAtFront = (size_type) (_it_begin._current_array_ptr - _ptr_array);
        const size_type nUsedPtrCount = (size_type) (_it_end._current_array_ptr - _it_begin._current_array_ptr) + 1;
        const size_type nUsedPtrSpace = nUsedPtrCount * sizeof(void *);
        const size_type nUnusedPtrCountAtBack = (_ptr_array_size - nUnusedPtrCountAtFront) - nUsedPtrCount;
        value_type **pPtrArrayBegin;

        if ((allocationSide == kSideBack) && (nAdditionalCapacity <= nUnusedPtrCountAtFront)) {
            // If we can take advantage of unused pointers at the front without doing any reallocation...

            if (nAdditionalCapacity < (nUnusedPtrCountAtFront / 2))
                // Possibly use more space than required, if there's a lot of extra space.
                nAdditionalCapacity = (nUnusedPtrCountAtFront / 2);

            pPtrArrayBegin = _ptr_array + (nUnusedPtrCountAtFront - nAdditionalCapacity);
            memmove(pPtrArrayBegin, _it_begin._current_array_ptr, nUsedPtrSpace);

#if FERMAT_DEBUG
            memset(pPtrArrayBegin + nUsedPtrCount, 0,
                   (size_t) (_ptr_array + _ptr_array_size) - (size_t) (pPtrArrayBegin + nUsedPtrCount));
#endif
        } else if ((allocationSide == kSideFront) && (nAdditionalCapacity <= nUnusedPtrCountAtBack))
        // If we can take advantage of unused pointers at the back without doing any reallocation...
        {
            if (nAdditionalCapacity < (nUnusedPtrCountAtBack / 2))
                // Possibly use more space than required, if there's a lot of extra space.
                nAdditionalCapacity = (nUnusedPtrCountAtBack / 2);

            pPtrArrayBegin = _it_begin._current_array_ptr + nAdditionalCapacity;
            memmove(pPtrArrayBegin, _it_begin._current_array_ptr, nUsedPtrSpace);

#if FERMAT_DEBUG
            memset(_ptr_array, 0, (size_t) ((uintptr_t) pPtrArrayBegin - (uintptr_t) _ptr_array));
#endif
        } else {
            // In this case we will have to do a reallocation.
            size_type nNewPtrArraySize = _ptr_array_size + std::max(_ptr_array_size, nAdditionalCapacity) + 2;
            nNewPtrArraySize = _array_allocator.good_size(nNewPtrArraySize);
            // Allocate extra capacity.
            value_type **const pNewPtrArray = do_allocate_ptr_array(nNewPtrArraySize);

            pPtrArrayBegin = pNewPtrArray + (_it_begin._current_array_ptr - _ptr_array) + ((allocationSide ==
                                     kSideFront)
                                     ? nAdditionalCapacity
                                     : 0);

            // The following is equivalent to: std::copy(_it_begin._current_array_ptr, _it_end._current_array_ptr + 1, pPtrArrayBegin);
            // It's OK to use memcpy instead of memmove because the destination is guaranteed to non-overlap the source.
            if (_ptr_array) // Could also say: 'if(_it_begin._current_array_ptr)'
                memcpy(pPtrArrayBegin, _it_begin._current_array_ptr, nUsedPtrSpace);

            do_free_ptr_array(_ptr_array, _ptr_array_size);

            _ptr_array = pNewPtrArray;
            _ptr_array_size = nNewPtrArraySize;
        }

        // We need to reset the begin and end iterators, as code that calls this expects them to *not* be invalidated.
        const difference_type nBeginOffset = _it_begin._current - _it_begin._begin;
        const difference_type nEndOffset = _it_end._current - _it_end._begin;
        _it_begin.set_subarray(pPtrArrayBegin);
        _it_begin._current = _it_begin._begin + nBeginOffset;
        _it_end.set_subarray((pPtrArrayBegin + nUsedPtrCount) - 1);
        _it_end._current = _it_end._begin + nEndOffset;
    }


    template<typename T, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void DequeBase<T, Allocator, ArrayAllocator, kDequeSubarraySize>::do_init(size_type n) {
        // This code is disabled because it doesn't currently work properly.
        // We are trying to make it so that a Deque can have a zero allocation
        // initial empty state, but we (OK, I) am having a hard time making
        // this elegant and efficient.
        //if(n)
        //{
        const size_type nNewPtrArraySize = (size_type) ((n / kDequeSubarraySize) + 1);
        // Always have at least one, even if n is zero.
        const size_type kMinPtrArraySize_ = kMinPtrArraySize;

        _ptr_array_size = _array_allocator.good_size(std::max(kMinPtrArraySize_, (nNewPtrArraySize + 2)));
        _ptr_array = do_allocate_ptr_array(_ptr_array_size);

        value_type **const pPtrArrayBegin = (_ptr_array + ((_ptr_array_size - nNewPtrArraySize) / 2));
        // Try to place it in the middle.
        value_type **const pPtrArrayEnd = pPtrArrayBegin + nNewPtrArraySize;
        value_type **pPtrArrayCurrent = pPtrArrayBegin;


        while (pPtrArrayCurrent < pPtrArrayEnd)
            *pPtrArrayCurrent++ = do_allocate_subarray();

        _it_begin.set_subarray(pPtrArrayBegin);
        _it_begin._current = _it_begin._begin;

        const difference_type endSubarrayIndex =
            static_cast<difference_type>(n / kDequeSubarraySize);
        _it_end.set_subarray(pPtrArrayBegin + endSubarrayIndex);
        _it_end._current = _it_end._begin + static_cast<difference_type>(n % kDequeSubarraySize);
    }


    ///////////////////////////////////////////////////////////////////////
    // DequeIterator
    ///////////////////////////////////////////////////////////////////////

    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::DequeIterator()
        : _current(nullptr), _begin(nullptr),_current_array_ptr(nullptr) {
        // Empty
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::DequeIterator(T **pCurrentArrayPtr, T *pCurrent)
        : _current(pCurrent), _begin(*pCurrentArrayPtr),
          _current_array_ptr(pCurrentArrayPtr) {
        // Empty
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::DequeIterator(const iterator &x)
        : _current(x._current), _begin(x._begin),  _current_array_ptr(x._current_array_ptr) {
        // Empty
    }

    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize> &DequeIterator<T, Pointer, Reference,
        kDequeSubarraySize>::operator=(const iterator &x) {
        _current = x._current;
        _begin = x._begin;
        _current_array_ptr = x._current_array_ptr;

        return *this;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::DequeIterator(const iterator &x, Increment)
        : _current(x._current), _begin(x._begin),  _current_array_ptr(x._current_array_ptr) {
        operator++();
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::DequeIterator(const iterator &x, Decrement)
        : _current(x._current), _begin(x._begin), _current_array_ptr(x._current_array_ptr) {
        operator--();
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::pointer
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator->() const {
        return _current;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::reference
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator*() const {
        return *_current;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type &
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator++() {
        if (TURBO_UNLIKELY(++_current == _begin + kDequeSubarraySize)) {
            _begin = *++_current_array_ptr;
            _current = _begin;
        }
        return *this;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator++(int) {
        const this_type temp(*this);
        operator++();
        return temp;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type &
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator--() {
        if (TURBO_UNLIKELY(_current == _begin)) {
            _begin = *--_current_array_ptr;
            _current = _begin + kDequeSubarraySize; // fall through...
        }
        --_current;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator--(int) {
        const this_type temp(*this);
        operator--();
        return temp;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type &
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator+=(difference_type n) {
        const difference_type subarrayPosition = (_current - _begin) + n;
        const difference_type kSubarray = (difference_type) kDequeSubarraySize;

        if (subarrayPosition >= 0 && subarrayPosition < kSubarray) {
            _current += n;
        } else {
            static_assert((kDequeSubarraySize & (kDequeSubarraySize - 1)) == 0, "Verify that it is a power of 2.");
            // Floor division: C++ truncates toward zero, but backward iteration needs toward -infinity.
            const difference_type subarrayIndex = (subarrayPosition >= 0)
                                                      ? subarrayPosition / kSubarray
                                                      : -(((-subarrayPosition) + kSubarray - 1) / kSubarray);

            set_subarray(_current_array_ptr + subarrayIndex);
            _current = _begin + (subarrayPosition - subarrayIndex * kSubarray);
        }
        return *this;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type &
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator-=(difference_type n) {
        return (*this).operator+=(-n);
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator+(difference_type n) const {
        return this_type(*this).operator+=(n);
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::operator-(difference_type n) const {
        return this_type(*this).operator+=(-n);
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::move(const iterator &first, const iterator &last,
                                                                   std::true_type) {
        const difference_type nElementCount = last - first;
        if (nElementCount <= 0)
            return *this;

        if ((first._begin == last._begin) && (first._begin == _begin) &&
            (first._current_array_ptr == last._current_array_ptr) &&
            (first._current_array_ptr == _current_array_ptr) &&
            (last._current >= first._current)) {
            const size_t nBytes = (size_t) ((uintptr_t) last._current - (uintptr_t) first._current);
            memmove(_current, first._current, nBytes);
            return *this + (last._current - first._current);
        }

        iterator src = first;
        this_type dst = *this;

        for (difference_type nLeft = nElementCount; nLeft > 0;) {
            const difference_type srcUntilChunkEnd = (src._begin + kDequeSubarraySize) - src._current;
            const difference_type dstUntilChunkEnd = (dst._begin + kDequeSubarraySize) - dst._current;
            difference_type batch = nLeft;
            if (srcUntilChunkEnd < batch)
                batch = srcUntilChunkEnd;
            if (dstUntilChunkEnd < batch)
                batch = dstUntilChunkEnd;
            if (batch == 0)
                break;

            memmove(dst._current, src._current, (size_t) batch * sizeof(T));

            nLeft -= batch;
            src._current += batch;
            dst._current += batch;

            if (src._current == src._begin + kDequeSubarraySize) {
                src._begin = *++src._current_array_ptr;
                src._current = src._begin;
            }
            if (dst._current == dst._begin + kDequeSubarraySize) {
                dst._begin = *++dst._current_array_ptr;
                dst._current = dst._begin;
            }
        }
        return *this + nElementCount;
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    typename DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::this_type
    DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::move(const iterator &first, const iterator &last,
                                                                   std::false_type) {
        return std::move(first, last, *this);
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    void DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::move_backward(
        const iterator &first, const iterator &last, std::true_type) {
        const difference_type nElementCount = last - first;
        if (nElementCount <= 0)
            return;

        if ((first._begin == last._begin) && (first._begin == _begin) &&
            (first._current_array_ptr == last._current_array_ptr) &&
            (first._current_array_ptr == _current_array_ptr) &&
            (last._current >= first._current)) {
            const difference_type nSameChunk = last._current - first._current;
            const size_t nBytes = (size_t) ((uintptr_t) last._current - (uintptr_t) first._current);
            memmove(_current - nSameChunk, first._current, nBytes);
            return;
        }

        iterator src = last;
        this_type dst = *this;

        for (difference_type nLeft = nElementCount; nLeft > 0;) {
            if (src._current == src._begin) {
                src._begin = *--src._current_array_ptr;
                src._current = src._begin + kDequeSubarraySize;
            }
            if (dst._current == dst._begin) {
                dst._begin = *--dst._current_array_ptr;
                dst._current = dst._begin + kDequeSubarraySize;
            }

            const difference_type srcUntilChunkBegin = src._current - src._begin;
            const difference_type dstUntilChunkBegin = dst._current - dst._begin;
            difference_type batch = nLeft;
            if (srcUntilChunkBegin < batch)
                batch = srcUntilChunkBegin;
            if (dstUntilChunkBegin < batch)
                batch = dstUntilChunkBegin;
            if (batch == 0)
                break;

            src._current -= batch;
            dst._current -= batch;
            memmove(dst._current, src._current, (size_t) batch * sizeof(T));
            nLeft -= batch;
        }
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    void DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::move_backward(
        const iterator &first, const iterator &last, std::false_type) {
        std::move_backward(first, last, *this);
    }


    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    void DequeIterator<T, Pointer, Reference, kDequeSubarraySize>::set_subarray(T **pCurrentArrayPtr) {
        _current_array_ptr = pCurrentArrayPtr;
        _begin = *pCurrentArrayPtr;
    }


    // The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
    // Thus we provide additional template paremeters here to support this. The defect report does not
    // require us to support comparisons between reverse_iterators and const_reverse_iterators.
    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline bool operator==(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
                           const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        return a._current == b._current;
    }


    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline bool operator!=(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
                           const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        return a._current != b._current;
    }


    // We provide a version of operator!= for the case where the iterators are of the
    // same type. This helps prevent ambiguity errors in the presence of rel_ops.
    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    inline bool operator!=(const DequeIterator<T, Pointer, Reference, kDequeSubarraySize> &a,
                           const DequeIterator<T, Pointer, Reference, kDequeSubarraySize> &b) {
        return a._current != b._current;
    }


    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline bool operator<(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
                          const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        return (a._current_array_ptr == b._current_array_ptr)
                   ? (a._current < b._current)
                   : (a._current_array_ptr < b._current_array_ptr);
    }


    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline bool operator>(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
                          const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        return (a._current_array_ptr == b._current_array_ptr)
                   ? (a._current > b._current)
                   : (a._current_array_ptr > b._current_array_ptr);
    }


    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline bool operator<=(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
                           const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        return (a._current_array_ptr == b._current_array_ptr)
                   ? (a._current <= b._current)
                   : (a._current_array_ptr <= b._current_array_ptr);
    }


    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline bool operator>=(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
                           const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        return (a._current_array_ptr == b._current_array_ptr)
                   ? (a._current >= b._current)
                   : (a._current_array_ptr >= b._current_array_ptr);
    }


    // Random access iterators must support operator + and operator -.
    // You can only add an integer to an iterator, and you cannot add two iterators.
    template<typename T, typename Pointer, typename Reference, unsigned kDequeSubarraySize>
    inline DequeIterator<T, Pointer, Reference, kDequeSubarraySize>
    operator+(ptrdiff_t n, const DequeIterator<T, Pointer, Reference, kDequeSubarraySize> &x) {
        return x + n; // Implement (n + x) in terms of (x + n).
    }


    // You can only add an integer to an iterator, but you can subtract two iterators.
    // The C++ defect report #179 mentioned above specifically refers to
    // operator - and states that we support the subtraction of const and non-const iterators.
    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB, unsigned
        kDequeSubarraySize>
    inline typename DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize>::difference_type
    operator-(const DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize> &a,
              const DequeIterator<T, PointerB, ReferenceB, kDequeSubarraySize> &b) {
        // This is a fairly clever algorithm that has been used in STL Deque implementations since the original HP STL:
        typedef typename DequeIterator<T, PointerA, ReferenceA, kDequeSubarraySize>::difference_type difference_type;

        return ((difference_type) kDequeSubarraySize * ((a._current_array_ptr - b._current_array_ptr) - 1)) + (
                   a._current - a._begin) + (b._begin + kDequeSubarraySize - b._current);
    }


    ///////////////////////////////////////////////////////////////////////
    // Deque
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque()
        : base_type((size_type) 0) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(const allocator_type &allocator)
        : base_type((size_type) 0, allocator) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(size_type n, const allocator_type &allocator)
        : base_type(n, allocator) {
        do_fill_init(value_type());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(size_type n, const value_type &value,
                                                          const allocator_type &allocator)
        : base_type(n, allocator) {
        do_fill_init(value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(const this_type &x)
        : base_type(x.size(), x._allocator) {
        std::uninitialized_copy(x._it_begin, x._it_end, _it_begin);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(this_type &&x) noexcept
        : base_type((size_type) 0, x._allocator) {
        swap(x);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(this_type &&x, const allocator_type &allocator)
        : base_type((size_type) 0, allocator) {
        swap(x); // member swap handles the case that x has a different allocator than our allocator by doing a copy.
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(std::initializer_list<value_type> ilist,
                                                          const allocator_type &allocator)
        : base_type(allocator) {
        do_init(ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::Deque(InputIterator first, InputIterator last)
        : base_type(allocator_type{})
    // Call the empty base constructor, which does nothing. We need to do all the work in our own do_init.
    {
        do_init(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::~Deque() {
        // Call destructors. Parent class will free the memory.
        fermat::destroy(_it_begin, _it_end);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::this_type &
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::operator=(const this_type &x) {
        // If not assigning to ourselves...
        if (&x != this) {
            do_assign(x.begin(), x.end(), std::false_type());
        }

        return *this;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::this_type &
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::operator=(this_type &&x) noexcept {
        if (this != &x) {
            this_type temp(_allocator);
            swap(temp);
            swap(x);
            // member swap handles the case that x has a different allocator than our allocator by doing a copy.
        }
        return *this;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::this_type &
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::operator=(std::initializer_list<value_type> ilist) {
        do_assign(ilist.begin(), ilist.end(), std::false_type());
        return *this;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::assign(size_type n, const value_type &value) {
        do_assign_values(n, value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::assign(std::initializer_list<value_type> ilist) {
        do_assign(ilist.begin(), ilist.end(), std::false_type());
    }


    // It turns out that the C++ std::Deque specifies a two argument
    // version of assign that takes (int size, int value). These are not
    // iterators, so we need to do a template compiler trick to do the right thing.
    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::assign(InputIterator first, InputIterator last) {
        do_assign(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::begin() noexcept {
        return _it_begin;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::begin() const noexcept {
        return _it_begin;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::cbegin() const noexcept {
        return _it_begin;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::end() noexcept {
        return _it_end;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::end() const noexcept {
        return _it_end;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::cend() const noexcept {
        return _it_end;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::rbegin() noexcept {
        return reverse_iterator(_it_end);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::rbegin() const noexcept {
        return const_reverse_iterator(_it_end);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::crbegin() const noexcept {
        return const_reverse_iterator(_it_end);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::rend() noexcept {
        return reverse_iterator(_it_begin);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::rend() const noexcept {
        return const_reverse_iterator(_it_begin);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::crend() const noexcept {
        return const_reverse_iterator(_it_begin);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::empty() const noexcept {
        return size() == 0;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type
    inline Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size() const noexcept {
        return (size_type) (_it_end - _it_begin);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::resize(size_type n, const value_type &value) {
        const size_type nSizeCurrent = size();

        if (n > nSizeCurrent) // We expect that more often than not, resizes will be upsizes.
            insert(_it_end, n - nSizeCurrent, value);
        else
            erase(_it_begin + (difference_type) n, _it_end);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::resize(size_type n) {
        resize(n, value_type());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::shrink_to_fit() {
        this_type x(std::make_move_iterator(begin()), std::make_move_iterator(end()));
        swap(x);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::set_capacity(size_type n) {
        // Currently there isn't a way to remove all allocations from a Deque, as it
        // requires a single starting allocation for the subarrays. So we can't just
        // free all memory without leaving it in a bad state. So the best means of
        // implementing set_capacity() is to do what we do below.

        if (n == 0) {
            this_type temp(_allocator);
            do_swap(temp);
        } else if (n < size()) {
            // We currently ignore the request to reduce capacity. To do: Implement this
            // and do it in a way that doesn't result in temporarily ~doubling our memory usage.
            // That might involve trimming unused subarrays from the front or back of
            // the container.
            resize(n);
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::operator[](size_type n) {
        // See DequeIterator::operator+=() for an explanation of the code below.
        iterator it(_it_begin);

        const difference_type subarrayPosition = (difference_type) ((it._current - it._begin) + (difference_type) n);
        const difference_type subarrayIndex = subarrayPosition / (difference_type) kDequeSubarraySize;

        return *(*(it._current_array_ptr + subarrayIndex) + (
                     subarrayPosition - (subarrayIndex * (difference_type) kDequeSubarraySize)));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::operator[](size_type n) const {
        // See DequeIterator::operator+=() for an explanation of the code below.
        iterator it(_it_begin);

        const difference_type subarrayPosition = (it._current - it._begin) + (difference_type) n;
        const difference_type subarrayIndex = subarrayPosition / (difference_type) kDequeSubarraySize;

        return *(*(it._current_array_ptr + subarrayIndex) + (
                     subarrayPosition - (subarrayIndex * (difference_type) kDequeSubarraySize)));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::at(size_type n) {
        return *(_it_begin.operator+((difference_type) n));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::at(size_type n) const {
        return *(_it_begin.operator+((difference_type) n));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::front() {
        return *_it_begin;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::front() const {
        return *_it_begin;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::back() {
        return *iterator(_it_end, typename iterator::Decrement());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::const_reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::back() const {
        return *iterator(_it_end, typename iterator::Decrement());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::push_front(const value_type &value) {
        emplace_front(value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::push_front(value_type &&value) {
        emplace_front(std::move(value));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::push_front() {
        emplace_front(value_type());
        return *_it_begin; // Same as return front();
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::push_back(const value_type &value) {
        emplace_back(value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::push_back(value_type &&value) {
        emplace_back(std::move(value));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::push_back() {
        emplace_back(value_type());
        return *iterator(_it_end, typename iterator::Decrement()); // Same thing as return back();
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::pop_front() {
        if ((_it_begin._current + 1) != _it_begin._begin + kDequeSubarraySize) // If the operation is very simple...
            fermat::destroy_at(_it_begin._current++);
        else {
            // This is executed only when we are popping the end (last) item off the front-most subarray.
            // In this case we need to free the subarray and point _it_begin to the next subarray.
#ifdef FERMAT_DEBUG
            value_type **pp = _it_begin._current_array_ptr;
#endif

            fermat::destroy_at(_it_begin._current); // _current == _begin + kDequeSubarraySize - 1
            DoFreeSubarray(_it_begin._begin);
            _it_begin.set_subarray(_it_begin._current_array_ptr + 1);
            _it_begin._current = _it_begin._begin;

#ifdef FERMAT_DEBUG
            *pp = nullptr;
#endif
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::pop_back() {
        if (_it_end._current != _it_end._begin) // If the operation is very simple...
            fermat::destroy_at(--_it_end._current);
        else {
            // This is executed only when we are popping the first item off the last subarray.
            // In this case we need to free the subarray and point _it_end to the previous subarray.
#ifdef FERMAT_DEBUG
            value_type **pp = _it_end._current_array_ptr;
#endif

            DoFreeSubarray(_it_end._begin);
            _it_end.set_subarray(_it_end._current_array_ptr - 1);
            _it_end._current = _it_end._begin + kDequeSubarraySize - 1;
            // Recall that _it_end points to one-past the last item in the container.
            fermat::destroy_at(_it_end._current);
            // Thus we need to call the destructor on the item *before* that last item.

#ifdef FERMAT_DEBUG
            *pp = nullptr;
#endif
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<class... Args>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::emplace(const_iterator position, Args &&... args) {
        if (TURBO_UNLIKELY(position._current == _it_end._current)) // If we are doing the same thing as push_back...
        {
            emplace_back(std::forward<Args>(args)...);
            return iterator(_it_end, typename iterator::Decrement());
            // Unfortunately, we need to make an iterator here, as the above push_back is an operation that can invalidate existing iterators.
        } else if (TURBO_UNLIKELY(position._current == _it_begin._current))
        // If we are doing the same thing as push_front...
        {
            emplace_front(std::forward<Args>(args)...);
            return _it_begin;
        }

        iterator itPosition(position, typename iterator::FromConst());
        value_type valueSaved(std::forward<Args>(args)...);
        // We need to save this because value may come from within our container. It would be somewhat tedious to make a workaround that could avoid this.
        const difference_type i(itPosition - _it_begin);


        if (i < (difference_type) (size() / 2))
        // Should we insert at the front or at the back? We divide the range in half.
        {
            emplace_front(std::move(*_it_begin));
            // This operation potentially invalidates all existing iterators and so we need to assign them anew relative to _it_begin below.

            itPosition = _it_begin + i;

            const iterator newPosition(itPosition, typename iterator::Increment());
            iterator oldBegin(_it_begin, typename iterator::Increment());
            const iterator oldBeginPlus1(oldBegin, typename iterator::Increment());

            oldBegin.move(oldBeginPlus1, newPosition, std::is_trivially_copyable<value_type>());
        } else {
            emplace_back(std::move(*iterator(_it_end, typename iterator::Decrement())));

            itPosition = _it_begin + i;

            iterator oldBack(_it_end, typename iterator::Decrement());
            const iterator oldBackMinus1(oldBack, typename iterator::Decrement());

            oldBack.move_backward(itPosition, oldBackMinus1, std::is_trivially_copyable<value_type>());
        }

        *itPosition = std::move(valueSaved);

        return itPosition;
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<class... Args>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::emplace_front(
        Args &&... args) {
        if (_it_begin._current != _it_begin._begin)
        // If we have room in the first subarray... we hope that usually this 'new' pathway gets executed, as it is slightly faster.
            construct_at(--_it_begin._current, std::forward<Args>(args)...);
        else {
            // To consider: Detect if value isn't coming from within this container and handle that efficiently.
            value_type valueSaved(std::forward<Args>(args)...);
            // We need to make a temporary, because args may be a value_type that comes from within our container and the operations below may change the container. But we can use move instead of copy.

            if (_it_begin._current_array_ptr == _ptr_array)
                // If there are no more pointers in front of the current (first) one...
                DoReallocPtrArray(1, kSideFront);

            _it_begin._current_array_ptr[-1] = do_allocate_subarray();

            _it_begin.set_subarray(_it_begin._current_array_ptr - 1);
            _it_begin._current = _it_begin._begin + kDequeSubarraySize - 1;
            construct_at(_it_begin._current, std::move(valueSaved));
        }

        return *_it_begin; // Same as return front();
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<class... Args>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reference Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::emplace_back(
        Args &&... args) {
        if ((_it_end._current + 1) != _it_end._begin + kDequeSubarraySize)
        // If we have room in the last subarray... we hope that usually this 'new' pathway gets executed, as it is slightly faster.
        {
            reference back = *_it_end._current;
            construct_at(_it_end._current++, std::forward<Args>(args)...);
            return back;
        } else {
            // To consider: Detect if value isn't coming from within this container and handle that efficiently.
            value_type valueSaved(std::forward<Args>(args)...);
            // We need to make a temporary, because args may be a value_type that comes from within our container and the operations below may change the container. But we can use move instead of copy.
            if (((_it_end._current_array_ptr - _ptr_array) + 1) >= (difference_type) _ptr_array_size)
                // If there are no more pointers after the current (last) one.
                DoReallocPtrArray(1, kSideBack);

            _it_end._current_array_ptr[1] = do_allocate_subarray();

            construct_at(_it_end._current, std::move(valueSaved));
            _it_end.set_subarray(_it_end._current_array_ptr + 1);
            _it_end._current = _it_end._begin;

            return *iterator(_it_end, typename iterator::Decrement()); // Same as return back();
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::insert(const_iterator position, const value_type &value) {
        return emplace(position, value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::insert(const_iterator position, value_type &&value) {
        return emplace(position, std::move(value));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::insert(const_iterator position, size_type n, const value_type &value) {
        return do_insert_values(position, n, value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::insert(const_iterator position, InputIterator first, InputIterator last) {
        return do_insert(position, first, last, std::is_integral<InputIterator>());
        // The C++ standard requires this sort of behaviour, as InputIterator might actually be Integer and 'first' is really 'count' and 'last' is really 'value'.
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        const difference_type i(position - _it_begin);
        do_insert(position, ilist.begin(), ilist.end(), std::false_type());
        return (_it_begin + i);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::erase(const_iterator position) {
        iterator itPosition(position, typename iterator::FromConst());
        iterator itNext(itPosition, typename iterator::Increment());
        const difference_type i(itPosition - _it_begin);

        if (i < (difference_type) (size() / 2))
        // Should we move the front entries forward or the back entries backward? We divide the range in half.
        {
            itNext.move_backward(_it_begin, itPosition, std::is_trivially_copyable<value_type>());
            pop_front();
        } else {
            itPosition.move(itNext, _it_end, std::is_trivially_copyable<value_type>());
            pop_back();
        }

        return _it_begin + i;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::erase(const_iterator first, const_iterator last) {
        iterator itFirst(first, typename iterator::FromConst());
        iterator itLast(last, typename iterator::FromConst());

        if ((itFirst != _it_begin) || (itLast != _it_end))
        // If not erasing everything... (We expect that the user won't call erase(begin, end) because instead the user would just call clear.)
        {
            const difference_type n(itLast - itFirst);
            const difference_type i(itFirst - _it_begin);

            if (i < (difference_type) ((size() - n) / 2)) {
                // Should we move the front entries forward or the back entries backward? We divide the range in half.
                const iterator itNewBegin(_it_begin + n);
                value_type **const pPtrArrayBegin = _it_begin._current_array_ptr;

                itLast.move_backward(_it_begin, itFirst, std::is_trivially_copyable<value_type>());
                fermat::destroy(_it_begin, itNewBegin);

                DoFreeSubarrays(pPtrArrayBegin, itNewBegin._current_array_ptr);

                _it_begin = itNewBegin;
            } else {
                // Else we will be moving back entries backward.
                iterator itNewEnd(_it_end - n);
                value_type **const pPtrArrayEnd = itNewEnd._current_array_ptr + 1;

                itFirst.move(itLast, _it_end, std::is_trivially_copyable<value_type>());
                fermat::destroy(itNewEnd, _it_end);

                DoFreeSubarrays(pPtrArrayEnd, _it_end._current_array_ptr + 1);

                _it_end = itNewEnd;
            }

            return _it_begin + i;
        }

        clear();
        return _it_end;
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::erase(reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reverse_iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::erase(reverse_iterator first, reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase(last.base(), first.base()));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::clear() {
        // Destroy all values and all subarrays they belong to, except for the first one,
        // as we need to reserve some space for a valid _it_begin/_it_end.
        if (_it_begin._current_array_ptr != _it_end._current_array_ptr) {
            // If there are multiple subarrays (more often than not, this will be so)...
            fermat::destroy(_it_begin._current, _it_begin._begin + kDequeSubarraySize);
            fermat::destroy(_it_end._begin, _it_end._current);
            DoFreeSubarray(_it_end._begin); // Leave _it_begin with a valid subarray.
        } else {
            fermat::destroy(_it_begin._current, _it_end._current);
        }

        for (value_type **pPtrArray = _it_begin._current_array_ptr + 1; pPtrArray < _it_end._current_array_ptr; ++
             pPtrArray) {
            fermat::destroy(*pPtrArray, *pPtrArray + kDequeSubarraySize);
            DoFreeSubarray(*pPtrArray);
        }

        _it_end = _it_begin; // _it_begin/_it_end will not be dereferencable.
    }


    //template <typename T, typename Allocator, unsigned kDequeSubarraySize>
    //void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::reset_lose_memory()
    //{
    //    // The reset_lose_memory function is a special extension function which unilaterally
    //    // resets the container to an empty state without freeing the memory of
    //    // the contained objects. This is useful for very quickly tearing down a
    //    // container built into scratch memory.
    //
    //    // Currently we are unable to get this reset_lose_memory operation to work correctly
    //    // as we haven't been able to find a good way to have a Deque initialize
    //    // without allocating memory. We can lose the old memory, but do_init
    //    // would necessarily do a ptrArray allocation. And this is not within
    //    // our definition of how reset_lose_memory works.
    //    base_type::do_init(0);
    //
    //}


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::swap(Deque &x) noexcept {
        do_swap(x);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename Integer>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_init(Integer n, Integer value, std::true_type) {
        base_type::do_init(n); // Call the base uninitialized init function.
        do_fill_init(value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_init(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        do_init_from_iterator(first, last, IC());
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_init_from_iterator(InputIterator first, InputIterator last,
                                                                        std::input_iterator_tag) {
        base_type::do_init(0); // Call the base uninitialized init function, but don't actually allocate any values.

        // We have little choice but to iterate through the source iterator and call
        // push_back for each item. It can be slow because it will keep reallocating the
        // container memory as we go (every kDequeSubarraySize elements). We are not allowed to use distance() on an InputIterator.
        for (; first != last; ++first)
        // InputIterators by definition actually only allow you to iterate through them once.
        {
            // Thus the standard *requires* that we do this (inefficient) implementation.
            push_back(*first);
            // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename ForwardIterator>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_init_from_iterator(ForwardIterator first, ForwardIterator last,
                                                                        std::forward_iterator_tag) {
        typedef typename std::remove_const<ForwardIterator>::type non_const_iterator_type;
        // If T is a const type (e.g. const int) then we need to initialize it as if it were non-const.
        typedef typename std::remove_const<value_type>::type non_const_value_type;

        auto const n = (size_type) std::distance(first, last);
        value_type **pPtrArrayCurrent;

        base_type::do_init(n); // Call the base uninitialized init function.

        for (pPtrArrayCurrent = _it_begin._current_array_ptr; pPtrArrayCurrent < _it_end._current_array_ptr; ++
             pPtrArrayCurrent) // Copy to the known-to-be-completely-used subarrays.
        {
            // We implment an algorithm here whereby we use uninitialized_copy() and advance() instead of just iterating from first to last and constructing as we go.
            // The reason for this is that we can take advantage of trivially copyable data types and implement construction as memcpy operations.
            ForwardIterator current(first);
            // To do: Implement a specialization of this algorithm for non-trivially copyable types which eliminates the need for 'current'.

            std::advance(current, kDequeSubarraySize);
            std::uninitialized_copy((non_const_iterator_type) first, (non_const_iterator_type) current,
                                    (non_const_value_type *) *pPtrArrayCurrent);
            first = current;
        }

        std::uninitialized_copy((non_const_iterator_type) first, (non_const_iterator_type) last,
                                (non_const_value_type *) _it_end._begin);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_fill_init(const value_type &value) {
        value_type **pPtrArrayCurrent = _it_begin._current_array_ptr;

        while (pPtrArrayCurrent < _it_end._current_array_ptr) {
            fermat::uninitialized_fill(*pPtrArrayCurrent, *pPtrArrayCurrent + kDequeSubarraySize, value);
            ++pPtrArrayCurrent;
        }
        fermat::uninitialized_fill(_it_end._begin, _it_end._current, value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename Integer>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_assign(Integer n, Integer value, std::true_type)
    // std::false_type means this is the integer version instead of iterator version.
    {
        do_assign_values(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_assign(InputIterator first, InputIterator last, std::false_type) {
        // std::false_type means this is the iterator version instead of integer version.

        // Actually, the implementation below requires first/last to be a ForwardIterator and not just an InputIterator.
        // But Paul Pedriana if you somehow need to work with an InputIterator and we can deal with it.
        auto const n = (size_type) std::distance(first, last);
        const size_type nSize = size();

        if (n > nSize) // If we are increasing the size...
        {
            InputIterator atEnd(first);

            std::advance(atEnd, (difference_type) nSize);
            std::copy(first, atEnd, _it_begin);
            insert(_it_end, atEnd, last);
        } else // n is <= size.
        {
            iterator itEnd(std::copy(first, last, _it_begin));

            if (n < nSize) // If we need to erase any trailing elements...
                erase(itEnd, _it_end);
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_assign_values(size_type n, const value_type &value) {
        const size_type nSize = size();

        if (n > nSize) // If we are increasing the size...
        {
            std::fill(_it_begin, _it_end, value);
            insert(_it_end, n - nSize, value);
        } else {
            erase(_it_begin + (difference_type) n, _it_end);
            std::fill(_it_begin, _it_end, value);
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename Integer>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_insert(const const_iterator &position, Integer n, Integer value,
                                                       std::true_type) {
        return do_insert_values(position, (size_type) n, (value_type) value);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_insert(const const_iterator &position, const InputIterator &first,
                                                       const InputIterator &last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        return do_insert_from_iterator(position, first, last, IC());
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename InputIterator>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_insert_from_iterator(const_iterator position,
                                                                     const InputIterator &first,
                                                                     const InputIterator &last,
                                                                     std::input_iterator_tag) {
        const difference_type index = std::distance(cbegin(), position);
        // We have little choice but to iterate through the source iterator and call
        // insert for each item. It can be slow because it will keep reallocating the
        // container memory as we go (every kDequeSubarraySize elements). We are not
        // allowed to use distance() on an InputIterator. InputIterators by definition
        // actually only allow you to iterate through them once. Thus the standard
        // *requires* that we do this (inefficient) implementation. Luckily,
        // InputIterators are in practice almost never used, so this code will likely
        // never get executed.
        for (InputIterator iter = first; iter != last; ++iter) {
            position = insert(position, *iter) + 1;
        }

        return begin() + index;
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    template<typename ForwardIterator>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_insert_from_iterator(const_iterator position,
                                                                     const ForwardIterator &first,
                                                                     const ForwardIterator &last,
                                                                     std::forward_iterator_tag) {
        const size_type n = (size_type) std::distance(first, last);

        // This implementation is nearly identical to do_insert_values below.
        // If you make a bug fix to one, you will likely want to fix the other.
        if (position._current == _it_begin._current) {
            // If inserting at the beginning or into an empty container...
            iterator itNewBegin(do_realloc_subarray(n, kSideFront));
            // itNewBegin to _it_begin refers to memory that isn't initialized yet; so it's not truly a valid iterator. Or at least not a dereferencable one.
            // We would like to use move here instead of copy when possible, which would be useful for
            // when inserting from a std::initializer_list, for example.
            // To do: solve this by having a template or runtime parameter which specifies move vs copy.
            std::uninitialized_copy(first, last, itNewBegin);
            _it_begin = itNewBegin;

            return _it_begin;
        } else if (TURBO_UNLIKELY(position._current == _it_end._current)) {
            // If inserting at the end (i.e. appending)...

            const iterator itNewEnd(do_realloc_subarray(n, kSideBack));
            // _it_end to itNewEnd refers to memory that isn't initialized yet; so it's not truly a valid iterator. Or at least not a dereferencable one.
            const iterator itFirstInserted(_it_end);
            // We would like to use move here instead of copy when possible, which would be useful for
            // when inserting from a std::initializer_list, for example.
            // To do: solve this by having a template or runtime parameter which specifies move vs copy.
            std::uninitialized_copy(first, last, _it_end);
            _it_end = itNewEnd;

            return itFirstInserted;
        } else {
            const difference_type nInsertionIndex = position - _it_begin;
            const size_type nSize = size();

            if (nInsertionIndex < (difference_type) (nSize / 2)) {
                // If the insertion index is in the front half of the Deque... grow the Deque at the front.

                const iterator itNewBegin(do_realloc_subarray(n, kSideFront));
                // itNewBegin to _it_begin refers to memory that isn't initialized yet; so it's not truly a valid iterator. Or at least not a dereferencable one.
                const iterator itOldBegin(_it_begin);
                const iterator itPosition(_it_begin + nInsertionIndex);
                // We need to reset this value because the reallocation above can invalidate iterators.

                // We have a problem here: we would like to use move instead of copy, but it may be that the range to be inserted comes from
                // this container and comes from the segment we need to move. So we can't use move operations unless we are careful to handle
                // that situation. The newly inserted contents must be contents that were moved to and not moved from. To do: solve this.
                if (nInsertionIndex >= (difference_type) n) {
                    // If the newly inserted items will be entirely within the old area...
                    iterator itUCopyEnd(_it_begin + (difference_type) n);

                    std::uninitialized_copy(_it_begin, itUCopyEnd, itNewBegin); // This can throw.
                    itUCopyEnd = std::copy(itUCopyEnd, itPosition, itOldBegin);
                    // Recycle 'itUCopyEnd' to mean something else.
                    std::copy(first, last, itUCopyEnd);
                } else {
                    // Else the newly inserted items are going within the newly allocated area at the front.
                    ForwardIterator mid(first);

                    std::advance(mid, (difference_type) n - nInsertionIndex);
                    uninitialized_copy_copy(_it_begin, itPosition, first, mid, itNewBegin); // This can throw.
                    std::copy(mid, last, itOldBegin);
                }
                _it_begin = itNewBegin;
            } else {
                const iterator itNewEnd(do_realloc_subarray(n, kSideBack));
                const iterator itOldEnd(_it_end);
                const difference_type nPushedCount = (difference_type) nSize - nInsertionIndex;
                const iterator itPosition(_it_end - nPushedCount);
                // We need to reset this value because the reallocation above can invalidate iterators.

                // We have a problem here: we would like to use move instead of copy, but it may be that the range to be inserted comes from
                // this container and comes from the segment to move. So we can't use move operations unless we are careful to handle
                // that situation. The newly inserted contents must be contents that were moved to and not moved from. To do: solve this.
                if (nPushedCount > (difference_type) n) {
                    const iterator itUCopyEnd(_it_end - (difference_type) n);

                    std::uninitialized_copy(itUCopyEnd, _it_end, _it_end);
                    std::copy_backward(itPosition, itUCopyEnd, itOldEnd);
                    std::copy(first, last, itPosition);
                } else {
                    ForwardIterator mid(first);

                    std::advance(mid, nPushedCount);
                    uninitialized_copy_copy(mid, last, itPosition, _it_end, _it_end);
                    std::copy(first, mid, itPosition);
                }
                _it_end = itNewEnd;
            }

            return iterator(_it_begin + nInsertionIndex);
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::iterator
    Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_insert_values(const_iterator position, size_type n,
                                                              const value_type &value) {
        // This implementation is nearly identical to do_insert_from_iterator above.
        // If you make a bug fix to one, you will likely want to fix the other.
        if (position._current == _it_begin._current) {
            // If inserting at the beginning...
            const iterator itNewBegin(do_realloc_subarray(n, kSideFront));

            // Note that we don't make a temp copy of 'value' here. This is because in a
            // Deque, insertion at either the front or back doesn't cause a reallocation
            // or move of data in the middle. That's a key feature of deques, in fact.
            fermat::uninitialized_fill(itNewBegin, _it_begin, value);
            _it_begin = itNewBegin;

            return _it_begin;
        } else if (TURBO_UNLIKELY(position._current == _it_end._current)) {
            // If inserting at the end (i.e. appending)...
            const iterator itNewEnd(do_realloc_subarray(n, kSideBack));
            const iterator itFirstInserted(_it_end);

            // Note that we don't make a temp copy of 'value' here. This is because in a
            // Deque, insertion at either the front or back doesn't cause a reallocation
            // or move of data in the middle. That's a key feature of deques, in fact.
            fermat::uninitialized_fill(_it_end, itNewEnd, value);
            _it_end = itNewEnd;

            return itFirstInserted;
        } else {
            // A key purpose of a Deque is to implement insertions and removals more efficiently
            // than with a vector. We are inserting into the middle of the Deque here. A quick and
            // dirty implementation of this would be to reallocate the subarrays and simply push
            // all values in the middle upward like you would do with a vector. Instead we implement
            // the minimum amount of reallocations needed but may need to do some value moving,
            // as the subarray sizes need to remain constant and can have no holes in them.
            const difference_type nInsertionIndex = position - _it_begin;
            const size_type nSize = size();
            const value_type valueSaved(value);

            if (nInsertionIndex < (difference_type) (nSize / 2)) {
                // If the insertion index is in the front half of the Deque... grow the Deque at the front.
                iterator itNewBegin(do_realloc_subarray(n, kSideFront));
                iterator itOldBegin(_it_begin);
                const iterator itPosition(_it_begin + nInsertionIndex);
                // We need to reset this value because the reallocation above can invalidate iterators.

                if (nInsertionIndex >= (difference_type) n) {
                    // If the newly inserted items will be entirely within the old area...
                    iterator itUCopyEnd(_it_begin + (difference_type) n);

                    if constexpr (std::is_trivially_copyable_v<value_type>) {
                        itNewBegin.move(_it_begin, itUCopyEnd, std::true_type{}); // This can throw.
                        itUCopyEnd = itOldBegin.move(itUCopyEnd, itPosition, std::true_type{});
                    } else {
                        fermat::uninitialized_move(_it_begin, itUCopyEnd, itNewBegin); // This can throw.
                        itUCopyEnd = std::move(itUCopyEnd, itPosition, itOldBegin);
                    }
                    // Recycle 'itUCopyEnd' to mean something else.
                    std::fill(itUCopyEnd, itPosition, valueSaved);
                } else {
                    // Else the newly inserted items are going within the newly allocated area at the front.
                    if constexpr (std::is_trivially_copyable_v<value_type>) {
                        itNewBegin.move(_it_begin, itPosition, std::true_type{}); // This can throw.
                    } else {
                        fermat::uninitialized_move(_it_begin, itPosition, itNewBegin); // This can throw.
                    }
                    fermat::uninitialized_fill(itNewBegin + nInsertionIndex, itNewBegin + (difference_type) n,
                                               valueSaved);
                    std::fill(itOldBegin, itPosition, valueSaved);
                }
                _it_begin = itNewBegin;

                return iterator(_it_begin + nInsertionIndex);
            } else {
                // Else the insertion index is in the back half of the Deque, so grow the Deque at the back.
                iterator itNewEnd(do_realloc_subarray(n, kSideBack));
                iterator itOldEnd(_it_end);
                const difference_type nPushedCount = (difference_type) nSize - nInsertionIndex;
                const iterator itPosition(_it_end - nPushedCount);
                // We need to reset this value because the reallocation above can invalidate iterators.

                if (nPushedCount > (difference_type) n) {
                    // If the newly inserted items will be entirely within the old area...
                    iterator itUCopyEnd(_it_end - (difference_type) n);

                    if constexpr (std::is_trivially_copyable_v<value_type>) {
                        iterator itWrite(_it_end);
                        itWrite.move(itUCopyEnd, _it_end, std::true_type{}); // This can throw.
                        itOldEnd.move_backward(itPosition, itUCopyEnd, std::true_type{});
                    } else {
                        fermat::uninitialized_move(itUCopyEnd, _it_end, _it_end); // This can throw.
                        std::move_backward(itPosition, itUCopyEnd, itOldEnd);
                    }
                    // Recycle 'itUCopyEnd' to mean something else.
                    std::fill(itPosition, itUCopyEnd, valueSaved);
                } else {
                    // Else the newly inserted items are going within the newly allocated area at the back.
                    fermat::uninitialized_fill(_it_end, itPosition + (difference_type) n, valueSaved);
                    if constexpr (std::is_trivially_copyable_v<value_type>) {
                        iterator itTailDest(_it_end + ((difference_type) n - nPushedCount));
                        itTailDest.move(itPosition, _it_end, std::true_type{});
                    } else {
                        fermat::uninitialized_move(itPosition, _it_end,
                                                   _it_end + ((difference_type) n - nPushedCount));
                    }
                    std::fill(itPosition, itOldEnd, valueSaved);
                }
                _it_end = itNewEnd;

                return iterator(_it_begin + nInsertionIndex);
            }
        }
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::do_swap(this_type &x) {
        std::swap(_ptr_array, x._ptr_array);
        std::swap(_ptr_array_size, x._ptr_array_size);
        std::swap(_it_begin, x._it_begin);
        std::swap(_it_end, x._it_end);
        std::swap(_allocator, x._allocator);
        std::swap(_array_allocator, x._array_allocator);
    }


    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::validate() const {
        // To do: More detailed validation.
        // To do: Try to make the validation resistant to crashes if the data is invalid.
        if ((end() - begin()) < 0)
            return false;
        return true;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool operator==(const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a,
                           const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        return ((a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin()));
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool operator!=(const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a,
                           const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        return ((a.size() != b.size()) || !std::equal(a.begin(), a.end(), b.begin()));
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool operator<(const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a,
                          const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool operator>(const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a,
                          const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        return b < a;
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool operator<=(const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a,
                           const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        return !(b < a);
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline bool operator>=(const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a,
                           const Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        return !(a < b);
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator, unsigned kDequeSubarraySize>
    inline void swap(Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &a, Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &b) {
        a.swap(b);
    }

    ///////////////////////////////////////////////////////////////////////
    // erase / erase_if
    //
    // https://en.cppreference.com/w/cpp/container/Deque/erase2
    ///////////////////////////////////////////////////////////////////////
    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator,
        unsigned kDequeSubarraySize, typename U>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type erase(
        Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &c, const U &value) {
        // Erases all elements that compare equal to value from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove(c.begin(), origEnd, value);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);

        return static_cast<typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type>(numRemoved);
    }

    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator,
        unsigned kDequeSubarraySize, typename Predicate>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type erase_if(
        Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &c, Predicate predicate) {
        // Erases all elements that satisfy the predicate pred from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove_if(c.begin(), origEnd, predicate);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);

        return static_cast<typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type>(numRemoved);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase_unsorted
    //
    // This serves a similar purpose as erase above but with the difference
    // that it doesn't preserve the relative order of what is left in the
    // Deque.
    //
    // Effects: Removes all elements equal to value from the Deque while
    // optimizing for speed with the potential reordering of elements as a
    // side effect.
    //
    // Complexity: Linear
    //
    ///////////////////////////////////////////////////////////////////////
    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator,
        unsigned kDequeSubarraySize, typename U>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type erase_unsorted(
        Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &c, const U &value) {
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

        auto origEnd = c.end();
        auto numRemoved = std::distance(itRemove, origEnd);
        c.erase(itRemove, origEnd);


        return static_cast<typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type>(numRemoved);
    }

    ///////////////////////////////////////////////////////////////////////
    // erase_unsorted_if
    //
    // This serves a similar purpose as erase_if above but with the
    // difference that it doesn't preserve the relative order of what is
    // left in the Deque.
    //
    // Effects: Removes all elements that return true for the predicate
    // while optimizing for speed with the potential reordering of elements
    // as a side effect.
    //
    // Complexity: Linear
    //
    ///////////////////////////////////////////////////////////////////////
    template<typename T, size_t Alignment, typename Policy, typename Allocator, typename ArrayAllocator,
        unsigned kDequeSubarraySize, typename Predicate>
    typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type erase_unsorted_if(
        Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize> &c, Predicate predicate) {
        // Erases all elements that satisfy predicate from the container.
        auto itRemove = c.begin();
        auto ritMove = c.rbegin();

        while (true) {
            itRemove = std::find_if(itRemove, ritMove.base(), predicate);
            if (itRemove == ritMove.base()) // any elements to remove?
                break;

            ritMove = std::find_if(ritMove, std::make_reverse_iterator(itRemove), std::not_fn(predicate));
            if (itRemove == ritMove.base()) // any elements that can be moved into place?
                break;

            *itRemove = std::move(*ritMove);
            ++itRemove;
            ++ritMove;
        }

        // now all elements in the range [itRemove, c.end()) are either to be removed or have already been moved from.

        auto origEnd = c.end();
        auto numRemoved = std::distance(itRemove, origEnd);
        c.erase(itRemove, origEnd);

        return static_cast<typename Deque<T, Alignment, Policy, Allocator, ArrayAllocator, kDequeSubarraySize>::size_type>(numRemoved);
    }
} // namespace fermat
