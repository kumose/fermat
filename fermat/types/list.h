///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_LIST_H
#define FERMAT_LIST_H


#include <fermat/types/internal/config.h>
#include <fermat/memory/allocator.h>
#include <fermat/types/type_traits.h>
#include <iterator>
#include <fermat/types/algorithm.h>
#include <initializer_list>
#include <fermat/types/memory.h>
#include <fermat/types/compressed_pair.h>
#if FERMAT_EXCEPTIONS_ENABLED
#include <stdexcept>
#endif

KM_DISABLE_ALL_VC_WARNINGS()

#include <new>
#include <cstddef>

KM_RESTORE_ALL_VC_WARNINGS()


// 4530 - C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
// 4345 - Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
// 4571 - catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
// 4623 - default constructor was implicitly defined as deleted
KM_DISABLE_VC_WARNING (
4530
4345
4571
4623
);


#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.

#endif


namespace fermat {
    /// FERMAT_LIST_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
#ifndef FERMAT_LIST_DEFAULT_NAME
#define FERMAT_LIST_DEFAULT_NAME FERMAT_DEFAULT_NAME_PREFIX " list" // Unless the user overrides something, this is "EASTL list".

#endif


    /// FERMAT_LIST_DEFAULT_ALLOCATOR
	///
#ifndef FERMAT_LIST_DEFAULT_ALLOCATOR
#define FERMAT_LIST_DEFAULT_ALLOCATOR allocator_type(FERMAT_LIST_DEFAULT_NAME)
#endif


    /// ListNodeBase
	///
	/// We define a ListNodeBase separately from ListNode (below), because it allows
	/// us to have non-templated operations such as insert, remove (below), and it
	/// makes it so that the list anchor node doesn't carry a T with it, which would
	/// waste space and possibly lead to surprising the user due to extra Ts existing
	/// that the user didn't explicitly create. The downside to all of this is that
	/// it makes debug viewing of a list harder, given that the node pointers are of
	/// type ListNodeBase and not ListNode. However, see ListNodeBaseProxy below.
	///
    struct ListNodeBase {
        ListNodeBase *mpNext;
        ListNodeBase *mpPrev;

        void insert(ListNodeBase *pNext) noexcept;

        // Inserts this standalone node before the node pNext in pNext's list.
        void remove() noexcept; // Removes this node from the list it's in. Leaves this node's mpNext/mpPrev invalid.
        void splice(ListNodeBase *pFirst, ListNodeBase *pLast) noexcept;

        // Removes [pFirst,pLast) from the list it's in and inserts it before this in this node's list.
        void reverse() noexcept; // Reverses the order of nodes in the circular list this node is a part of.
        static void swap(ListNodeBase &a, ListNodeBase &b) noexcept;

        // Swaps the nodes a and b in the lists to which they belong.

        void insert_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept;

        // Differs from splice in that first/final aren't in another list.
        static void remove_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept; //
    };

    KM_DISABLE_VC_WARNING (
    4625
    4626
    )
    template<typename T>
    struct ListNode : public ListNodeBase {
        T mValue;
    };

    KM_RESTORE_VC_WARNING()

    /// ListIterator
	///
    template<typename T, typename Pointer, typename Reference>
    struct ListIterator {
        typedef ListIterator<T, Pointer, Reference> this_type;
        typedef ListIterator<T, T *, T &> iterator;
        typedef ListIterator<T, const T *, const T &> const_iterator;
        typedef size_t size_type; // See config.h for the definition of size_t, which defaults to size_t.
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
        typedef size_t size_type; // See config.h for the definition of size_t, which defaults to size_t.
        typedef ptrdiff_t difference_type;
        typedef ListNodeBase base_node_type;
        // We use ListNodeBase instead of ListNode<T> because we don't want to create a T.

    protected:
        fermat::compressed_pair<base_node_type, allocator_type> mNodeAllocator;
        size_type mSize;

        base_node_type &internalNode() noexcept { return mNodeAllocator.first(); }
        base_node_type const &internalNode() const noexcept { return mNodeAllocator.first(); }
        allocator_type &internalAllocator() noexcept { return mNodeAllocator.second(); }
        const allocator_type &internalAllocator() const noexcept { return mNodeAllocator.second(); }

    public:
        const allocator_type &get_allocator() const noexcept;

        allocator_type &get_allocator() noexcept;

        void set_allocator(const allocator_type &allocator);

    protected:
        ListBase();

        ListBase(const allocator_type &a);

        ~ListBase();

        node_type *DoAllocateNode();

        void DoFreeNode(node_type *pNode);

        void DoInit() noexcept;

        void DoClear();
    }; // ListBase

    template<typename T, size_t Alignment, typename Allocator = BasicAllocator<T, Alignment>>
    class list : public ListBase<T, Alignment, Allocator> {
        typedef ListBase<T, Alignment, Allocator> base_type;
        typedef list<T, Alignment, Allocator> this_type;

