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
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <fermat/container/compressed_pair.h>
#include <fermat/container/construct.h>

namespace fermat {
    /// ListNodeBase
    ///
    /// We define a ListNodeBase separately from ListNode (below), because it allows
    /// us to have non-templated operations such as insert, remove (below), and it
    /// makes it so that the List anchor node doesn't carry a T with it, which would
    /// waste space and possibly lead to surprising the user due to extra Ts existing
    /// that the user didn't explicitly create. The downside to all of this is that
    /// it makes debug viewing of a List harder, given that the node pointers are of
    /// type ListNodeBase and not ListNode. However, see ListNodeBaseProxy below.
    ///
    struct ListNodeBase {
        ListNodeBase *_next;
        ListNodeBase *_prev;

        void insert(ListNodeBase *pNext) noexcept;

        // Inserts this standalone node before the node pNext in pNext's List.
        void remove() noexcept; // Removes this node from the List it's in. Leaves this node's _next/_prev invalid.
        void splice(ListNodeBase *pFirst, ListNodeBase *pLast) noexcept;

        // Removes [pFirst,pLast) from the List it's in and inserts it before this in this node's List.
        void reverse() noexcept; // Reverses the order of nodes in the circular List this node is a part of.
        static void swap(ListNodeBase &a, ListNodeBase &b) noexcept;

        // Swaps the nodes a and b in the lists to which they belong.

        void insert_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept;

        // Differs from splice in that first/final aren't in another List.
        static void remove_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept; //
    };

    template<typename T>
    struct ListNode : public ListNodeBase {
        T mValue;
    };

    /// ListIterator
    ///
    template<typename T, typename Pointer, typename Reference>
    struct ListIterator {
        typedef ListIterator<T, Pointer, Reference> this_type;
        typedef ListIterator<T, T *, T &> iterator;
        typedef ListIterator<T, const T *, const T &> const_iterator;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T value_type;
        typedef ListNodeBase base_node_type;
        typedef ListNode<T> node_type;
        typedef Pointer pointer;
        typedef Reference reference;
        typedef std::bidirectional_iterator_tag iterator_category;

    public:
        base_node_type *mpNode;

    public:
        ListIterator() noexcept;

        ListIterator(const ListNodeBase *pNode) noexcept;

        template<typename This = this_type, std::enable_if_t<!std::is_same_v<This, iterator>, bool> = true>
        inline ListIterator(const iterator &x) noexcept
            : mpNode(x.mpNode) {
            // Empty
        }

        this_type next() const noexcept;

        this_type prev() const noexcept;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        this_type &operator++() noexcept;

        this_type operator++(int) noexcept;

        this_type &operator--() noexcept;

        this_type operator--(int) noexcept;
    }; // ListIterator


    /// ListBase
    ///
    /// See VectorBase (class vector) for an explanation of why we
    /// create this separate base class.
    ///
    template<typename T, size_t Alignment, typename Allocator>
    class ListBase {
    public:
        typedef T value_type;
        typedef Allocator allocator_type;
        typedef ListNode<T> node_type;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef ListNodeBase base_node_type;
        // We use ListNodeBase instead of ListNode<T> because we don't want to create a T.

    protected:
        fermat::compressed_pair<base_node_type, allocator_type> _node_allocator;
        size_type _size;


        base_node_type &internal_node() noexcept { return _node_allocator.first(); }
        [[nodiscard]] base_node_type const &internal_node() const noexcept { return _node_allocator.first(); }
        allocator_type &internal_allocator() noexcept { return _node_allocator.second(); }
        const allocator_type &internal_allocator() const noexcept { return _node_allocator.second(); }

    public:
        const allocator_type &get_allocator() const noexcept;

        allocator_type &get_allocator() noexcept;

        void set_allocator(const allocator_type &allocator);

    protected:
        ListBase();

        ListBase(const allocator_type &a);

        ~ListBase();

        node_type *do_allocate_node();

        void do_free_node(node_type *pNode);

        void do_init() noexcept;

        void do_clear();
    }; // ListBase


    /// List
    ///
    /// Pool allocation
    /// If you want to make a custom memory pool for a List container, your pool
    /// needs to contain items of type List::node_type. So if you have a memory
    /// pool that has a constructor that takes the size of pool items and the
    /// count of pool items, you would do this (assuming that MemoryPool implements
    /// the Allocator interface):
    ///     typedef List<Widget, MemoryPool> WidgetList;           // Delare your WidgetList type.
    ///     MemoryPool myPool(sizeof(WidgetList::node_type), 100); // Make a pool of 100 Widget nodes.
    ///     WidgetList myList(&myPool);                            // Create a List that uses the pool.
    ///
    template<typename T, size_t Alignment = 0, typename Allocator = AlignedAllocator<ListNode<T>, Alignment> >
    class List : public ListBase<T, Alignment, Allocator> {
        typedef ListBase<T, Alignment, Allocator> base_type;
        typedef List<T, Alignment, Allocator> this_type;

    protected:
        using base_type::_node_allocator;
        using base_type::do_allocate_node;
        using base_type::do_free_node;
        using base_type::do_clear;
        using base_type::do_init;
        using base_type::_size;
        using base_type::internal_node;
        using base_type::internal_allocator;

