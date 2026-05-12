///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a Vector (array-like container), much like the C++ 
// std::Vector class.
// The primary distinctions between this Vector and std::Vector are:
//    - Vector has a couple extension functions that increase performance.
//    - Vector can contain objects with alignment requirements. std::Vector 
//      cannot do so without a bit of tedious non-portable effort.
//    - Vector supports debug memory naming natively.
//    - Vector is easier to read, debug, and visualize.
//    - Vector is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - Vector has less deeply nested function calls and allows the user to 
//      enable forced inlining in debug builds in order to reduce bloat.
//    - Vector<bool> is a Vector of boolean values and not a bit Vector.
//    - Vector guarantees that memory is contiguous and that Vector::iterator
//      is nothing more than a pointer to T.
//    - Vector has an explicit data() method for obtaining a pointer to storage 
//      which is safe to call even if the block is empty. This avoids the 
//      common &v[0], &v.front(), and &*v.begin() constructs that trigger false 
//      asserts in STL debugging modes.
//    - Vector data is guaranteed to be contiguous.
//    - Vector has a set_capacity() function which frees excess capacity. 
//      The only way to do this with std::Vector is via the cryptic non-obvious 
//      trick of using: Vector<SomeClass>(x).swap(x);
///////////////////////////////////////////////////////////////////////////////

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
#include <fermat/container/utility.h>
#include <fermat/container/traits.h>

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
    template<typename T, size_t Alignment>
    struct VectorBase {
        using allocator_type = BytesPoolAllocator<T, Alignment>;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

        static const size_type npos = (size_type) -1; /// 'npos' means non-valid position or simply non-position.
        static const size_type kMaxSize = (size_type) -2;
        /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.

        size_type GetNewCapacity(size_type currentSize);

    protected:
        T *_begin{nullptr};
        T *_end{nullptr};
        T *_capacity_end{nullptr};

    public:
        VectorBase() = default;

        VectorBase(size_type n);

        virtual ~VectorBase();

    protected:
        virtual T *do_allocate(size_type *n);

        virtual size_type do_allocate_size(size_type n);

        virtual void do_free(T *p, size_type n);

        void uninitialized_n(size_t n);

        void construct_at(T *ptr, const T *, size_type n);

        void construct_at(T *ptr, size_type n);

        template<typename... Args>
        void construct_args_at(T *ptr, size_type n, const Args &... args);

        template<typename... Args>
        void construct_args_at(T *ptr, Args &&... args);

        void construct_move_at(T *ptr, T *, size_type n);
    }; // VectorBase


    /// Vector
    ///
    /// Implements a dynamic array.
    ///
    template<typename T, size_t Alignment = 0>
    class Vector : public VectorBase<T, Alignment> {
        typedef VectorBase<T, Alignment> base_type;
        typedef Vector<T, Alignment> this_type;

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
        using base_type::construct_move_at;
        using base_type::construct_at;
        using base_type::construct_args_at;

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

        explicit Vector(size_type n);

        Vector(size_type n, const value_type &value);

        Vector(const this_type &x);

        Vector(this_type &&x) noexcept;

        Vector(std::initializer_list<value_type> ilist);

        // note: this has pre-C++11 semantics:
        // this constructor is equivalent to the constructor Vector(static_cast<size_type>(first), static_cast<value_type>(last), allocator) if InputIterator is an integral type.
        template<typename InputIterator>
        Vector(InputIterator first, InputIterator last);

        ~Vector();

        this_type &operator=(const this_type &x);

        this_type &operator=(std::initializer_list<value_type> ilist);

        this_type &operator=(this_type &&x);

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

        bool validate() const noexcept;

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

    template<typename T, size_t Alignment>
    inline VectorBase<T, Alignment>::VectorBase(size_type n) {
        uninitialized_n(n);
    }


    template<typename T, size_t Alignment>
    inline VectorBase<T, Alignment>::~VectorBase() {
        if (_begin) {
            do_free(_begin, _capacity_end - _begin);
        }
    }


    template<typename T, size_t Alignment>
    inline T *VectorBase<T, Alignment>::do_allocate(size_type *n) {
        // If n is zero, then we allocate no memory and just return nullptr.
        // This is fine, as our default ctor initializes with NULL pointers.
        if (TURBO_LIKELY(n)) {
            auto ptr = allocator_type::pooled_alloc(n);
            return ptr;
        } else {
            return nullptr;
        }
    }
    template<typename T, size_t Alignment>
    inline typename  VectorBase<T, Alignment>::size_type VectorBase<T, Alignment>::do_allocate_size(size_type n) {
        if (TURBO_LIKELY(n)) {
            return  allocator_type::pooled_alloc_size(n);
        } else {
            return 0;
        }
    }

    template<typename T, size_t Alignment>
    void VectorBase<T, Alignment>::uninitialized_n(size_t n) {
        _begin = allocator_type::pooled_alloc(&n);
        _end = _begin;
        _capacity_end = _begin + n;
    }

    template<typename T, size_t Alignment>
    void VectorBase<T, Alignment>::construct_at(T *ptr, const T *values, size_type n) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (n > 0) {
                std::memcpy(static_cast<void *>(ptr), static_cast<const void *>(values), n * sizeof(T));
            }
        } else {
            for (size_t i = 0; i < n; ++i) {
                ::new(static_cast<void *>(ptr + i)) T(values[i]);
            }
        }
    }

    template<typename T, size_t Alignment>
    void VectorBase<T, Alignment>::construct_at(T *ptr, size_type n) {
        if constexpr (std::is_trivially_default_constructible_v<T>) {
            if (n > 0) {
                // For POD/Trivial types, zero-initialize the memory block.
                // Note: If you don't need zero-initialization for PODs,
                // you could technically leave this empty to maximize performance.
                std::memset(static_cast<void *>(ptr), 0, n * sizeof(T));
            }
        } else {
            for (size_type i = 0; i < n; ++i) {
                // Value-initialization: invokes the default constructor.
                ::new(static_cast<void *>(ptr + i)) T();
            }
        }
    }

    template<typename T, size_t Alignment>
    template<typename... Args>
    void VectorBase<T, Alignment>::construct_args_at(T *ptr, size_type n, const Args &... args) {
        for (size_type i = 0; i < n; ++i) {
            if constexpr (sizeof...(Args) > 0) {
                if (i < n - 1) {
                    // Construct via lvalue references for all but the last element
                    ::new(static_cast<void *>(ptr + i)) T(args...);
                } else {
                    // Forward arguments to the last element to allow move semantics
                    ::new(static_cast<void *>(ptr + i)) T(std::forward<Args>(args)...);
                }
            } else {
                // Default construction if no arguments are provided
                ::new(static_cast<void *>(ptr + i)) T();
            }
        }
    }

    template<typename T, size_t Alignment>
    template<typename... Args>
    void VectorBase<T, Alignment>::construct_args_at(T *ptr, Args &&... args) {
        // This will correctly call T(const T&) if an lvalue is passed,
        // or T(T&&) if std::move() is used.
        ::new(static_cast<T *>(ptr)) T(std::forward<Args>(args)...);
    }

    template<typename T, size_t Alignment>
    void VectorBase<T, Alignment>::construct_move_at(T *ptr, T *values, size_type n) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (n > 0) {
                // Trivial types can be moved safely via memmove (handles overlap).
                std::memmove(static_cast<void *>(ptr), static_cast<const void *>(values), n * sizeof(T));
            }
        } else {
            for (size_type i = 0; i < n; ++i) {
                // Placement new with move semantics.
                ::new(static_cast<void *>(ptr + i)) T(std::move(values[i]));
            }
        }
    }

    template<typename T, size_t Alignment>
    inline void VectorBase<T, Alignment>::do_free(T *p, size_type n) {
        if (p) {
            allocator_type::pooled_free(p, n);
        }
    }


    template<typename T, size_t Alignment>
    inline typename VectorBase<T, Alignment>::size_type
    VectorBase<T, Alignment>::GetNewCapacity(size_type currentSize) {
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

    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::Vector() noexcept
        : base_type() {
        // Empty
    }


    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::Vector(size_type n)
        : base_type(n) {
        std::uninitialized_value_construct_n(_begin, n);
        _end = _begin + n;
    }


    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::Vector(size_type n, const value_type &value)
        : base_type(n) {
        std::uninitialized_fill_n(_begin, n, value);
        _end = _begin + n;
    }


    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::Vector(const this_type &x)
        : base_type(x.size()) {
        _end = std::uninitialized_copy(x._begin, x._end, _begin);
    }


    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::Vector(this_type &&x) noexcept {
        DoSwap(x);
    }


    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::Vector(std::initializer_list<value_type> ilist)
        : base_type() {
        DoInit(ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline Vector<T, Alignment>::Vector(InputIterator first, InputIterator last)
        : base_type() {
        DoInit(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment>
    inline Vector<T, Alignment>::~Vector() {
        // Call destructor for the values. Parent class will free the memory.
        std::destroy(_begin, _end);
    }


    template<typename T, size_t Alignment>
    typename Vector<T, Alignment>::this_type &
    Vector<T, Alignment>::operator=(const this_type &x) {
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


    template<typename T, size_t Alignment>
    typename Vector<T, Alignment>::this_type &
    Vector<T, Alignment>::operator=(std::initializer_list<value_type> ilist) {
        typedef typename std::initializer_list<value_type>::iterator InputIterator;
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC());
        // initializer_list has const elements and so we can't move from them.
        return *this;
    }


    template<typename T, size_t Alignment>
    typename Vector<T, Alignment>::this_type &
    Vector<T, Alignment>::operator=(this_type &&x) {
        if (this != &x) {
            DoClearCapacity();
            // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
            swap(x);
            // member swap handles the case that x has a different allocator than our allocator by doing a copy.
        }
        return *this;
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::assign(size_type n, const value_type &value) {
        DoAssignValues(n, value);
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline void Vector<T, Alignment>::assign(InputIterator first, InputIterator last) {
        // It turns out that the C++ std::Vector<int, int> specifies a two argument
        // version of assign that takes (int size, int value). These are not iterators,
        // so we need to do a template compiler trick to do the right thing.
        DoAssign<InputIterator, false>(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::assign(std::initializer_list<value_type> ilist) {
        typedef typename std::initializer_list<value_type>::iterator InputIterator;
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC());
        // initializer_list has const elements and so we can't move from them.
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::begin() noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_iterator
    Vector<T, Alignment>::begin() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_iterator
    Vector<T, Alignment>::cbegin() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::end() noexcept {
        return _end;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_iterator
    Vector<T, Alignment>::end() const noexcept {
        return _end;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_iterator
    Vector<T, Alignment>::cend() const noexcept {
        return _end;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::rbegin() noexcept {
        return reverse_iterator(_end);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reverse_iterator
    Vector<T, Alignment>::rbegin() const noexcept {
        return const_reverse_iterator(_end);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reverse_iterator
    Vector<T, Alignment>::crbegin() const noexcept {
        return const_reverse_iterator(_end);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::rend() noexcept {
        return reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reverse_iterator
    Vector<T, Alignment>::rend() const noexcept {
        return const_reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reverse_iterator
    Vector<T, Alignment>::crend() const noexcept {
        return const_reverse_iterator(_begin);
    }


    template<typename T, size_t Alignment>
    bool Vector<T, Alignment>::empty() const noexcept {
        return (_begin == _end);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::size_type
    Vector<T, Alignment>::size() const noexcept {
        return (size_type) (_end - _begin);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::size_type
    Vector<T, Alignment>::capacity() const noexcept {
        return (size_type) (_capacity_end - _begin);
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::resize(size_type n, const value_type &value) {
        if (n > (size_type) (_end - _begin)) // We expect that more often than not, resizes will be upsizes.
            DoInsertValuesEnd(n - ((size_type) (_end - _begin)), value);
        else {
            std::destroy(_begin + n, _end);
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::resize(size_type n) {
        // Alternative implementation:
        // resize(n, value_type());

        if (n > (size_type) (_end - _begin)) // We expect that more often than not, resizes will be upsizes.
            DoInsertValuesEnd(n - ((size_type) (_end - _begin)));
        else {
            std::destroy(_begin + n, _end);
            _end = _begin + n;
        }
    }


    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::reserve(size_type n) {
        // If the user wants to reduce the reserved memory, there is the set_capacity function.
        if (n > size_type(_capacity_end - _begin)) // If n > capacity ...
            DoGrow(n);
    }


    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::set_capacity(size_type n) {
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
            do_free(_begin, (size_type) (_capacity_end - _begin));

            const ptrdiff_t nPrevSize = _end - _begin;
            _begin = pNewData;
            _end = pNewData + nPrevSize;
            _capacity_end = _begin + nn;
        }
    }

    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::shrink_to_fit() {
        // This is the simplest way to accomplish this, and it is as efficient as any other.
        auto n = do_allocate_size(size());
        if (n < capacity()) {
            this_type temp = this_type(std::move_iterator<iterator>(begin()), std::move_iterator<iterator>(end()));

            // Call DoSwap() rather than swap() as we know our allocators match and we don't want to invoke the code path
            // handling non matching allocators as it imposes additional restrictions on the type of T to be copyable
            DoSwap(temp);
        }

    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::pointer
    Vector<T, Alignment>::data() noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_pointer
    Vector<T, Alignment>::data() const noexcept {
        return _begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reference
    Vector<T, Alignment>::operator[](size_type n) {
        // We allow the user to use a reference to v[0] of an empty container. But this was merely grandfathered in and ideally we shouldn't allow such access to [0].
        if (TURBO_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(_end - _begin)))))
            KCHECK(false) << "Vector::operator[] -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reference
    Vector<T, Alignment>::operator[](size_type n) const {
        if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
            KCHECK(false) << "Vector::operator[] -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reference
    Vector<T, Alignment>::at(size_type n) {
        // The difference between at() and operator[] is it signals
        // the requested position is out of range by throwing an
        // out_of_range exception.

        if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
            KCHECK(false) << "Vector::at -- out of range";
        return *(_begin + n);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reference
    Vector<T, Alignment>::at(size_type n) const {
        if (TURBO_UNLIKELY(n >= (static_cast<size_type>(_end - _begin))))
            KCHECK(false) << "Vector::at -- out of range";

        return *(_begin + n);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reference
    Vector<T, Alignment>::front() {
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            // We don't allow the user to reference an empty container.
            KCHECK(false) << "Vector::front -- empty Vector";

        return *_begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reference
    Vector<T, Alignment>::front() const {
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            // We don't allow the user to reference an empty container.
            KCHECK(false) << "Vector::front -- empty Vector";

        return *_begin;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reference
    Vector<T, Alignment>::back() {
        // if _end is nullptr the expression (_end - 1) is undefined behaviour.
        // any use of back() with an empty Vector is thus conceptually wrong.
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            KCHECK(false) << "Vector::back -- empty Vector";

        return *(_end - 1);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::const_reference
    Vector<T, Alignment>::back() const {
        // if _end is nullptr the expression (_end - 1) is undefined behaviour.
        // any use of back() with an empty Vector is thus conceptually wrong.
        if (TURBO_UNLIKELY((_begin == nullptr) || (_end <= _begin)))
            KCHECK(false) << "Vector::back -- empty Vector";

        return *(_end - 1);
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::push_back(const value_type &value) {
        if (_end < _capacity_end)
            construct_at(_end++, std::addressof(value), 1);
        else
            DoInsertValueEnd(value);
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::push_back(value_type &&value) {
        if (_end < _capacity_end)
            construct_move_at(_end++, &value, 1);
        else
            DoInsertValueEnd(std::move(value));
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reference
    Vector<T, Alignment>::push_back() {
        if (_end < _capacity_end)
            construct_at(_end++, 1);
        else
            DoInsertValueEnd();

        return *(_end - 1); // Same as return back();
    }


    template<typename T, size_t Alignment>
    inline void *Vector<T, Alignment>::push_back_uninitialized() {
        if (_end == _capacity_end) {
            const size_type nPrevSize = size_type(_end - _begin);
            const size_type nNewCapacity = GetNewCapacity(nPrevSize);
            DoGrow(nNewCapacity);
        }

        return _end++;
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::pop_back() {
        if (TURBO_UNLIKELY(_end <= _begin))
            KCHECK(false) << "Vector::pop_back -- empty Vector";


        --_end;
        _end->~value_type();
    }

    template<typename T, size_t Alignment>
    template<class... Args>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::emplace(const_iterator position, Args &&... args) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.

        if ((_end == _capacity_end) || (position != _end))
            DoInsertValue(position, std::forward<Args>(args)...);
        else {
            construct_args_at(_end, std::forward<Args>(args)...);
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        }

        return _begin + n;
    }

    template<typename T, size_t Alignment>
    template<class... Args>
    inline typename Vector<T, Alignment>::reference
    Vector<T, Alignment>::emplace_back(Args &&... args) {
        if (_end < _capacity_end) {
            construct_args_at(_end, std::forward<Args>(args)...);
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        } else
            DoInsertValueEnd(std::forward<Args>(args)...);

        return back();
    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::insert(const_iterator position, const value_type &value) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Vector::insert -- invalid position";

        // We implment a quick pathway for the case that the insertion position is at the end and we have free capacity for it.
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.

        if ((_end == _capacity_end) || (position != _end))
            DoInsertValue(position, value);
        else {
            construct_at(_end, &value, 1);
            ++_end; // Increment this after the construction above in case the construction throws an exception.
        }

        return _begin + n;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::insert(const_iterator position, value_type &&value) {
        return emplace(position, std::move(value));
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::insert(const_iterator position, size_type n, const value_type &value) {
        const ptrdiff_t p = position - _begin; // Save this because we might reallocate.
        DoInsertValues(position, n, value);
        return _begin + p;
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::insert(const_iterator position, InputIterator first, InputIterator last) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.
        DoInsert(position, first, last, std::is_integral<InputIterator>());
        return _begin + n;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        const ptrdiff_t n = position - _begin; // Save this because we might reallocate.
        DoInsert(position, ilist.begin(), ilist.end(), std::false_type());
        return _begin + n;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::erase(const_iterator position) {
        if (TURBO_UNLIKELY((position < _begin) || (position >= _end)))
            KCHECK(false) << "Vector::erase -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if ((position + 1) < _end)
            std::move(destPosition + 1, _end, destPosition);
        --_end;
        _end->~value_type();
        return destPosition;
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::erase(const_iterator first, const_iterator last) {
        if (TURBO_UNLIKELY((first < _begin) || (first > _end) || (last < _begin) || (last > _end) || (last < first)))
            KCHECK(false) << "Vector::erase -- invalid position";
        if (first != last) {
            iterator const position = const_cast<value_type *>(std::move(
                const_cast<value_type *>(last), const_cast<value_type *>(_end), const_cast<value_type *>(first)));
            std::destroy(position, _end);
            _end -= (last - first);
        }

        return const_cast<value_type *>(first);
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::erase_unsorted(const_iterator position) {
        if (TURBO_UNLIKELY((position < _begin) || (position >= _end)))
            KCHECK(false) << "Vector::erase -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);
        *destPosition = std::move(*(_end - 1));

        // pop_back();
        --_end;
        _end->~value_type();

        return destPosition;
    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator Vector<T, Alignment>::erase_first(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        iterator it = std::find(begin(), end(), value);

        if (it != end())
            return erase(it);
        else
            return it;
    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::iterator
    Vector<T, Alignment>::erase_first_unsorted(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        iterator it = std::find(begin(), end(), value);

        if (it != end())
            return erase_unsorted(it);
        else
            return it;
    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::erase_last(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        reverse_iterator it = std::find(rbegin(), rend(), value);

        if (it != rend())
            return erase(it);
        else
            return it;
    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::erase_last_unsorted(const T &value) {
        static_assert(has_equality_operator<T>::value, "T must be comparable");

        reverse_iterator it = std::find(rbegin(), rend(), value);

        if (it != rend())
            return erase_unsorted(it);
        else
            return it;
    }

    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase(last.base(), first.base()));
    }


    template<typename T, size_t Alignment>
    inline typename Vector<T, Alignment>::reverse_iterator
    Vector<T, Alignment>::erase_unsorted(const_reverse_iterator position) {
        return reverse_iterator(erase_unsorted((++position).base()));
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::clear() noexcept {
        std::destroy(_begin, _end);
        _end = _begin;
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::reset_lose_memory() noexcept {
        // The reset function is a special extension function which unilaterally
        // resets the container to an empty state without freeing the memory of
        // the contained objects. This is useful for very quickly tearing down a
        // container built into scratch memory.
        _begin = _end = _capacity_end = nullptr;
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::swap(this_type &x) {
        DoSwap(x);
    }


    template<typename T, size_t Alignment>
    template<typename ForwardIterator>
    inline typename Vector<T, Alignment>::pointer
    Vector<T, Alignment>::do_realloc(size_type *newCapacity, ForwardIterator first, ForwardIterator last,
                                     should_copy_tag) {
        T *const p = do_allocate(newCapacity); // p is of type T* but is not constructed.
        std::uninitialized_copy(first, last, p); // copy-constructs p from [first,last).
        return p;
    }


    template<typename T, size_t Alignment>
    template<typename ForwardIterator>
    inline typename Vector<T, Alignment>::pointer
    Vector<T, Alignment>::do_realloc(size_type *newCapacity, ForwardIterator first, ForwardIterator last,
                                     should_move_tag) {
        T *const p = do_allocate(newCapacity); // p is of type T* but is not constructed.
        std::uninitialized_move(first, last, p); // move-constructs p from [first,last).
        return p;
    }


    template<typename T, size_t Alignment>
    template<typename Integer>
    inline void Vector<T, Alignment>::DoInit(Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to initialize a Vector larger than can fit in a size_type!");
        size_type nn  =n;
        _begin = do_allocate(&nn);
        _capacity_end = _begin + nn;
        _end = _begin + n;

        std::uninitialized_fill_n(_begin, n, value);
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline void Vector<T, Alignment>::DoInit(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoInitFromIterator(first, last, IC());
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline void Vector<T, Alignment>::DoInitFromIterator(InputIterator first, InputIterator last,
                                                         std::input_iterator_tag) {
        // To do: Use emplace_back instead of push_back(). Our emplace_back will work below without any ifdefs.
        for (; first != last; ++first)
            // InputIterators by definition actually only allow you to iterate through them once.
            push_back(*first); // Thus the standard *requires* that we do this (inefficient) implementation.
    } // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.


    template<typename T, size_t Alignment>
    template<typename ForwardIterator>
    inline void Vector<T, Alignment>::DoInitFromIterator(ForwardIterator first, ForwardIterator last,
                                                         std::forward_iterator_tag) {
        const auto d = std::distance(first, last);

        container_internal::AssertValueFitsInType<size_type>(
            d, "Attempting to initialize a Vector larger than can fit in a size_type!");

        const size_type n = static_cast<size_type>(d);
        auto nn = n;
        _begin = do_allocate(&nn);
        _capacity_end = _begin + nn;
        _end = _begin + n;

        std::uninitialized_copy(first, last, _begin);
    }


    template<typename T, size_t Alignment>
    template<typename Integer, bool bMove>
    inline void Vector<T, Alignment>::DoAssign(Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(n, "Attempting to assign more values than can fit in a size_type!");
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator, bool bMove>
    inline void Vector<T, Alignment>::DoAssign(InputIterator first, InputIterator last, std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoAssignFromIterator<InputIterator, bMove>(first, last, IC());
    }


    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::DoAssignValues(size_type n, const value_type &value) {
        if (n > size_type(_capacity_end - _begin)) // If n > capacity ...
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


    template<typename T, size_t Alignment>
    template<typename InputIterator, bool bMove>
    void Vector<T, Alignment>::DoAssignFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag) {
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


    template<typename T, size_t Alignment>
    template<typename RandomAccessIterator, bool bMove>
    void Vector<T, Alignment>::DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last,
                                                    std::random_access_iterator_tag) {
        const auto d = std::distance(first, last);
        container_internal::AssertValueFitsInType<size_type>(d, "Attempting to assign more values than can fit in a size_type!");

        const size_type n = static_cast<size_type>(d);
        // If n > capacity ...
        if (n > size_type(_capacity_end - _begin)) {
            auto nn = n;
            pointer const pNewData = do_realloc(&nn, first, last, should_move_or_copy_tag<bMove>());
            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end - _begin));

            _begin = pNewData;
            _end = _begin + n;
            _capacity_end = _begin + nn;
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


    template<typename T, size_t Alignment>
    template<typename Integer>
    inline void Vector<T, Alignment>::DoInsert(const_iterator position, Integer n, Integer value, std::true_type) {
        container_internal::AssertValueFitsInType<size_type>(
            n, "Attempting to insert more elements than can can fit in size_type!");

        DoInsertValues(position, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline void Vector<T, Alignment>::DoInsert(const_iterator position, InputIterator first, InputIterator last,
                                               std::false_type) {
        typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
        DoInsertFromIterator(position, first, last, IC());
    }


    template<typename T, size_t Alignment>
    template<typename InputIterator>
    inline void Vector<T, Alignment>::DoInsertFromIterator(const_iterator position, InputIterator first,
                                                           InputIterator last, std::input_iterator_tag) {
        for (; first != last; ++first, ++position)
            position = insert(position, *first);
    }


    template<typename T, size_t Alignment>
    template<typename BidirectionalIterator>
    void Vector<T, Alignment>::DoInsertFromIterator(const_iterator position, BidirectionalIterator first,
                                                    BidirectionalIterator last, std::bidirectional_iterator_tag) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Vector::insert -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if (first != last) {
            const auto d = std::distance(first, last);
            container_internal::AssertValueFitsInType<size_type>(
                d, "Attempting to insert more elements than can fit in a Vector.");

            const size_type n = static_cast<size_type>(d); // n is the number of elements we are inserting.

            if (n <= size_type(_capacity_end - _end)) // If n fits within the existing capacity...
            {
                const size_type nExtra = static_cast<size_type>(_end - destPosition);

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
                do_free(_begin, (size_type) (_capacity_end - _begin));

                _begin = pNewData;
                _end = pNewEnd;
                _capacity_end = pNewData + nNewCapacity;
            }
        }
    }


    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::DoInsertValues(const_iterator position, size_type n, const value_type &value) {
        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Vector::insert -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if (n <= size_type(_capacity_end - _end)) // If n is <= capacity...
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
            do_free(_begin, (size_type) (_capacity_end - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end = pNewData + nNewCapacity;
        }
    }


    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::DoClearCapacity()
    // This function exists because set_capacity() currently indirectly requires value_type to be default-constructible,
    {
        // and some functions that need to clear our capacity (e.g. operator=) aren't supposed to require default-constructibility.
        clear();
        this_type temp(std::move(*this)); // This is the simplest way to accomplish this,
        swap(temp); // and it is as efficient as any other.
    }


    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::DoGrow(size_type newCapacity) {
        pointer const pNewData = do_allocate(&newCapacity);

        pointer pNewEnd = std::uninitialized_move(_begin, _end, pNewData);

        std::destroy(_begin, _end);
        do_free(_begin, (size_type) (_capacity_end - _begin));

        _begin = pNewData;
        _end = pNewEnd;
        _capacity_end = pNewData + newCapacity;
    }


    template<typename T, size_t Alignment>
    inline void Vector<T, Alignment>::DoSwap(this_type &x) {
        std::swap(_begin, x._begin);
        std::swap(_end, x._end);
        std::swap(_capacity_end, x._capacity_end);
    }

    // The code duplication between this and the version that takes no value argument and default constructs the values
    // is unfortunate but not easily resolved without relying on C++11 perfect forwarding.
    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::DoInsertValuesEnd(size_type n, const value_type &value) {
        if (n > size_type(_capacity_end - _end)) {
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
            do_free(_begin, (size_type) (_capacity_end - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end = pNewData + nNewCapacity;
        } else {
            std::uninitialized_fill_n(_end, n, value);
            _end += n;
        }
    }

    template<typename T, size_t Alignment>
    void Vector<T, Alignment>::DoInsertValuesEnd(size_type n) {
        if (n > size_type(_capacity_end - _end)) {
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
            do_free(_begin, (size_type) (_capacity_end - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end = pNewData + nNewCapacity;
        } else {
            std::uninitialized_value_construct_n(_end, n);
            _end += n;
        }
    }

    template<typename T, size_t Alignment>
    template<typename... Args>
    void Vector<T, Alignment>::DoInsertValue(const_iterator position, Args &&... args) {
        // To consider: It's feasible that the args is from a value_type comes from within the current sequence itself and
        // so we need to be sure to handle that case. This is different from insert(position, const value_type&) because in
        // this case value is potentially being modified.


        if (TURBO_UNLIKELY((position < _begin) || (position > _end)))
            KCHECK(false) << "Vector::insert/emplace -- invalid position";

        // C++11 stipulates that position is const_iterator, but the return value is iterator.
        iterator destPosition = const_cast<value_type *>(position);

        if (_end != _capacity_end) // If size < capacity ...
        {
            // We need to take into account the possibility that args is a value_type that comes from within the Vector itself.
            // creating a temporary value on the stack here is not an optimal way to solve this because sizeof(value_type) may be
            // too much for the given platform. An alternative solution may be to specialize this function for the case of the
            // argument being const value_type& or value_type&&.
            KCHECK(position < _end);
            // While insert at end() is valid, our design is such that calling code should handle that case before getting here, as our streamlined logic directly doesn't handle this particular case due to resulting negative ranges.

            value_type value(std::forward<Args>(args)...);
            // Need to do this before the move_backward below because maybe args refers to something within the moving range.
            construct_move_at(_end, _end - 1, 1);
            // _end is uninitialized memory, so we must construct into it instead of move into it like we do with the other elements below.
            std::move_backward(destPosition, _end - 1, _end);
            // We need to go backward because of potential overlap issues.
            std::destroy_at(destPosition);
            construct_move_at(destPosition, std::addressof(value), 1);
            // Move the value argument to the given position.
            ++_end;
        } else // else (size == capacity)
        {
            const size_type nPosSize = size_type(destPosition - _begin); // Index of the insertion position.
            const size_type nPrevSize = size_type(_end - _begin);
            size_type nNewCapacity = GetNewCapacity(nPrevSize);
            pointer const pNewData = do_allocate(&nNewCapacity);


            construct_args_at(pNewData + nPosSize, std::forward<Args>(args)...);
            // Because the old data is potentially being moved rather than copied, we need to move
            pointer pNewEnd = std::uninitialized_move(_begin, destPosition, pNewData);
            // the value first, because it might possibly be a reference to the old data being moved.
            pNewEnd = std::uninitialized_move(destPosition, _end, ++pNewEnd);

            std::destroy(_begin, _end);
            do_free(_begin, (size_type) (_capacity_end - _begin));

            _begin = pNewData;
            _end = pNewEnd;
            _capacity_end = pNewData + nNewCapacity;
        }
    }

    // assumes _end == _capacity_end, ie. create a new array and move existing elements into it while inserting the new element at the end.
    template<typename T, size_t Alignment>
    template<typename... Args>
    void Vector<T, Alignment>::DoInsertValueEnd(Args &&... args) {
        const size_type nPrevSize = size_type(_end - _begin);
        size_type nNewCapacity = GetNewCapacity(nPrevSize);
        pointer const pNewData = do_allocate(&nNewCapacity);


        // Because args... may potentially reference an element (or its sub-object) of this Vector, we need to construct
        // the new element first, prior to moving it (leaving it in an unspecified state) with the call to uninitialized_move.
        construct_args_at(pNewData + nPrevSize, std::forward<Args>(args)...);
        pointer pNewEnd = std::uninitialized_move(_begin, _end, pNewData);
        pNewEnd++;

        std::destroy(_begin, _end);
        do_free(_begin, (size_type) (_capacity_end - _begin));

        _begin = pNewData;
        _end = pNewEnd;
        _capacity_end = pNewData + nNewCapacity;
    }


    template<typename T, size_t Alignment>
    inline bool Vector<T, Alignment>::validate() const noexcept {
        if (_end < _begin)
            return false;
        if (_capacity_end < _end)
            return false;
        return true;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment>
    inline bool operator==(const Vector<T, Alignment> &a, const Vector<T, Alignment> &b) {
        return ((a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin()));
    }

    template<typename T, size_t Alignment>
    inline bool operator!=(const Vector<T, Alignment> &a, const Vector<T, Alignment> &b) {
        return ((a.size() != b.size()) || !std::equal(a.begin(), a.end(), b.begin()));
    }


    template<typename T, size_t Alignment>
    inline bool operator<(const Vector<T, Alignment> &a, const Vector<T, Alignment> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template<typename T, size_t Alignment>
    inline bool operator>(const Vector<T, Alignment> &a, const Vector<T, Alignment> &b) {
        return b < a;
    }


    template<typename T, size_t Alignment>
    inline bool operator<=(const Vector<T, Alignment> &a, const Vector<T, Alignment> &b) {
        return !(b < a);
    }


    template<typename T, size_t Alignment>
    inline bool operator>=(const Vector<T, Alignment> &a, const Vector<T, Alignment> &b) {
        return !(a < b);
    }

    template<typename T, size_t Alignment>
    inline void swap(Vector<T, Alignment> &a, Vector<T, Alignment> &b) noexcept {
        a.swap(b);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase / erase_if
    //
    // https://en.cppreference.com/w/cpp/container/Vector/erase2
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class U>
    typename Vector<T, Alignment>::size_type erase(Vector<T, Alignment> &c, const U &value) {
        // Erases all elements that compare equal to value from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove(c.begin(), origEnd, value);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);

        return static_cast<typename Vector<T, Alignment>::size_type>(numRemoved);
    }

    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename Vector<T, Alignment>::size_type erase_if(Vector<T, Alignment> &c, Predicate predicate) {
        // Erases all elements that satisfy the predicate pred from the container.
        auto origEnd = c.end();
        auto newEnd = std::remove_if(c.begin(), origEnd, predicate);
        auto numRemoved = std::distance(newEnd, origEnd);
        c.erase(newEnd, origEnd);


        return static_cast<typename Vector<T, Alignment>::size_type>(numRemoved);
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
    typename Vector<T, Alignment>::size_type erase_unsorted(Vector<T, Alignment> &c, const U &value) {
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

        return static_cast<typename Vector<T, Alignment>::size_type>(numRemoved);
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
    typename Vector<T, Alignment>::size_type erase_unsorted_if(Vector<T, Alignment> &c, Predicate predicate) {
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

        return static_cast<typename Vector<T, Alignment>::size_type>(numRemoved);
    }
} // namespace fermat