    protected:
        using base_type::mNodeAllocator;
        using base_type::DoAllocateNode;
        using base_type::DoFreeNode;
        using base_type::DoClear;
        using base_type::DoInit;
        using base_type::mSize;
        using base_type::internalNode;
        using base_type::internalAllocator;

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

        static_assert(!std::is_const<value_type>::value, "vector<T> value_type must be non-const.");
        static_assert(!std::is_volatile<value_type>::value, "vector<T> value_type must be non-volatile.");

    public:
        list();

        list(const allocator_type &allocator);

        explicit list(size_type n, const allocator_type &allocator = FERMAT_LIST_DEFAULT_ALLOCATOR);

        list(size_type n, const value_type &value, const allocator_type &allocator = FERMAT_LIST_DEFAULT_ALLOCATOR);

        list(const this_type &x);

        list(const this_type &x, const allocator_type &allocator);

        list(this_type &&x);

        list(this_type &&, const allocator_type &);

        list(std::initializer_list<value_type> ilist, const allocator_type &allocator = FERMAT_LIST_DEFAULT_ALLOCATOR);

        template<typename InputIterator>
        list(InputIterator first, InputIterator last);

        // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

        this_type &operator=(const this_type &x);

        this_type &operator=(std::initializer_list<value_type> ilist);

        this_type &operator=(this_type &&x);

        // In the case that the two containers' allocators are unequal, swap copies elements instead
        // of replacing them in place. In this case swap is an O(n) operation instead of O(1).
        void swap(this_type &x);

        void assign(size_type n, const value_type &value);

        template<typename InputIterator> // It turns out that the C++ std::list specifies a two argument
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

        bool empty() const noexcept;

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

        void push_front(value_type &&x);

        reference push_front();

        void *push_front_uninitialized();

        void push_back(const value_type &value);

        void push_back(value_type &&x);

        reference push_back();

        void *push_back_uninitialized();

        void pop_front();

        void pop_back();

        template<typename... Args>
        iterator emplace(const_iterator position, Args &&... args);

        iterator insert(const_iterator position);

        iterator insert(const_iterator position, const value_type &value);

        iterator insert(const_iterator position, value_type &&x);

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

        size_type remove(const T &x);

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
        bool validate() const;


    protected:
        node_type *DoCreateNode();

        template<typename... Args>
        node_type *DoCreateNode(Args &&... args);

        template<typename Integer>
        void DoAssign(Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void DoAssign(InputIterator first, InputIterator last, std::false_type);

        void DoAssignValues(size_type n, const value_type &value);

        template<typename Integer>
        void DoInsert(ListNodeBase *pNode, Integer n, Integer value, std::true_type);

        template<typename InputIterator>
        void DoInsert(ListNodeBase *pNode, InputIterator first, InputIterator last, std::false_type);

        void DoInsertValues(ListNodeBase *pNode, size_type n, const value_type &value);

        template<typename... Args>
        void DoInsertValue(ListNodeBase *pNode, Args &&... args);

        void DoErase(ListNodeBase *pNode);

        void DoSwap(this_type &x);

        template<typename Compare>
        iterator DoSort(iterator i1, iterator end2, size_type n, Compare &compare);
    }; // class list


    ///////////////////////////////////////////////////////////////////////
    // ListNodeBase
    ///////////////////////////////////////////////////////////////////////

    // Swaps the nodes a and b in the lists to which they belong. This is similar to
    // splicing a into b's list and b into a's list at the same time.
    // Works by swapping the members of a and b, and fixes up the lists that a and b
    // were part of to point to the new members.
    inline void ListNodeBase::swap(ListNodeBase &a, ListNodeBase &b) noexcept {
        const ListNodeBase temp(a);
        a = b;
        b = temp;

        if (a.mpNext == &b)
            a.mpNext = a.mpPrev = &a;
        else
            a.mpNext->mpPrev = a.mpPrev->mpNext = &a;

        if (b.mpNext == &a)
            b.mpNext = b.mpPrev = &b;
        else
            b.mpNext->mpPrev = b.mpPrev->mpNext = &b;
    }


    // splices the [first,last) range from its current list into our list before this node.
    inline void ListNodeBase::splice(ListNodeBase *first, ListNodeBase *last) noexcept {
        // We assume that [first, last] are not within our list.
        last->mpPrev->mpNext = this;
        first->mpPrev->mpNext = last;
        this->mpPrev->mpNext = first;

        ListNodeBase *const pTemp = this->mpPrev;
        this->mpPrev = last->mpPrev;
        last->mpPrev = first->mpPrev;
        first->mpPrev = pTemp;
    }