    public:
        typedef T value_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
        typedef ListIterator<T, T *, T &> iterator;
        typedef ListIterator<T, const T *, const T &> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef typename base_type::size_type size_type;
        typedef typename base_type::difference_type difference_type;
        typedef typename base_type::allocator_type allocator_type;
        typedef typename base_type::node_type node_type;
        typedef typename base_type::base_node_type base_node_type;

        using base_type::get_allocator;

        static_assert(!std::is_const_v<value_type>, "List<T> value_type must be non-const.");
        static_assert(!std::is_volatile_v<value_type>, "List<T> value_type must be non-volatile.");

    public:
        List();

        List(const allocator_type &allocator);

        explicit List(size_type n, const allocator_type &allocator = allocator_type{});

        List(size_type n, const value_type &value, const allocator_type &allocator = allocator_type{});

        List(const this_type &x);

        List(const this_type &x, const allocator_type &allocator);

        List(this_type &&x) noexcept;

        List(this_type &&, const allocator_type &);

        List(std::initializer_list<value_type> ilist, const allocator_type &allocator = allocator_type{});

        template<typename InputIterator>
        List(InputIterator first, InputIterator last);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        this_type &operator=(const this_type &x);

        this_type &operator=(std::initializer_list<value_type> ilist);

        this_type &operator=(this_type &&x) noexcept;

        // In the case that the two containers' allocators are unequal, swap copies elements instead
        // of replacing them in place. In this case swap is an O(n) operation instead of O(1).
        void swap(this_type &x) noexcept;

        void assign(size_type n, const value_type &value);

        template<typename InputIterator> // It turns out that the C++ std::List specifies a two argument
        void assign(InputIterator first, InputIterator last);

        // version of assign that takes (int size, int value). These are not
        // iterators, so we need to do a template compiler trick to do the right thing.
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

        [[nodiscard]] bool empty() const noexcept;

        size_type size() const noexcept;

        void resize(size_type n, const value_type &value);

        void resize(size_type n);

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        template<typename... Args>
        reference emplace_front(Args &&... args);

        template<typename... Args>
        reference emplace_back(Args &&... args);

        void push_front(const value_type &value);

        void push_front(value_type &&value) noexcept;

        reference push_front();

        void *push_front_uninitialized();

        void push_back(const value_type &value);

        void push_back(value_type &&value) noexcept;

        reference push_back();

        void *push_back_uninitialized();

        void pop_front();

        void pop_back();

        template<typename... Args>
        iterator emplace(const_iterator position, Args &&... args);

        iterator insert(const_iterator position);

        iterator insert(const_iterator position, const value_type &value);

        iterator insert(const_iterator position, value_type &&value);

        iterator insert(const_iterator position, std::initializer_list<value_type> ilist);

        iterator insert(const_iterator position, size_type n, const value_type &value);

        template<typename InputIterator>
        iterator insert(const_iterator position, InputIterator first, InputIterator last);

        iterator erase(const_iterator position);

        iterator erase(const_iterator first, const_iterator last);

        reverse_iterator erase(const_reverse_iterator position);

        reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

        void clear() noexcept;

        void reset_lose_memory() noexcept;

        // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        size_type remove(const T &value);

        template<typename Predicate>
        size_type remove_if(Predicate);

        void reverse() noexcept;

        // splice inserts elements in the range [first,last) before position and removes the elements from x.
        // In the case that the two containers' allocators are unequal, splice copies elements
        // instead of splicing them. In this case elements are not removed from x, and iterators
        // into the spliced elements from x continue to point to the original values in x.
        void splice(const_iterator position, this_type &x);

        void splice(const_iterator position, this_type &x, const_iterator i);

        void splice(const_iterator position, this_type &x, const_iterator first, const_iterator last);

        void splice(const_iterator position, this_type &&x);

        void splice(const_iterator position, this_type &&x, const_iterator i);

        void splice(const_iterator position, this_type &&x, const_iterator first, const_iterator last);

    public:
        // For merge, see notes for splice regarding the handling of unequal allocators.
        void merge(this_type &x);

        void merge(this_type &&x);

        template<typename Compare>
        void merge(this_type &x, Compare compare);

        template<typename Compare>
        void merge(this_type &&x, Compare compare);

        size_type unique();

        template<typename BinaryPredicate>
        size_type unique(BinaryPredicate);

        // Sorting functionality
        // This is independent of the global sort algorithms, as lists are
        // linked nodes and can be sorted more efficiently by moving nodes
        // around in ways that global sort algorithms aren't privy to.
        void sort();

        template<typename Compare>
        void sort(Compare compare);

    public:
        [[nodiscard]] bool validate() const;

    protected:
        node_type *do_create_node();

        template<typename... Args>
        node_type *do_create_node(Args &&... args);