    inline void ListNodeBase::reverse() noexcept {
        ListNodeBase *pNode = this;
        do {
            KM_ANALYSIS_ASSUME(pNode != NULL);
            ListNodeBase *const pTemp = pNode->mpNext;
            pNode->mpNext = pNode->mpPrev;
            pNode->mpPrev = pTemp;
            pNode = pNode->mpPrev;
        } while (pNode != this);
    }


    inline void ListNodeBase::insert(ListNodeBase *pNext) noexcept {
        mpNext = pNext;
        mpPrev = pNext->mpPrev;
        pNext->mpPrev->mpNext = this;
        pNext->mpPrev = this;
    }


    // Removes this node from the list that it's in. Assumes that the
    // node is within a list and thus that its prev/next pointers are valid.
    inline void ListNodeBase::remove() noexcept {
        mpNext->mpPrev = mpPrev;
        mpPrev->mpNext = mpNext;
    }


    // Inserts the standalone range [pFirst, pFinal] before pPosition. Assumes that the
    // range is not within a list and thus that it's prev/next pointers are not valid.
    // Assumes that this node is within a list and thus that its prev/next pointers are valid.
    inline void ListNodeBase::insert_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept {
        mpPrev->mpNext = pFirst;
        pFirst->mpPrev = mpPrev;
        mpPrev = pFinal;
        pFinal->mpNext = this;
    }