        template<typename Integer>
        void do_assign(Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void do_assign(InputIterator first, InputIterator last, std::false_type);

        void DoAssignValues(size_type n, const value_type &value);

        template<typename Integer>
        void DoInsert(ListNodeBase *pNode, Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void DoInsert(ListNodeBase *pNode, InputIterator first, InputIterator last, std::false_type);

        void DoInsertValues(ListNodeBase *pNode, size_type n, const value_type &value);

        template<typename... Args>
        void DoInsertValue(ListNodeBase *pNode, Args &&... args);

        void do_erase(ListNodeBase *pNode);

        void do_swap(this_type &x);

        template<typename Compare>
        iterator DoSort(iterator i1, iterator end2, size_type n, Compare &compare);
    }; // class List


    ///////////////////////////////////////////////////////////////////////
    // ListNodeBase
    ///////////////////////////////////////////////////////////////////////

    // Swaps the nodes a and b in the lists to which they belong. This is similar to
    // splicing a into b's List and b into a's List at the same time.
    // Works by swapping the members of a and b, and fixes up the lists that a and b
    // were part of to point to the new members.
    inline void ListNodeBase::swap(ListNodeBase &a, ListNodeBase &b) noexcept {
        const ListNodeBase temp(a);
        a = b;
        b = temp;

        if (a._next == &b)
            a._next = a._prev = &a;
        else
            a._next->_prev = a._prev->_next = &a;

        if (b._next == &a)
            b._next = b._prev = &b;
        else
            b._next->_prev = b._prev->_next = &b;
    }


    // splices the [first,last) range from its current List into our List before this node.
    inline void ListNodeBase::splice(ListNodeBase *first, ListNodeBase *last) noexcept {
        // We assume that [first, last] are not within our List.
        last->_prev->_next = this;
        first->_prev->_next = last;
        this->_prev->_next = first;

        ListNodeBase *const pTemp = this->_prev;
        this->_prev = last->_prev;
        last->_prev = first->_prev;
        first->_prev = pTemp;
    }


    inline void ListNodeBase::reverse() noexcept {
        ListNodeBase *pNode = this;
        do {
            DKCHECK(pNode != nullptr);
            ListNodeBase *const pTemp = pNode->_next;
            pNode->_next = pNode->_prev;
            pNode->_prev = pTemp;
            pNode = pNode->_prev;
        } while (pNode != this);
    }


    inline void ListNodeBase::insert(ListNodeBase *pNext) noexcept {
        _next = pNext;
        _prev = pNext->_prev;
        pNext->_prev->_next = this;
        pNext->_prev = this;
    }


    // Removes this node from the List that it's in. Assumes that the
    // node is within a List and thus that its prev/next pointers are valid.
    inline void ListNodeBase::remove() noexcept {
        _next->_prev = _prev;
        _prev->_next = _next;
    }


    // Inserts the standalone range [pFirst, pFinal] before pPosition. Assumes that the
    // range is not within a List and thus that it's prev/next pointers are not valid.
    // Assumes that this node is within a List and thus that its prev/next pointers are valid.
    inline void ListNodeBase::insert_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept {
        _prev->_next = pFirst;
        pFirst->_prev = _prev;
        _prev = pFinal;
        pFinal->_next = this;
    }


    // Removes the range [pFirst, pFinal] from the List that it's in. Assumes that the
    // range is within a List and thus that its prev/next pointers are valid.
    inline void ListNodeBase::remove_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept {
        pFinal->_next->_prev = pFirst->_prev;
        pFirst->_prev->_next = pFinal->_next;
    }


    ///////////////////////////////////////////////////////////////////////
    // ListIterator
    ///////////////////////////////////////////////////////////////////////

    template<typename T, typename Pointer, typename Reference>
    inline ListIterator<T, Pointer, Reference>::ListIterator() noexcept
        : mpNode() // To consider: Do we really need to intialize mpNode?
    {
        // Empty
    }


    template<typename T, typename Pointer, typename Reference>
    inline ListIterator<T, Pointer, Reference>::ListIterator(const ListNodeBase *pNode) noexcept
        : mpNode(const_cast<base_node_type *>(pNode)) {
        // Empty
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::next() const noexcept {
        return ListIterator(mpNode->_next);
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::prev() const noexcept {
        return ListIterator(mpNode->_prev);
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::reference
    ListIterator<T, Pointer, Reference>::operator*() const noexcept {
        return static_cast<node_type *>(mpNode)->mValue;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::pointer
    ListIterator<T, Pointer, Reference>::operator->() const noexcept {
        return &static_cast<node_type *>(mpNode)->mValue;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type &
    ListIterator<T, Pointer, Reference>::operator++() noexcept {
        mpNode = mpNode->_next;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::operator++(int) noexcept {
        this_type temp(*this);
        mpNode = mpNode->_next;
        return temp;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type &
    ListIterator<T, Pointer, Reference>::operator--() noexcept {
        mpNode = mpNode->_prev;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::operator--(int) noexcept {
        this_type temp(*this);
        mpNode = mpNode->_prev;
        return temp;
    }


    // The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
    // Thus we provide additional template paremeters here to support this. The defect report does not
    // require us to support comparisons between reverse_iterators and const_reverse_iterators.
    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator==(const ListIterator<T, PointerA, ReferenceA> &a,
                           const ListIterator<T, PointerB, ReferenceB> &b) noexcept {
        return a.mpNode == b.mpNode;
    }


    template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
    inline bool operator!=(const ListIterator<T, PointerA, ReferenceA> &a,
                           const ListIterator<T, PointerB, ReferenceB> &b) noexcept {
        return a.mpNode != b.mpNode;
    }


    // We provide a version of operator!= for the case where the iterators are of the
    // same type. This helps prevent ambiguity errors in the presence of rel_ops.
    template<typename T, typename Pointer, typename Reference>
    inline bool operator!=(const ListIterator<T, Pointer, Reference> &a,
                           const ListIterator<T, Pointer, Reference> &b) noexcept {
        return a.mpNode != b.mpNode;
    }


    ///////////////////////////////////////////////////////////////////////
    // ListBase
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline ListBase<T, Alignment, Allocator>::ListBase()
        : _node_allocator(base_node_type(), allocator_type{})
          , _size(0) {
        do_init();
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline ListBase<T, Alignment, Allocator>::ListBase(const allocator_type &allocator)
        : _node_allocator(base_node_type(), allocator)
          , _size(0) {
        do_init();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline ListBase<T, Alignment, Allocator>::~ListBase() {
        do_clear();
    }


    template<typename T, size_t Alignment, typename Allocator>
    const typename ListBase<T, Alignment, Allocator>::allocator_type &
    ListBase<T, Alignment, Allocator>::get_allocator() const noexcept {
        return internal_allocator();
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename ListBase<T, Alignment, Allocator>::allocator_type &
    ListBase<T, Alignment, Allocator>::get_allocator() noexcept {
        return internal_allocator();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::set_allocator(const allocator_type &allocator) {
        if ((internal_allocator() != allocator) && (
                static_cast<node_type *>(internal_node()._next) != &internal_node()))
            throw std::logic_error("List::set_allocator -- cannot change allocator after allocations have been made.");
        internal_allocator() = allocator;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename ListBase<T, Alignment, Allocator>::node_type *
    ListBase<T, Alignment, Allocator>::do_allocate_node() {
        node_type *pNode = internal_allocator().allocate(1);
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::do_free_node(node_type *p) {
        internal_allocator().deallocate(p, 1);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::do_init() noexcept {
        internal_node()._next = &internal_node();
        internal_node()._prev = &internal_node();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::do_clear() {
        base_node_type *p = internal_node()._next;

        while (p != &internal_node()) {
            auto *pTemp = static_cast<node_type *>(p);
            p = p->_next;
            if constexpr (!std::is_trivially_destructible_v<value_type>) {
                pTemp->~node_type();
            }
            internal_allocator().deallocate(pTemp, 1);
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // List
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List()
        : base_type() {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(const allocator_type &allocator)
        : base_type(allocator) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(size_type n, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsertValues(&internal_node(), n, value_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(size_type n, const value_type &value, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsertValues(&internal_node(), n, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(const this_type &x)
        : base_type(x.internal_allocator()) {
        DoInsert(&internal_node(), const_iterator(x.internal_node()._next), const_iterator(&x.internal_node()),
                 std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(const this_type &x, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsert(&internal_node(), const_iterator(x.internal_node()._next), const_iterator(&x.internal_node()),
                 std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(this_type &&x) noexcept
        : base_type(std::move(x.internal_allocator())) {
        swap(x);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(this_type &&x, const allocator_type &allocator)
        : base_type(allocator) {
        swap(x); // member swap handles the case that x has a different allocator than our allocator by doing a copy.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline List<T, Alignment, Allocator>::List(std::initializer_list<value_type> ilist, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsert(&internal_node(), ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    List<T, Alignment, Allocator>::List(InputIterator first, InputIterator last)
        : base_type(allocator_type{}) {
        //insert(const_iterator(&internal_node()), first, last);
        DoInsert(&internal_node(), first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::iterator
    inline List<T, Alignment, Allocator>::begin() noexcept {
        return iterator(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_iterator
    List<T, Alignment, Allocator>::begin() const noexcept {
        return const_iterator(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_iterator
    List<T, Alignment, Allocator>::cbegin() const noexcept {
        return const_iterator(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::end() noexcept {
        return iterator(&internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_iterator
    List<T, Alignment, Allocator>::end() const noexcept {
        return const_iterator(&internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_iterator
    List<T, Alignment, Allocator>::cend() const noexcept {
        return const_iterator(&internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reverse_iterator
    List<T, Alignment, Allocator>::rbegin() noexcept {
        return reverse_iterator(&internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_reverse_iterator
    List<T, Alignment, Allocator>::rbegin() const noexcept {
        return const_reverse_iterator(&internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_reverse_iterator
    List<T, Alignment, Allocator>::crbegin() const noexcept {
        return const_reverse_iterator(&internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reverse_iterator
    List<T, Alignment, Allocator>::rend() noexcept {
        return reverse_iterator(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_reverse_iterator
    List<T, Alignment, Allocator>::rend() const noexcept {
        return const_reverse_iterator(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_reverse_iterator
    List<T, Alignment, Allocator>::crend() const noexcept {
        return const_reverse_iterator(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reference
    List<T, Alignment, Allocator>::front() {
        return static_cast<node_type *>(internal_node()._next)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_reference
    List<T, Alignment, Allocator>::front() const {
        return static_cast<node_type *>(internal_node()._next)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reference
    List<T, Alignment, Allocator>::back() {
        return static_cast<node_type *>(internal_node()._prev)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::const_reference
    List<T, Alignment, Allocator>::back() const {
        return static_cast<node_type *>(internal_node()._prev)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool List<T, Alignment, Allocator>::empty() const noexcept {
        return (_size == 0);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::size_type
    List<T, Alignment, Allocator>::size() const noexcept {
        return _size;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::this_type &
    List<T, Alignment, Allocator>::operator=(const this_type &x) {
        if (TURBO_UNLIKELY(this != &x)) {
            do_assign(x.begin(), x.end(), std::false_type());
        }

        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::this_type &
    List<T, Alignment, Allocator>::operator=(this_type &&x) noexcept {
        if (this != &x) {
            clear();
            // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
            swap(x);
            // member swap handles the case that x has a different allocator than our allocator by doing a copy.
        }
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::this_type &
    List<T, Alignment, Allocator>::operator=(std::initializer_list<value_type> ilist) {
        do_assign(ilist.begin(), ilist.end(), std::false_type());
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::assign(size_type n, const value_type &value) {
        DoAssignValues(n, value);
    }


    // It turns out that the C++ std::List specifies a two argument
    // version of assign that takes (int size, int value). These are not
    // iterators, so we need to do a template compiler trick to do the right thing.
    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void List<T, Alignment, Allocator>::assign(InputIterator first, InputIterator last) {
        do_assign(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::assign(std::initializer_list<value_type> ilist) {
        do_assign(ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::clear() noexcept {
        do_clear();
        do_init();
        _size = 0;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::reset_lose_memory() noexcept {
        // The reset_lose_memory function is a special extension function which unilaterally
        // resets the container to an empty state without freeing the memory of
        // the contained objects. This is useful for very quickly tearing down a
        // container built into scratch memory.
        do_init();
        _size = 0;
    }


    template<typename T, size_t Alignment, typename Allocator>
    void List<T, Alignment, Allocator>::resize(size_type n, const value_type &value) {
        iterator current(internal_node()._next);
        size_type i = 0;

        while ((current.mpNode != &internal_node()) && (i < n)) {
            ++current;
            ++i;
        }
        if (i == n)
            erase(current, &internal_node());
        else
            insert(&internal_node(), n - i, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::resize(size_type n) {
        resize(n, value_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    typename List<T, Alignment, Allocator>::reference List<T, Alignment, Allocator>::emplace_front(Args &&... args) {
        DoInsertValue(internal_node()._next, std::forward<Args>(args)...);
        return static_cast<node_type *>(internal_node()._next)->mValue; // Same as return front();
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    typename List<T, Alignment, Allocator>::reference List<T, Alignment, Allocator>::emplace_back(Args &&... args) {
        DoInsertValue(&internal_node(), std::forward<Args>(args)...);
        return static_cast<node_type *>(internal_node()._prev)->mValue; // Same as return back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::push_front(const value_type &value) {
        DoInsertValue(internal_node()._next, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::push_front(value_type &&value) noexcept {
        emplace(begin(), std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reference
    List<T, Alignment, Allocator>::push_front() {
        node_type *const pNode = do_create_node();
        pNode->insert(internal_node()._next);
        ++_size;
        return static_cast<node_type *>(internal_node()._next)->mValue; // Same as return front();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void *List<T, Alignment, Allocator>::push_front_uninitialized() {
        node_type *const pNode = do_allocate_node();
        pNode->insert(internal_node()._next);
        ++_size;
        return &pNode->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::pop_front() {
        do_erase(internal_node()._next);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::push_back(const value_type &value) {
        DoInsertValue(&internal_node(), value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::push_back(value_type &&value) noexcept {
        emplace(end(), std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reference
    List<T, Alignment, Allocator>::push_back() {
        node_type *const pNode = do_create_node();
        pNode->insert(&internal_node());
        ++_size;
        return static_cast<node_type *>(internal_node()._prev)->mValue; // Same as return back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void *List<T, Alignment, Allocator>::push_back_uninitialized() {
        node_type *const pNode = do_allocate_node();
        pNode->insert(&internal_node());
        ++_size;
        return &pNode->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::pop_back() {
        do_erase(internal_node()._prev);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::emplace(const_iterator position, Args &&... args) {
        DoInsertValue(position.mpNode, std::forward<Args>(args)...);
        return iterator(position.mpNode->_prev);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::insert(const_iterator position) {
        ListNodeBase *const pNode = do_create_node(value_type());
        pNode->insert(position.mpNode);
        ++_size;
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::insert(const_iterator position, const value_type &value) {
        ListNodeBase *const pNode = do_create_node(value);
        pNode->insert(position.mpNode);
        ++_size;
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::insert(const_iterator position, value_type &&value) {
        return emplace(position, std::move(value));
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::insert(const_iterator position, size_type n, const value_type &value) {
        iterator itPrev(position.mpNode);
        --itPrev;
        DoInsertValues(position.mpNode, n, value);
        return ++itPrev; // Inserts in front of position, returns iterator to new elements.
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::insert(const_iterator position, InputIterator first, InputIterator last) {
        iterator itPrev(position.mpNode);
        --itPrev;
        DoInsert(position.mpNode, first, last, std::is_integral<InputIterator>());
        return ++itPrev; // Inserts in front of position, returns iterator to new elements.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        iterator itPrev(position.mpNode);
        --itPrev;
        DoInsert(position.mpNode, ilist.begin(), ilist.end(), std::false_type());
        return ++itPrev; // Inserts in front of position, returns iterator to new elements.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::erase(const_iterator position) {
        ++position;
        do_erase(position.mpNode->_prev);
        return iterator(position.mpNode);
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::erase(const_iterator first, const_iterator last) {
        while (first != last)
            first = erase(first);
        return iterator(last.mpNode);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::reverse_iterator
    List<T, Alignment, Allocator>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::reverse_iterator
    List<T, Alignment, Allocator>::erase(const_reverse_iterator first, const_reverse_iterator last) {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        const_iterator itLastBase((++last).base());
        const_iterator itFirstBase((++first).base());

        return reverse_iterator(erase(itLastBase, itFirstBase));
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::size_type List<T, Alignment, Allocator>::remove(const value_type &value) {
        iterator current(internal_node()._next);
        size_type numRemoved = 0;

        while (current.mpNode != &internal_node()) {
            if (TURBO_LIKELY(!(*current == value)))
                ++current; // We have duplicate '++current' statements here and below, but the logic here forces this.
            else {
                ++current;
                do_erase(current.mpNode->_prev);
                ++numRemoved;
            }
        }
        return numRemoved;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Predicate>
    inline typename List<T, Alignment, Allocator>::size_type List<T, Alignment, Allocator>::remove_if(
        Predicate predicate) {
        size_type numRemoved = 0;
        for (iterator first(internal_node()._next), last(&internal_node()); first != last;) {
            iterator temp(first);
            ++temp;
            if (predicate(static_cast<node_type *>(first.mpNode)->mValue)) {
                do_erase(first.mpNode);
                ++numRemoved;
            }
            first = temp;
        }
        return numRemoved;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::reverse() noexcept {
        ((ListNodeBase &) internal_node()).reverse();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::splice(const_iterator position, this_type &x) {
        // Splicing operations cannot succeed if the two containers use unequal allocators.
        // This issue is not addressed in the C++ 1998 standard but is discussed in the
        // LWG defect reports, such as #431. There is no simple solution to this problem.
        // One option is to throw an exception. Another option which probably captures the
        // user intent most of the time is to copy the range from the source to the dest and
        // remove it from the source.

        if (internal_allocator() == x.internal_allocator()) {
            if (x._size) {
                (position.mpNode)->splice(x.internal_node()._next, &x.internal_node());
                _size += x._size;
                x._size = 0;
            }
        } else {
            insert(position, x.begin(), x.end());
            x.clear();
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::splice(const_iterator position, this_type &&x) {
        return splice(position, x); // This will call splice(const_iterator, const this_type&);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::splice(const_iterator position, List &x, const_iterator i) {
        if (internal_allocator() == x.internal_allocator()) {
            iterator i2(i.mpNode);
            ++i2;
            if ((position != i) && (position != i2)) {
                (position.mpNode)->splice(i.mpNode, i2.mpNode);

                ++_size;
                --x._size;
            }
        } else {
            insert(position, *i);
            x.erase(i);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::splice(const_iterator position, List<T, Alignment, Allocator> &&x,
                                                      const_iterator i) {
        return splice(position, x, i); // This will call splice(const_iterator, const this_type&, const_iterator);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::splice(const_iterator position, this_type &x, const_iterator first,
                                                      const_iterator last) {
        if (internal_allocator() == x.internal_allocator()) {
            auto const n = (size_type) std::distance(first, last);

            if (n) {
                (position.mpNode)->splice(first.mpNode, last.mpNode);
                _size += n;
                x._size -= n;
            }
        } else {
            insert(position, first, last);
            x.erase(first, last);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::splice(const_iterator position, List<T, Alignment, Allocator> &&x,
                                                      const_iterator first,
                                                      const_iterator last) {
        return splice(position, x, first, last);
        // This will call splice(const_iterator, const this_type&, const_iterator, const_iterator);
    }


    // does not propagate allocators on swap.
    // in addition, requires T be copy constructible and copy assignable, which isn't required by the standard.
    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::swap(this_type &x) noexcept {
        if (internal_allocator() == x.internal_allocator()) // If allocators are equivalent...
            do_swap(x);
        else // else swap the contents.
        {
            const this_type temp(*this); // Can't call std::swap because that would
            *this = x; // itself call this member swap function.
            x = temp;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void List<T, Alignment, Allocator>::merge(this_type &x) {
        if (this != &x) {
            iterator first(begin());
            iterator firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while ((first != last) && (firstX != lastX)) {
                if (*firstX < *first) {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                } else
                    ++first;
            }

            if (firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void List<T, Alignment, Allocator>::merge(this_type &&x) {
        return merge(x); // This will call merge(this_type&)
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    void List<T, Alignment, Allocator>::merge(this_type &x, Compare compare) {
        if (this != &x) {
            iterator first(begin());
            iterator firstX(x.begin());
            const iterator last(end());
            const iterator lastX(x.end());

            while ((first != last) && (firstX != lastX)) {
                if (compare(*firstX, *first)) {
                    iterator next(firstX);

                    splice(first, x, firstX, ++next);
                    firstX = next;
                } else
                    ++first;
            }

            if (firstX != lastX)
                splice(last, x, firstX, lastX);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    void List<T, Alignment, Allocator>::merge(this_type &&x, Compare compare) {
        return merge(x, compare); // This will call merge(this_type&, Compare)
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename List<T, Alignment, Allocator>::size_type List<T, Alignment, Allocator>::unique() {
        size_type numRemoved = 0;
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (*first == *next) {
                    do_erase(next.mpNode);
                    ++numRemoved;
                    next = first;
                } else {
                    first = next;
                }
            }
        }

        return numRemoved;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename BinaryPredicate>
    typename List<T, Alignment, Allocator>::size_type List<T, Alignment, Allocator>::unique(BinaryPredicate predicate) {
        size_type numRemoved = 0;
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (predicate(*first, *next)) {
                    do_erase(next.mpNode);
                    ++numRemoved;
                    next = first;
                } else {
                    first = next;
                }
            }
        }

        return numRemoved;
    }


    template<typename T, size_t Alignment, typename Allocator>
    void List<T, Alignment, Allocator>::sort() {
        std::less<value_type> compare;
        DoSort(begin(), end(), size(), compare);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    void List<T, Alignment, Allocator>::sort(Compare compare) {
        DoSort(begin(), end(), size(), compare);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    typename List<T, Alignment, Allocator>::iterator
    List<T, Alignment, Allocator>::DoSort(iterator i1, iterator end2, size_type n, Compare &compare) {
        // A previous version of this function did this by creating temporary lists,
        // but that was incompatible with fixed_list because the sizes could be too big.
        // We sort subsegments by recursive descent. Then merge as we ascend.
        // Return an iterator to the beginning of the sorted subsegment.
        // Start with a special case for small node counts.
        switch (n) {
            case 0:
            case 1:
                return i1;

            case 2:
                // Potentialy swap these two nodes and return the resulting first of them.
                if (compare(*--end2, *i1)) {
                    end2.mpNode->remove();
                    end2.mpNode->insert(i1.mpNode);
                    return end2;
                }
                return i1;

            case 3: {
                // We do a List insertion sort. Measurements showed this improved performance 3-12%.
                iterator lowest = i1;

                for (iterator current = i1.next(); current != end2; ++current) {
                    if (compare(*current, *lowest))
                        lowest = current;
                }

                if (lowest == i1)
                    ++i1;
                else {
                    lowest.mpNode->remove();
                    lowest.mpNode->insert(i1.mpNode);
                }

                if (compare(*--end2, *i1))
                // At this point, i1 refers to the second element in this three element segment.
                {
                    end2.mpNode->remove();
                    end2.mpNode->insert(i1.mpNode);
                }

                return lowest;
            }
            default:
                TURBO_UNREACHABLE();
        }

        // Divide the range into two parts are recursively sort each part. Upon return we will have
        // two halves that are each sorted but we'll need to merge the two together before returning.
        iterator result;
        size_type nMid = (n / 2);
        iterator end1 = std::next(i1, (difference_type) nMid);
        i1 = DoSort(i1, end1, nMid, compare); // Return the new beginning of the first sorted sub-range.
        iterator i2 = DoSort(end1, end2, n - nMid, compare); // Return the new beginning of the second sorted sub-range.

        // If the start of the second List is before the start of the first List, insert the first List
        // into the second at an appropriate starting place.
        if (compare(*i2, *i1)) {
            // Find the position to insert the first List into the second List.
            iterator ix = i2.next();
            while ((ix != end2) && compare(*ix, *i1))
                ++ix;

            // Cut out the initial segment of the second List and move it to be in front of the first List.
            ListNodeBase *i2Cut = i2.mpNode;
            ListNodeBase *i2CutLast = ix.mpNode->_prev;
            result = i2;
            end1 = i2 = ix;
            ListNodeBase::remove_range(i2Cut, i2CutLast);
            i1.mpNode->insert_range(i2Cut, i2CutLast);
        } else {
            result = i1;
            end1 = i2;
        }

        // Merge the two segments. We do this by merging the second sub-segment into the first, by walking forward in each of the two sub-segments.
        for (++i1; (i1 != end1) && (i2 != end2); ++i1) // while still working on either segment...
        {
            if (compare(*i2, *i1)) // If i2 is less than i1 and it needs to be merged in front of i1...
            {
                // Find the position to insert the i2 List into the i1 List.
                iterator ix = i2.next();
                while ((ix != end2) && compare(*ix, *i1))
                    ++ix;

                // Cut this section of the i2 sub-segment out and merge into the appropriate place in the i1 List.
                ListNodeBase *i2Cut = i2.mpNode;
                ListNodeBase *i2CutLast = ix.mpNode->_prev;
                if (end1 == i2)
                    end1 = ix;
                i2 = ix;
                ListNodeBase::remove_range(i2Cut, i2CutLast);
                i1.mpNode->insert_range(i2Cut, i2CutLast);
            }
        }

        return result;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    inline typename List<T, Alignment, Allocator>::node_type *
    List<T, Alignment, Allocator>::do_create_node(Args &&... args) {
        node_type *const pNode = do_allocate_node(); // pNode is of type node_type, but it's uninitialized memory.
        fermat::construct_at(&pNode->mValue, std::forward<Args>(args)...);
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename List<T, Alignment, Allocator>::node_type *
    List<T, Alignment, Allocator>::do_create_node() {
        node_type *const pNode = do_allocate_node();
        fermat::construct_at(&pNode->mValue);
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void List<T, Alignment, Allocator>::do_assign(Integer n, Integer value, std::true_type) {
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    void List<T, Alignment, Allocator>::do_assign(InputIterator first, InputIterator last, std::false_type) {
        auto *pNode = static_cast<node_type *>(internal_node()._next);

        for (; (pNode != &internal_node()) && (first != last); ++first) {
            pNode->mValue = *first;
            pNode = static_cast<node_type *>(pNode->_next);
        }

        if (first == last)
            erase(const_iterator(pNode), &internal_node());
        else
            DoInsert(&internal_node(), first, last, std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    void List<T, Alignment, Allocator>::DoAssignValues(size_type n, const value_type &value) {
        auto *pNode = static_cast<node_type *>(internal_node()._next);

        for (; (pNode != &internal_node()) && (n > 0); --n) {
            pNode->mValue = value;
            pNode = static_cast<node_type *>(pNode->_next);
        }

        if (n)
            DoInsertValues(&internal_node(), n, value);
        else
            erase(const_iterator(pNode), &internal_node());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void List<T, Alignment, Allocator>::DoInsert(ListNodeBase *pNode, Integer n, Integer value, std::true_type) {
        DoInsertValues(pNode, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void List<T, Alignment, Allocator>::DoInsert(ListNodeBase *pNode, InputIterator first, InputIterator last,
                                                        std::false_type) {
        for (; first != last; ++first)
            DoInsertValue(pNode, *first);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment,
        Allocator>::DoInsertValues(ListNodeBase *pNode, size_type n, const value_type &value) {
        for (; n > 0; --n)
            DoInsertValue(pNode, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    inline void List<T, Alignment, Allocator>::DoInsertValue(ListNodeBase *pNode, Args &&... args) {
        node_type *const pNodeNew = do_create_node(std::forward<Args>(args)...);
        pNodeNew->insert(pNode);
        ++_size;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::do_erase(ListNodeBase *pNode) {
        pNode->remove();
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            ((node_type *) pNode)->~node_type();
        }

        do_free_node(((node_type *) pNode));
        --_size;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void List<T, Alignment, Allocator>::do_swap(this_type &x) {
        ListNodeBase::swap((ListNodeBase &) internal_node(), (ListNodeBase &) x.internal_node());
        // We need to implement a special swap because we can't do a shallow swap.
        std::swap(internal_allocator(), x.internal_allocator());
        std::swap(_size, x._size);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool List<T, Alignment, Allocator>::validate() const {
        size_type n = 0;

        for (const_iterator i(begin()), iEnd(end()); i != iEnd; ++i)
            ++n;

        if (n != _size)
            return false;

        // To do: More validation.
        return true;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    bool operator==(const List<T, Alignment, Allocator> &a, const List<T, Alignment, Allocator> &b) {
        typename List<T, Alignment, Allocator>::const_iterator ia = a.begin();
        typename List<T, Alignment, Allocator>::const_iterator ib = b.begin();
        typename List<T, Alignment, Allocator>::const_iterator enda = a.end();

        if (a.size() == b.size()) {
            while ((ia != enda) && (*ia == *ib)) {
                ++ia;
                ++ib;
            }
            return (ia == enda);
        }
        return false;
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator<(const List<T, Alignment, Allocator> &a, const List<T, Alignment, Allocator> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator!=(const List<T, Alignment, Allocator> &a, const List<T, Alignment, Allocator> &b) {
        return !(a == b);
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator>(const List<T, Alignment, Allocator> &a, const List<T, Alignment, Allocator> &b) {
        return b < a;
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator<=(const List<T, Alignment, Allocator> &a, const List<T, Alignment, Allocator> &b) {
        return !(b < a);
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator>=(const List<T, Alignment, Allocator> &a, const List<T, Alignment, Allocator> &b) {
        return !(a < b);
    }

    template<typename T, size_t Alignment, typename Allocator>
    void swap(List<T, Alignment, Allocator> &a, List<T, Alignment, Allocator> &b) noexcept {
        a.swap(b);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase / erase_if
    //
    // https://en.cppreference.com/w/cpp/container/List/erase2
    ///////////////////////////////////////////////////////////////////////
    template<class T, size_t Alignment, class Allocator, class U>
    typename List<T, Alignment, Allocator>::size_type erase(List<T, Alignment, Allocator> &c, const U &value) {
        // Erases all elements that compare equal to value from the container.
        return c.remove(value);
    }

    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename List<T, Alignment, Allocator>::size_type erase_if(List<T, Alignment, Allocator> &c, Predicate predicate) {
        // Erases all elements that satisfy the predicate pred from the container.
        return c.remove_if(predicate);
    }
} // namespace fermat