    // Removes the range [pFirst, pFinal] from the list that it's in. Assumes that the
    // range is within a list and thus that its prev/next pointers are valid.
    inline void ListNodeBase::remove_range(ListNodeBase *pFirst, ListNodeBase *pFinal) noexcept {
        pFinal->mpNext->mpPrev = pFirst->mpPrev;
        pFirst->mpPrev->mpNext = pFinal->mpNext;
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
        return ListIterator(mpNode->mpNext);
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::prev() const noexcept {
        return ListIterator(mpNode->mpPrev);
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
        mpNode = mpNode->mpNext;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::operator++(int) noexcept {
        this_type temp(*this);
        mpNode = mpNode->mpNext;
        return temp;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type &
    ListIterator<T, Pointer, Reference>::operator--() noexcept {
        mpNode = mpNode->mpPrev;
        return *this;
    }


    template<typename T, typename Pointer, typename Reference>
    inline typename ListIterator<T, Pointer, Reference>::this_type
    ListIterator<T, Pointer, Reference>::operator--(int) noexcept {
        this_type temp(*this);
        mpNode = mpNode->mpPrev;
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
        : mNodeAllocator(base_node_type(), allocator_type(FERMAT_LIST_DEFAULT_NAME))
		  , mSize(0)
    {
        DoInit();
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline ListBase<T, Alignment, Allocator>::ListBase(const allocator_type &allocator)
        : mNodeAllocator(base_node_type(), allocator)
		  , mSize(0)
    {
        DoInit();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline ListBase<T, Alignment, Allocator>::~ListBase() {
        DoClear();
    }


    template<typename T, size_t Alignment, typename Allocator>
    const typename ListBase<T, Alignment, Allocator>::allocator_type &
    ListBase<T, Alignment, Allocator>::get_allocator() const noexcept {
        return internalAllocator();
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename ListBase<T, Alignment, Allocator>::allocator_type &
    ListBase<T, Alignment, Allocator>::get_allocator() noexcept {
        return internalAllocator();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::set_allocator(const allocator_type &allocator) {
        if ((internalAllocator() != allocator) && (static_cast<node_type *>(internalNode().mpNext) != &internalNode()))
            FERMAT_THROW_MSG_OR_ASSERT(std::logic_error,
                                       "list::set_allocator -- cannot change allocator after allocations have been made.");
        internalAllocator() = allocator;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename ListBase<T, Alignment, Allocator>::node_type *
    ListBase<T, Alignment, Allocator>::DoAllocateNode() {
        node_type *pNode = (node_type *) allocate_memory(internalAllocator(), sizeof(node_type),
                                                         FERMAT_ALIGN_OF(node_type), 0);
        FERMAT_ASSERT(pNode != nullptr);
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::DoFreeNode(node_type *p) {
        EASTLFree(internalAllocator(), p, sizeof(node_type));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::DoInit() noexcept {
        internalNode().mpNext = &internalNode();
        internalNode().mpPrev = &internalNode();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void ListBase<T, Alignment, Allocator>::DoClear() {
        base_node_type *p = internalNode().mpNext;

        while (p != &internalNode()) {
            node_type *const pTemp = static_cast<node_type *>(p);
            p = p->mpNext;
            pTemp->~node_type();
            EASTLFree(internalAllocator(), pTemp, sizeof(node_type));
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // list
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list()
        : base_type() {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(const allocator_type &allocator)
        : base_type(allocator) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(size_type n, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsertValues(&internalNode(), n, value_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(size_type n, const value_type &value, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsertValues(&internalNode(), n, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(const this_type &x)
        : base_type(x.internalAllocator()) {
        DoInsert(&internalNode(), const_iterator(x.internalNode().mpNext), const_iterator(&x.internalNode()),
                 std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(const this_type &x, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsert(&internalNode(), const_iterator(x.internalNode().mpNext), const_iterator(&x.internalNode()),
                 std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(this_type &&x)
        : base_type(std::move(x.internalAllocator())) {
        swap(x);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(this_type &&x, const allocator_type &allocator)
        : base_type(allocator) {
        swap(x); // member swap handles the case that x has a different allocator than our allocator by doing a copy.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline list<T, Alignment, Allocator>::list(std::initializer_list<value_type> ilist, const allocator_type &allocator)
        : base_type(allocator) {
        DoInsert(&internalNode(), ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    list<T, Alignment, Allocator>::list(InputIterator first, InputIterator last)
        : base_type(FERMAT_LIST_DEFAULT_ALLOCATOR) {
        //insert(const_iterator(&internalNode()), first, last);
        DoInsert(&internalNode(), first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::iterator
    inline list<T, Alignment, Allocator>::begin() noexcept {
        return iterator(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_iterator
    list<T, Alignment, Allocator>::begin() const noexcept {
        return const_iterator(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_iterator
    list<T, Alignment, Allocator>::cbegin() const noexcept {
        return const_iterator(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::end() noexcept {
        return iterator(&internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_iterator
    list<T, Alignment, Allocator>::end() const noexcept {
        return const_iterator(&internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_iterator
    list<T, Alignment, Allocator>::cend() const noexcept {
        return const_iterator(&internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reverse_iterator
    list<T, Alignment, Allocator>::rbegin() noexcept {
        return reverse_iterator(&internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_reverse_iterator
    list<T, Alignment, Allocator>::rbegin() const noexcept {
        return const_reverse_iterator(&internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_reverse_iterator
    list<T, Alignment, Allocator>::crbegin() const noexcept {
        return const_reverse_iterator(&internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reverse_iterator
    list<T, Alignment, Allocator>::rend() noexcept {
        return reverse_iterator(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_reverse_iterator
    list<T, Alignment, Allocator>::rend() const noexcept {
        return const_reverse_iterator(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_reverse_iterator
    list<T, Alignment, Allocator>::crend() const noexcept {
        return const_reverse_iterator(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reference
    list<T, Alignment, Allocator>::front() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(static_cast<node_type *>(internalNode().mpNext) == &internalNode()))
            FERMAT_FAIL_MSG("list::front -- empty container");
#else
        // We allow the user to reference an empty container.
#endif

        return static_cast<node_type *>(internalNode().mpNext)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_reference
    list<T, Alignment, Allocator>::front() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(static_cast<node_type *>(internalNode().mpNext) == &internalNode()))
            FERMAT_FAIL_MSG("list::front -- empty container");
#else
        // We allow the user to reference an empty container.
#endif

        return static_cast<node_type *>(internalNode().mpNext)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reference
    list<T, Alignment, Allocator>::back() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(static_cast<node_type *>(internalNode().mpNext) == &internalNode()))
            FERMAT_FAIL_MSG("list::back -- empty container");
#else
        // We allow the user to reference an empty container.
#endif

        return static_cast<node_type *>(internalNode().mpPrev)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::const_reference
    list<T, Alignment, Allocator>::back() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(static_cast<node_type *>(internalNode().mpNext) == &internalNode()))
            FERMAT_FAIL_MSG("list::back -- empty container");
#else
        // We allow the user to reference an empty container.
#endif

        return static_cast<node_type *>(internalNode().mpPrev)->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool list<T, Alignment, Allocator>::empty() const noexcept {
        return (mSize == 0);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::size_type
    list<T, Alignment, Allocator>::size() const noexcept {
        return mSize;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::this_type &
    list<T, Alignment, Allocator>::operator=(const this_type &x) {
        if (this != &x) // If not assigning to self...
        {
            // If (FERMAT_ALLOCATOR_COPY_ENABLED == 1) and the current contents are allocated by an
            // allocator that's unequal to x's allocator, we need to reallocate our elements with
            // our current allocator and reallocate it with x's allocator. If the allocators are
            // equal then we can use a more optimal algorithm that doesn't reallocate our elements
            // but instead can copy them in place.

#if FERMAT_ALLOCATOR_COPY_ENABLED
            bool bSlowerPathwayRequired = (internalAllocator() != x.internalAllocator());
#else
            bool bSlowerPathwayRequired = false;
#endif

            if (bSlowerPathwayRequired) {
                clear();

#if FERMAT_ALLOCATOR_COPY_ENABLED
                internalAllocator() = x.internalAllocator();
#endif
            }

            DoAssign(x.begin(), x.end(), std::false_type());
        }

        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::this_type &
    list<T, Alignment, Allocator>::operator=(this_type &&x) {
        if (this != &x) {
            clear();
            // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
            swap(x);
            // member swap handles the case that x has a different allocator than our allocator by doing a copy.
        }
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::this_type &
    list<T, Alignment, Allocator>::operator=(std::initializer_list<value_type> ilist) {
        DoAssign(ilist.begin(), ilist.end(), std::false_type());
        return *this;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::assign(size_type n, const value_type &value) {
        DoAssignValues(n, value);
    }


    // It turns out that the C++ std::list specifies a two argument
    // version of assign that takes (int size, int value). These are not
    // iterators, so we need to do a template compiler trick to do the right thing.
    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void list<T, Alignment, Allocator>::assign(InputIterator first, InputIterator last) {
        DoAssign(first, last, std::is_integral<InputIterator>());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::assign(std::initializer_list<value_type> ilist) {
        DoAssign(ilist.begin(), ilist.end(), std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::clear() noexcept {
        DoClear();
        DoInit();
        mSize = 0;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::reset_lose_memory() noexcept {
        // The reset_lose_memory function is a special extension function which unilaterally
        // resets the container to an empty state without freeing the memory of
        // the contained objects. This is useful for very quickly tearing down a
        // container built into scratch memory.
        DoInit();
        mSize = 0;
    }


    template<typename T, size_t Alignment, typename Allocator>
    void list<T, Alignment, Allocator>::resize(size_type n, const value_type &value) {
        iterator current(internalNode().mpNext);
        size_type i = 0;

        while ((current.mpNode != &internalNode()) && (i < n)) {
            ++current;
            ++i;
        }
        if (i == n)
            erase(current, &internalNode());
        else
            insert(&internalNode(), n - i, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::resize(size_type n) {
        resize(n, value_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    typename list<T, Alignment, Allocator>::reference list<T, Alignment, Allocator>::emplace_front(Args &&... args) {
        DoInsertValue(internalNode().mpNext, std::forward<Args>(args)...);
        return static_cast<node_type *>(internalNode().mpNext)->mValue; // Same as return front();
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    typename list<T, Alignment, Allocator>::reference list<T, Alignment, Allocator>::emplace_back(Args &&... args) {
        DoInsertValue(&internalNode(), std::forward<Args>(args)...);
        return static_cast<node_type *>(internalNode().mpPrev)->mValue; // Same as return back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::push_front(const value_type &value) {
        DoInsertValue(internalNode().mpNext, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::push_front(value_type &&value) {
        emplace(begin(), std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reference
    list<T, Alignment, Allocator>::push_front() {
        node_type *const pNode = DoCreateNode();
        pNode->insert(internalNode().mpNext);
        ++mSize;
        return static_cast<node_type *>(internalNode().mpNext)->mValue; // Same as return front();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void *list<T, Alignment, Allocator>::push_front_uninitialized() {
        node_type *const pNode = DoAllocateNode();
        pNode->insert(internalNode().mpNext);
        ++mSize;
        return &pNode->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::pop_front() {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(static_cast<node_type *>(internalNode().mpNext) == &internalNode()))
            FERMAT_FAIL_MSG("list::pop_front -- empty container");
#endif

        DoErase(internalNode().mpNext);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::push_back(const value_type &value) {
        DoInsertValue(&internalNode(), value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::push_back(value_type &&value) {
        emplace(end(), std::move(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reference
    list<T, Alignment, Allocator>::push_back() {
        node_type *const pNode = DoCreateNode();
        pNode->insert(&internalNode());
        ++mSize;
        return static_cast<node_type *>(internalNode().mpPrev)->mValue; // Same as return back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void *list<T, Alignment, Allocator>::push_back_uninitialized() {
        node_type *const pNode = DoAllocateNode();
        pNode->insert(&internalNode());
        ++mSize;
        return &pNode->mValue;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::pop_back() {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(static_cast<node_type *>(internalNode().mpNext) == &internalNode()))
            FERMAT_FAIL_MSG("list::pop_back -- empty container");
#endif

        DoErase(internalNode().mpPrev);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::emplace(const_iterator position, Args &&... args) {
        DoInsertValue(position.mpNode, std::forward<Args>(args)...);
        return iterator(position.mpNode->mpPrev);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::insert(const_iterator position) {
        ListNodeBase *const pNode = DoCreateNode(value_type());
        pNode->insert(position.mpNode);
        ++mSize;
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::insert(const_iterator position, const value_type &value) {
        ListNodeBase *const pNode = DoCreateNode(value);
        pNode->insert(position.mpNode);
        ++mSize;
        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::insert(const_iterator position, value_type &&value) {
        return emplace(position, std::move(value));
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::insert(const_iterator position, size_type n, const value_type &value) {
        iterator itPrev(position.mpNode);
        --itPrev;
        DoInsertValues(position.mpNode, n, value);
        return ++itPrev; // Inserts in front of position, returns iterator to new elements.
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::insert(const_iterator position, InputIterator first, InputIterator last) {
        iterator itPrev(position.mpNode);
        --itPrev;
        DoInsert(position.mpNode, first, last, std::is_integral<InputIterator>());
        return ++itPrev; // Inserts in front of position, returns iterator to new elements.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::insert(const_iterator position, std::initializer_list<value_type> ilist) {
        iterator itPrev(position.mpNode);
        --itPrev;
        DoInsert(position.mpNode, ilist.begin(), ilist.end(), std::false_type());
        return ++itPrev; // Inserts in front of position, returns iterator to new elements.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::erase(const_iterator position) {
        ++position;
        DoErase(position.mpNode->mpPrev);
        return iterator(position.mpNode);
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::erase(const_iterator first, const_iterator last) {
        while (first != last)
            first = erase(first);
        return iterator(last.mpNode);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::reverse_iterator
    list<T, Alignment, Allocator>::erase(const_reverse_iterator position) {
        return reverse_iterator(erase((++position).base()));
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::reverse_iterator
    list<T, Alignment, Allocator>::erase(const_reverse_iterator first, const_reverse_iterator last) {
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
    typename list<T, Alignment, Allocator>::size_type list<T, Alignment, Allocator>::remove(const value_type &value) {
        iterator current(internalNode().mpNext);
        size_type numRemoved = 0;

        while (current.mpNode != &internalNode()) {
            if (FERMAT_LIKELY(!(*current == value)))
                ++current; // We have duplicate '++current' statements here and below, but the logic here forces this.
            else {
                ++current;
                DoErase(current.mpNode->mpPrev);
                ++numRemoved;
            }
        }
        return numRemoved;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Predicate>
    inline typename list<T, Alignment, Allocator>::size_type list<T, Alignment, Allocator>::remove_if(Predicate predicate) {
        size_type numRemoved = 0;
        for (iterator first(internalNode().mpNext), last(&internalNode()); first != last;) {
            iterator temp(first);
            ++temp;
            if (predicate(static_cast<node_type *>(first.mpNode)->mValue)) {
                DoErase(first.mpNode);
                ++numRemoved;
            }
            first = temp;
        }
        return numRemoved;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::reverse() noexcept {
        ((ListNodeBase &) internalNode()).reverse();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::splice(const_iterator position, this_type &x) {
        // Splicing operations cannot succeed if the two containers use unequal allocators.
        // This issue is not addressed in the C++ 1998 standard but is discussed in the
        // LWG defect reports, such as #431. There is no simple solution to this problem.
        // One option is to throw an exception. Another option which probably captures the
        // user intent most of the time is to copy the range from the source to the dest and
        // remove it from the source.

        if (internalAllocator() == x.internalAllocator()) {
            if (x.mSize) {
                (position.mpNode)->splice(x.internalNode().mpNext, &x.internalNode());
                mSize += x.mSize;
                x.mSize = 0;
            }
        } else {
            insert(position, x.begin(), x.end());
            x.clear();
        }
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::splice(const_iterator position, this_type &&x) {
        return splice(position, x); // This will call splice(const_iterator, const this_type&);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::splice(const_iterator position, list &x, const_iterator i) {
        if (internalAllocator() == x.internalAllocator()) {
            iterator i2(i.mpNode);
            ++i2;
            if ((position != i) && (position != i2)) {
                (position.mpNode)->splice(i.mpNode, i2.mpNode);

                ++mSize;
                --x.mSize;

            }
        } else {
            insert(position, *i);
            x.erase(i);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::splice(const_iterator position, list<T, Alignment, Allocator> &&x, const_iterator i) {
        return splice(position, x, i); // This will call splice(const_iterator, const this_type&, const_iterator);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::splice(const_iterator position, this_type &x, const_iterator first,
                                           const_iterator last) {
        if (internalAllocator() == x.internalAllocator()) {

            const size_type n = (size_type) std::distance(first, last);

            if (n) {
                (position.mpNode)->splice(first.mpNode, last.mpNode);
                mSize += n;
                x.mSize -= n;
            }
        } else {
            insert(position, first, last);
            x.erase(first, last);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::splice(const_iterator position, list<T, Alignment, Allocator> &&x, const_iterator first,
                                           const_iterator last) {
        return splice(position, x, first, last);
        // This will call splice(const_iterator, const this_type&, const_iterator, const_iterator);
    }


    // does not propagate allocators on swap.
    // in addition, requires T be copy constructible and copy assignable, which isn't required by the standard.
    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::swap(this_type &x) {
        if (internalAllocator() == x.internalAllocator()) // If allocators are equivalent...
            DoSwap(x);
        else // else swap the contents.
        {
            const this_type temp(*this); // Can't call std::swap because that would
            *this = x; // itself call this member swap function.
            x = temp;
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    void list<T, Alignment, Allocator>::merge(this_type &x) {
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
    void list<T, Alignment, Allocator>::merge(this_type &&x) {
        return merge(x); // This will call merge(this_type&)
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    void list<T, Alignment, Allocator>::merge(this_type &x, Compare compare) {
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
    void list<T, Alignment, Allocator>::merge(this_type &&x, Compare compare) {
        return merge(x, compare); // This will call merge(this_type&, Compare)
    }


    template<typename T, size_t Alignment, typename Allocator>
    typename list<T, Alignment, Allocator>::size_type list<T, Alignment, Allocator>::unique() {
        size_type numRemoved = 0;
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (*first == *next) {
                    DoErase(next.mpNode);
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
    typename list<T, Alignment, Allocator>::size_type list<T, Alignment, Allocator>::unique(BinaryPredicate predicate) {
        size_type numRemoved = 0;
        iterator first(begin());
        const iterator last(end());

        if (first != last) {
            iterator next(first);

            while (++next != last) {
                if (predicate(*first, *next)) {
                    DoErase(next.mpNode);
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
    void list<T, Alignment, Allocator>::sort() {
        std::less<value_type> compare;
        DoSort(begin(), end(), size(), compare);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    void list<T, Alignment, Allocator>::sort(Compare compare) {
        DoSort(begin(), end(), size(), compare);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Compare>
    typename list<T, Alignment, Allocator>::iterator
    list<T, Alignment, Allocator>::DoSort(iterator i1, iterator end2, size_type n, Compare &compare) {
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
                // We do a list insertion sort. Measurements showed this improved performance 3-12%.
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
        }

        // Divide the range into two parts are recursively sort each part. Upon return we will have
        // two halves that are each sorted but we'll need to merge the two together before returning.
        iterator result;
        size_type nMid = (n / 2);
        iterator end1 = std::next(i1, (difference_type) nMid);
        i1 = DoSort(i1, end1, nMid, compare); // Return the new beginning of the first sorted sub-range.
        iterator i2 = DoSort(end1, end2, n - nMid, compare); // Return the new beginning of the second sorted sub-range.

        // If the start of the second list is before the start of the first list, insert the first list
        // into the second at an appropriate starting place.
        if (compare(*i2, *i1)) {
            // Find the position to insert the first list into the second list.
            iterator ix = i2.next();
            while ((ix != end2) && compare(*ix, *i1))
                ++ix;

            // Cut out the initial segment of the second list and move it to be in front of the first list.
            ListNodeBase *i2Cut = i2.mpNode;
            ListNodeBase *i2CutLast = ix.mpNode->mpPrev;
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
                // Find the position to insert the i2 list into the i1 list.
                iterator ix = i2.next();
                while ((ix != end2) && compare(*ix, *i1))
                    ++ix;

                // Cut this section of the i2 sub-segment out and merge into the appropriate place in the i1 list.
                ListNodeBase *i2Cut = i2.mpNode;
                ListNodeBase *i2CutLast = ix.mpNode->mpPrev;
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
    inline typename list<T, Alignment, Allocator>::node_type *
    list<T, Alignment, Allocator>::DoCreateNode(Args &&... args) {
        node_type *const pNode = DoAllocateNode(); // pNode is of type node_type, but it's uninitialized memory.

#if FERMAT_EXCEPTIONS_ENABLED
        try {
            detail::allocator_construct(internalAllocator(), &pNode->mValue, std::forward<Args>(args)...);
        } catch (...) {
            DoFreeNode(pNode);
            throw;
        }
#else
        detail::allocator_construct(internalAllocator(), &pNode->mValue, std::forward<Args>(args)...);
#endif

        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename list<T, Alignment, Allocator>::node_type *
    list<T, Alignment, Allocator>::DoCreateNode() {
        node_type *const pNode = DoAllocateNode();

#if FERMAT_EXCEPTIONS_ENABLED
        try {
            detail::allocator_construct(internalAllocator(), &pNode->mValue);
        } catch (...) {
            DoFreeNode(pNode);
            throw;
        }
#else
        detail::allocator_construct(internalAllocator(), &pNode->mValue);
#endif

        return pNode;
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void list<T, Alignment, Allocator>::DoAssign(Integer n, Integer value, std::true_type) {
        DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    void list<T, Alignment, Allocator>::DoAssign(InputIterator first, InputIterator last, std::false_type) {
        node_type *pNode = static_cast<node_type *>(internalNode().mpNext);

        for (; (pNode != &internalNode()) && (first != last); ++first) {
            pNode->mValue = *first;
            pNode = static_cast<node_type *>(pNode->mpNext);
        }

        if (first == last)
            erase(const_iterator(pNode), &internalNode());
        else
            DoInsert(&internalNode(), first, last, std::false_type());
    }


    template<typename T, size_t Alignment, typename Allocator>
    void list<T, Alignment, Allocator>::DoAssignValues(size_type n, const value_type &value) {
        node_type *pNode = static_cast<node_type *>(internalNode().mpNext);

        for (; (pNode != &internalNode()) && (n > 0); --n) {
            pNode->mValue = value;
            pNode = static_cast<node_type *>(pNode->mpNext);
        }

        if (n)
            DoInsertValues(&internalNode(), n, value);
        else
            erase(const_iterator(pNode), &internalNode());
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename Integer>
    inline void list<T, Alignment, Allocator>::DoInsert(ListNodeBase *pNode, Integer n, Integer value, std::true_type) {
        DoInsertValues(pNode, static_cast<size_type>(n), static_cast<value_type>(value));
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename InputIterator>
    inline void list<T, Alignment, Allocator>::DoInsert(ListNodeBase *pNode, InputIterator first, InputIterator last, std::false_type) {
        for (; first != last; ++first)
            DoInsertValue(pNode, *first);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::DoInsertValues(ListNodeBase *pNode, size_type n, const value_type &value) {
        for (; n > 0; --n)
            DoInsertValue(pNode, value);
    }


    template<typename T, size_t Alignment, typename Allocator>
    template<typename... Args>
    inline void list<T, Alignment, Allocator>::DoInsertValue(ListNodeBase *pNode, Args &&... args) {
        node_type *const pNodeNew = DoCreateNode(std::forward<Args>(args)...);
        pNodeNew->insert(pNode);
        ++mSize;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::DoErase(ListNodeBase *pNode) {
        pNode->remove();
        ((node_type *) pNode)->~node_type();
        DoFreeNode(((node_type *) pNode));
        --mSize;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void list<T, Alignment, Allocator>::DoSwap(this_type &x) {
        ListNodeBase::swap((ListNodeBase &) internalNode(), (ListNodeBase &) x.internalNode());
        // We need to implement a special swap because we can't do a shallow swap.
        std::swap(internalAllocator(), x.internalAllocator()); // We do this even if FERMAT_ALLOCATOR_COPY_ENABLED is 0.
        std::swap(mSize, x.mSize);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool list<T, Alignment, Allocator>::validate() const {
        size_type n = 0;

        for (const_iterator i(begin()), iEnd(end()); i != iEnd; ++i)
            ++n;

        if (n != mSize)
            return false;

        // To do: More validation.
        return true;
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    bool operator==(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        typename list<T, Alignment, Allocator>::const_iterator ia = a.begin();
        typename list<T, Alignment, Allocator>::const_iterator ib = b.begin();
        typename list<T, Alignment, Allocator>::const_iterator enda = a.end();

        if (a.size() == b.size()) {
            while ((ia != enda) && (*ia == *ib)) {
                ++ia;
                ++ib;
            }
            return (ia == enda);
        }
        return false;

    }

#if defined(KM_COMPILER_HAS_THREE_WAY_COMPARISON)
    template<typename T, size_t Alignment, typename Allocator>
    inline synth_three_way_result<T> operator<=>(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(), synth_three_way{});
    }
#else
    template<typename T, size_t Alignment, typename Allocator>
    bool operator<(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator!=(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        return !(a == b);
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator>(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        return b < a;
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator<=(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        return !(b < a);
    }

    template<typename T, size_t Alignment, typename Allocator>
    bool operator>=(const list<T, Alignment, Allocator> &a, const list<T, Alignment, Allocator> &b) {
        return !(a < b);
    }
#endif
    template<typename T, size_t Alignment, typename Allocator>
    void swap(list<T, Alignment, Allocator> &a, list<T, Alignment, Allocator> &b) {
        a.swap(b);
    }


    ///////////////////////////////////////////////////////////////////////
    // erase / erase_if
    //
    // https://en.cppreference.com/w/cpp/container/list/erase2
    ///////////////////////////////////////////////////////////////////////
    template<class T,size_t Alignment,  class Allocator, class U>
    typename list<T, Alignment, Allocator>::size_type erase(list<T, Alignment, Allocator> &c, const U &value) {
        // Erases all elements that compare equal to value from the container.
        return c.remove(value);
    }

    template<class T, size_t Alignment, class Allocator, class Predicate>
    typename list<T, Alignment, Allocator>::size_type erase_if(list<T, Alignment, Allocator> &c, Predicate predicate) {
        // Erases all elements that satisfy the predicate pred from the container.
        return c.remove_if(predicate);
    }
} // namespace fermat


KM_RESTORE_SN_WARNING()

KM_RESTORE_VC_WARNING();


#endif // Header include guard
