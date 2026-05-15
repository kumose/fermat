///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// An slist is a singly-linked list. The C++ standard library doesn't define
// such a thing as an slist, nor does the C++ TR1. Our implementation of slist
// largely follows the design of the SGI STL slist container, which is also 
// found in STLPort. Singly-linked lists use less memory than doubly-linked 
// lists, but are less flexible. 
//
// In looking at slist, you will notice a lot of references to things like
// 'before first', 'before last', 'insert after', and 'erase after'. This is 
// due to the fact that std::list insert and erase works on the node before
// the referenced node, whereas slist is singly linked and operations are only
// efficient if they work on the node after the referenced node. This is because
// with an slist node you know the node after it but not the node before it.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef FERMAT_SLIST_H
#define FERMAT_SLIST_H


#include <fermat/types/internal/config.h>
#include <fermat/memory/allocator.h>
#include <fermat/types/type_traits.h>
#include <iterator>
#include <fermat/types/algorithm.h>
#include <initializer_list>
#include <fermat/types/memory.h>
#include <fermat/types/sort.h>
#include <fermat/types/compressed_pair.h>
#include <cstddef>
#if FERMAT_EXCEPTIONS_ENABLED
#include <stdexcept>
#endif

KM_DISABLE_ALL_VC_WARNINGS();

#include <new>

KM_RESTORE_ALL_VC_WARNINGS();

KM_DISABLE_SN_WARNING(828);
// The EDG SN compiler has a bug in its handling of variadic template arguments and mistakenly reports "parameter "args" was never referenced"


// 4530 - C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
// 4345 - Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
// 4571 - catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
KM_DISABLE_VC_WARNING(4530 4345 4571);


#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.

#endif


namespace fermat {
	/// FERMAT_SLIST_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
#ifndef FERMAT_SLIST_DEFAULT_NAME
#define FERMAT_SLIST_DEFAULT_NAME FERMAT_DEFAULT_NAME_PREFIX " slist" // Unless the user overrides something, this is "EASTL slist".

#endif


	/// FERMAT_SLIST_DEFAULT_ALLOCATOR
	///
#ifndef FERMAT_SLIST_DEFAULT_ALLOCATOR
#define FERMAT_SLIST_DEFAULT_ALLOCATOR allocator_type(FERMAT_SLIST_DEFAULT_NAME)
#endif


	/// SListNodeBase
	///
	/// This is a standalone struct so that operations on it can be done without templates
	/// and so that an empty slist can have an SListNodeBase and thus not create any
	/// instances of T.
	///
	struct SListNodeBase {
		SListNodeBase *mpNext;
	};


	template<typename T>
	struct SListNode : public SListNodeBase {
		T mValue;
	};

	/// SListIterator
	///
	template<typename T, typename Pointer, typename Reference>
	struct SListIterator {
		typedef SListIterator<T, Pointer, Reference> this_type;
		typedef SListIterator<T, T *, T &> iterator;
		typedef SListIterator<T, const T *, const T &> const_iterator;
		typedef size_t size_type; // See config.h for the definition of size_t, which defaults to size_t.
		typedef ptrdiff_t difference_type;
		typedef T value_type;
		typedef SListNodeBase base_node_type;
		typedef SListNode<T> node_type;
		typedef Pointer pointer;
		typedef Reference reference;
		typedef std::forward_iterator_tag iterator_category;

	public:
		base_node_type *mpNode;

	public:
		SListIterator();

		SListIterator(const SListNodeBase *pNode);

		template<typename This = this_type, std::enable_if_t<!std::is_same_v<This, iterator>, bool> = true>
		inline SListIterator(const iterator &x)
			: mpNode(x.mpNode) {
			// Empty
		}

		reference operator*() const;

		pointer operator->() const;

		this_type &operator++();

		this_type operator++(int);
	};


	/// SListBase
	///
	/// See VectorBase (class vector) for an explanation of why we
	/// create this separate base class.
	///
	template<typename T, size_t Alignment, typename Allocator>
	struct SListBase {
	public:
		typedef Allocator allocator_type;
		typedef SListNode<T> node_type;
		typedef size_t size_type; // See config.h for the definition of size_t, which defaults to size_t.
		typedef ptrdiff_t difference_type;
		typedef SListNodeBase base_node_type;
		// We use SListNodeBase instead of SListNode<T> because we don't want to create a T.

	protected:
		fermat::compressed_pair<base_node_type, allocator_type> mNodeAllocator;
#if FERMAT_SLIST_SIZE_CACHE
		size_type mSize;
#endif

		base_node_type &internalNode() noexcept { return mNodeAllocator.first(); }
		base_node_type const &internalNode() const noexcept { return mNodeAllocator.first(); }
		allocator_type &internalAllocator() noexcept { return mNodeAllocator.second(); }
		const allocator_type &internalAllocator() const noexcept { return mNodeAllocator.second(); }

	public:
		const allocator_type &get_allocator() const noexcept;

		allocator_type &get_allocator() noexcept;

		void set_allocator(const allocator_type &allocator);

	protected:
		SListBase();

		SListBase(const allocator_type &a);

		~SListBase();

		node_type *DoAllocateNode();

		void DoFreeNode(node_type *pNode);

		SListNodeBase *DoEraseAfter(SListNodeBase *pNode);

		SListNodeBase *DoEraseAfter(SListNodeBase *pNode, SListNodeBase *pNodeLast);
	}; // class SListBase


	/// slist
	///
	/// This is the equivalent of C++11's forward_list.
	///
	/// -- size() is O(n) --
	/// Note that as of this writing, list::size() is an O(n) operation when FERMAT_SLIST_SIZE_CACHE is disabled.
	/// That is, getting the size of the list is not a fast operation, as it requires traversing the list and
	/// counting the nodes. We could make list::size() be fast by having a member mSize variable. There are reasons
	/// for having such functionality and reasons for not having such functionality. We currently choose
	/// to not have a member mSize variable as it would add four bytes to the class, add a tiny amount
	/// of processing to functions such as insert and erase, and would only serve to improve the size
	/// function, but no others. The alternative argument is that the C++ standard states that std::list
	/// should be an O(1) operation (i.e. have a member size variable), most C++ standard library list
	/// implementations do so, the size is but an integer which is quick to update, and many users
	/// expect to have a fast size function. The FERMAT_SLIST_SIZE_CACHE option changes this.
	/// To consider: Make size caching an optional template parameter.
	///
	/// Pool allocation
	/// If you want to make a custom memory pool for a list container, your pool
	/// needs to contain items of type slist::node_type. So if you have a memory
	/// pool that has a constructor that takes the size of pool items and the
	/// count of pool items, you would do this (assuming that MemoryPool implements
	/// the Allocator interface):
	///     typedef slist<Widget, MemoryPool> WidgetList;          // Delare your WidgetList type.
	///     MemoryPool myPool(sizeof(WidgetList::node_type), 100); // Make a pool of 100 Widget nodes.
	///     WidgetList myList(&myPool);                            // Create a list that uses the pool.
	///
	template<typename T, size_t Alignment,  typename Allocator = BasicAllocator<T, Alignment>>
	class slist : public SListBase<T, Alignment, Allocator> {
		typedef SListBase<T, Alignment, Allocator> base_type;
		typedef slist<T, Alignment, Allocator> this_type;

	protected:
		using base_type::mNodeAllocator;
		using base_type::DoEraseAfter;
		using base_type::DoAllocateNode;
		using base_type::DoFreeNode;
#if FERMAT_SLIST_SIZE_CACHE
		using base_type::mSize;
#endif
		using base_type::internalNode;
		using base_type::internalAllocator;

	public:
		typedef T value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef SListIterator<T, T *, T &> iterator;
		typedef SListIterator<T, const T *, const T &> const_iterator;
		typedef typename base_type::size_type size_type;
		typedef typename base_type::difference_type difference_type;
		typedef typename base_type::allocator_type allocator_type;
		typedef typename base_type::node_type node_type;
		typedef typename base_type::base_node_type base_node_type;

		static_assert(!std::is_const<value_type>::value, "slist<T> value_type must be non-const.");
		static_assert(!std::is_volatile<value_type>::value, "slist<T> value_type must be non-volatile.");

	public:
		slist();

		slist(const allocator_type &allocator);

		explicit slist(size_type n, const allocator_type &allocator = FERMAT_SLIST_DEFAULT_ALLOCATOR);

		slist(size_type n, const value_type &value, const allocator_type &allocator = FERMAT_SLIST_DEFAULT_ALLOCATOR);

		slist(const this_type &x);

		slist(std::initializer_list<value_type> ilist,
		      const allocator_type &allocator = FERMAT_SLIST_DEFAULT_ALLOCATOR);

		slist(this_type &&x);

		slist(this_type &&x, const allocator_type &allocator);

		template<typename InputIterator>
		slist(InputIterator first, InputIterator last);

		// allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

		this_type &operator=(const this_type &x);

		this_type &operator=(std::initializer_list<value_type>);

		this_type &operator=(this_type &&x);

		void swap(this_type &x);

		void assign(size_type n, const value_type &value);

		void assign(std::initializer_list<value_type> ilist);

		template<typename InputIterator>
		void assign(InputIterator first, InputIterator last);

		iterator begin() noexcept;

		const_iterator begin() const noexcept;

		const_iterator cbegin() const noexcept;

		iterator end() noexcept;

		const_iterator end() const noexcept;

		const_iterator cend() const noexcept;

		iterator before_begin() noexcept;

		const_iterator before_begin() const noexcept;

		const_iterator cbefore_begin() const noexcept;

		iterator previous(const_iterator position);

		const_iterator previous(const_iterator position) const;

		reference front();

		const_reference front() const;

		template<class... Args>
		reference emplace_front(Args &&... args);

		void push_front(const value_type &value);

		reference push_front();

		void push_front(value_type &&value);

		void pop_front();

		bool empty() const noexcept;

		size_type size() const noexcept;

		void resize(size_type n, const value_type &value);

		void resize(size_type n);

		iterator insert(const_iterator position);

		iterator insert(const_iterator position, const value_type &value);

		void insert(const_iterator position, size_type n, const value_type &value);

		template<typename InputIterator>
		void insert(const_iterator position, InputIterator first, InputIterator last);

		// Returns an iterator pointing to the last inserted element, or position if insertion count is zero.
		iterator insert_after(const_iterator position);

		iterator insert_after(const_iterator position, const value_type &value);

		iterator insert_after(const_iterator position, size_type n, const value_type &value);

		iterator insert_after(const_iterator position, std::initializer_list<value_type> ilist);

		iterator insert_after(const_iterator position, value_type &&value);

		template<class... Args>
		iterator emplace_after(const_iterator position, Args &&... args);

		template<typename InputIterator>
		iterator insert_after(const_iterator position, InputIterator first, InputIterator last);

		iterator erase(const_iterator position);

		iterator erase(const_iterator first, const_iterator last);

		iterator erase_after(const_iterator position);

		iterator erase_after(const_iterator before_first, const_iterator last);

		void clear() noexcept;

		void reset_lose_memory() noexcept;

		// This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

		size_type remove(const value_type &value);

		template<typename Predicate>
		size_type remove_if(Predicate predicate);

		void reverse() noexcept;

		// splice splices to before position, like with the list container. However, in order to do so
		// it must walk the list from beginning to position, which is an O(n) operation that can thus
		// be slow. It's recommended that the splice_after functions be used whenever possible as they are O(1).
		void splice(const_iterator position, this_type &x);

		void splice(const_iterator position, this_type &x, const_iterator i);

		void splice(const_iterator position, this_type &x, const_iterator first, const_iterator last);

		void splice(const_iterator position, this_type &&x);

		void splice(const_iterator position, this_type &&x, const_iterator i);

		void splice(const_iterator position, this_type &&x, const_iterator first, const_iterator last);

		void splice_after(const_iterator position, this_type &x);

		void splice_after(const_iterator position, this_type &x, const_iterator i);

		void splice_after(const_iterator position, this_type &x, const_iterator first, const_iterator last);

		void splice_after(const_iterator position, this_type &&x);

		void splice_after(const_iterator position, this_type &&x, const_iterator i);

		void splice_after(const_iterator position, this_type &&x, const_iterator first, const_iterator last);

		size_type unique();

		template<typename BinaryPredicate>
		size_type unique(BinaryPredicate);

		// Sorting functionality
		// This is independent of the global sort algorithms, as lists are
		// linked nodes and can be sorted more efficiently by moving nodes
		// around in ways that global sort algorithms aren't privy to.
		void sort();

		template<class Compare>
		void sort(Compare compare);

		// Not yet implemented:
		// void merge(this_type& x);
		// void merge(this_type&& x);
		// template <class Compare>
		// void merge(this_type& x, Compare compare);
		// template <class Compare>
		// void merge(this_type&& x, Compare compare);
		// If these get implemented then make sure to override them in fixed_slist.

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

		template<typename InputIterator>
		node_type *DoInsertAfter(SListNodeBase *pNode, InputIterator first, InputIterator last);

		template<typename Integer>
		node_type *DoInsertAfter(SListNodeBase *pNode, Integer n, Integer value, std::true_type);

		template<typename InputIterator>
		node_type *DoInsertAfter(SListNodeBase *pNode, InputIterator first, InputIterator last, std::false_type);

		node_type *DoInsertValueAfter(SListNodeBase *pNode);

		node_type *DoInsertValuesAfter(SListNodeBase *pNode, size_type n, const value_type &value);

		template<typename... Args>
		node_type *DoInsertValueAfter(SListNodeBase *pNode, Args &&... args);

		void DoSwap(this_type &x);
	}; // class slist


	///////////////////////////////////////////////////////////////////////
	// SListNodeBase functions
	///////////////////////////////////////////////////////////////////////

	inline SListNodeBase *SListNodeInsertAfter(SListNodeBase *pPrevNode, SListNodeBase *pNode) {
		pNode->mpNext = pPrevNode->mpNext;
		pPrevNode->mpNext = pNode;
		return pNode;
	}

	inline SListNodeBase *SListNodeGetPrevious(SListNodeBase *pNodeBase, const SListNodeBase *pNode) {
		while (pNodeBase && (pNodeBase->mpNext != pNode))
			pNodeBase = pNodeBase->mpNext;
		return pNodeBase;
	}

	inline const SListNodeBase *SListNodeGetPrevious(const SListNodeBase *pNodeBase, const SListNodeBase *pNode) {
		while (pNodeBase && (pNodeBase->mpNext != pNode))
			pNodeBase = pNodeBase->mpNext;
		return pNodeBase;
	}

	inline void SListNodeSpliceAfter(SListNodeBase *pNode, SListNodeBase *pNodeBeforeFirst,
	                                 SListNodeBase *pNodeBeforeLast) {
		if ((pNode != pNodeBeforeFirst) && (pNode != pNodeBeforeLast)) {
			SListNodeBase *const pFirst = pNodeBeforeFirst->mpNext;
			SListNodeBase *const pPosition = pNode->mpNext;

			pNodeBeforeFirst->mpNext = pNodeBeforeLast->mpNext;
			pNode->mpNext = pFirst;
			pNodeBeforeLast->mpNext = pPosition;
		}
	}

	inline void SListNodeSpliceAfter(SListNodeBase *pNode, SListNodeBase *pNodeBase) {
		SListNodeBase *const pNodeBeforeLast = SListNodeGetPrevious(pNodeBase, NULL);

		if (pNodeBeforeLast != pNodeBase) {
			SListNodeBase *const pPosition = pNode->mpNext;
			pNode->mpNext = pNodeBase->mpNext;
			pNodeBase->mpNext = NULL;
			pNodeBeforeLast->mpNext = pPosition;
		}
	}

	inline SListNodeBase *SListNodeReverse(SListNodeBase *pNode) {
		SListNodeBase *pNodeFirst = pNode;
		pNode = pNode->mpNext;
		pNodeFirst->mpNext = NULL;

		while (pNode) {
			SListNodeBase *const pTemp = pNode->mpNext;
			pNode->mpNext = pNodeFirst;
			pNodeFirst = pNode;
			pNode = pTemp;
		}
		return pNodeFirst;
	}

	inline uint32_t SListNodeGetSize(SListNodeBase *pNode) {
		uint32_t n = 0;
		while (pNode) {
			++n;
			pNode = pNode->mpNext;
		}
		return n;
	}


	///////////////////////////////////////////////////////////////////////
	// SListIterator functions
	///////////////////////////////////////////////////////////////////////

	template<typename T, typename Pointer, typename Reference>
	inline SListIterator<T, Pointer, Reference>::SListIterator()
		: mpNode(NULL) {
		// Empty
	}


	template<typename T, typename Pointer, typename Reference>
	inline SListIterator<T, Pointer, Reference>::SListIterator(const SListNodeBase *pNode)
		: mpNode(const_cast<base_node_type *>(pNode)) {
		// Empty
	}


	template<typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::reference
	SListIterator<T, Pointer, Reference>::operator*() const {
		return static_cast<node_type *>(mpNode)->mValue;
	}


	template<typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::pointer
	SListIterator<T, Pointer, Reference>::operator->() const {
		return &static_cast<node_type *>(mpNode)->mValue;
	}


	template<typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::this_type &
	SListIterator<T, Pointer, Reference>::operator++() {
		mpNode = mpNode->mpNext;
		return *this;
	}


	template<typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::this_type
	SListIterator<T, Pointer, Reference>::operator++(int) {
		this_type temp(*this);
		mpNode = mpNode->mpNext;
		return temp;
	}

	// The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
	// Thus we provide additional template paremeters here to support this. The defect report does not
	// require us to support comparisons between reverse_iterators and const_reverse_iterators.
	template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator==(const SListIterator<T, PointerA, ReferenceA> &a,
	                       const SListIterator<T, PointerB, ReferenceB> &b) {
		return a.mpNode == b.mpNode;
	}


	template<typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator!=(const SListIterator<T, PointerA, ReferenceA> &a,
	                       const SListIterator<T, PointerB, ReferenceB> &b) {
		return a.mpNode != b.mpNode;
	}


	// We provide a version of operator!= for the case where the iterators are of the
	// same type. This helps prevent ambiguity errors in the presence of rel_ops.
	template<typename T, typename Pointer, typename Reference>
	inline bool operator!=(const SListIterator<T, Pointer, Reference> &a,
	                       const SListIterator<T, Pointer, Reference> &b) {
		return a.mpNode != b.mpNode;
	}


	///////////////////////////////////////////////////////////////////////
	// SListBase functions
	///////////////////////////////////////////////////////////////////////

	template<typename T, size_t Alignment, typename Allocator>
	inline SListBase<T, Alignment, Allocator>::SListBase()
		: mNodeAllocator(base_node_type(), allocator_type(FERMAT_SLIST_DEFAULT_NAME))
#if FERMAT_SLIST_SIZE_CACHE
		  , mSize(0)
#endif
	{
		internalNode().mpNext = NULL;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline SListBase<T, Alignment, Allocator>::SListBase(const allocator_type &allocator)
		: mNodeAllocator(base_node_type(), allocator)
#if FERMAT_SLIST_SIZE_CACHE
		  , mSize(0)
#endif
	{
		internalNode().mpNext = NULL;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline SListBase<T, Alignment, Allocator>::~SListBase() {
		DoEraseAfter(&internalNode(), NULL);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline const typename SListBase<T, Alignment, Allocator>::allocator_type &
	SListBase<T, Alignment, Allocator>::get_allocator() const noexcept {
		return internalAllocator();
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename SListBase<T, Alignment, Allocator>::allocator_type &
	SListBase<T, Alignment, Allocator>::get_allocator() noexcept {
		return internalAllocator();
	}


	template<typename T, size_t Alignment, typename Allocator>
	void
	SListBase<T, Alignment, Allocator>::set_allocator(const allocator_type &allocator) {
		if ((internalAllocator() != allocator) && (static_cast<node_type *>(internalNode().mpNext) != NULL))
			FERMAT_THROW_MSG_OR_ASSERT(std::logic_error,
		                           "slist::set_allocator -- cannot change allocator after allocations have been made.");
		internalAllocator() = allocator;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline SListNode<T> *SListBase<T, Alignment, Allocator>::DoAllocateNode() {
		return (node_type *) allocate_memory(internalAllocator(), sizeof(node_type), FERMAT_ALIGN_OF(node_type), 0);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void SListBase<T, Alignment, Allocator>::DoFreeNode(node_type *pNode) {
		EASTLFree(internalAllocator(), pNode, sizeof(node_type));
	}


	template<typename T, size_t Alignment, typename Allocator>
	SListNodeBase *SListBase<T, Alignment, Allocator>::DoEraseAfter(SListNodeBase *pNode) {
		node_type *const pNodeNext = static_cast<node_type *>((base_node_type *) pNode->mpNext);
		SListNodeBase *const pNodeNextNext = pNodeNext->mpNext;

		pNode->mpNext = pNodeNextNext;
		pNodeNext->~node_type();
		DoFreeNode(pNodeNext);
#if FERMAT_SLIST_SIZE_CACHE
		--mSize;
#endif
		return pNodeNextNext;
	}


	template<typename T, size_t Alignment, typename Allocator>
	SListNodeBase *SListBase<T, Alignment, Allocator>::DoEraseAfter(SListNodeBase *pNode, SListNodeBase *pNodeLast) {
		node_type *pNodeCurrent = static_cast<node_type *>((base_node_type *) pNode->mpNext);

		while (pNodeCurrent != (base_node_type *) pNodeLast) {
			node_type *const pNodeTemp = pNodeCurrent;
			pNodeCurrent = static_cast<node_type *>((base_node_type *) pNodeCurrent->mpNext);
			pNodeTemp->~node_type();
			DoFreeNode(pNodeTemp);
#if FERMAT_SLIST_SIZE_CACHE
			--mSize;
#endif
		}
		pNode->mpNext = pNodeLast;
		return pNodeLast;
	}


	///////////////////////////////////////////////////////////////////////
	// slist functions
	///////////////////////////////////////////////////////////////////////

	template<typename T, size_t Alignment, typename Allocator>
	inline slist<T, Alignment, Allocator>::slist()
		: base_type() {
		// Empty
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline slist<T, Alignment, Allocator>::slist(const allocator_type &allocator)
		: base_type(allocator) {
		// Empty
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline slist<T, Alignment, Allocator>::slist(size_type n, const allocator_type &allocator)
		: base_type(allocator) {
		DoInsertValuesAfter(&internalNode(), n, value_type());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline slist<T, Alignment, Allocator>::slist(size_type n, const value_type &value, const allocator_type &allocator)
		: base_type(allocator) {
		DoInsertValuesAfter(&internalNode(), n, value);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline slist<T, Alignment, Allocator>::slist(const slist &x)
		: base_type(x.internalAllocator()) {
		DoInsertAfter(&internalNode(), const_iterator(x.internalNode().mpNext), const_iterator(NULL), std::false_type());
	}


	template<typename T, size_t Alignment, typename Allocator>
	slist<T, Alignment, Allocator>::slist(this_type &&x)
		: base_type(x.internalAllocator()) {
		swap(x);
	}

	template<typename T, size_t Alignment, typename Allocator>
	slist<T, Alignment, Allocator>::slist(this_type &&x, const allocator_type &allocator)
		: base_type(allocator) {
		swap(x); // member swap handles the case that x has a different allocator than our allocator by doing a copy.
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline slist<T, Alignment, Allocator>::slist(std::initializer_list<value_type> ilist, const allocator_type &allocator)
		: base_type(allocator) {
		DoInsertAfter(&internalNode(), ilist.begin(), ilist.end());
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator>
	inline slist<T, Alignment, Allocator>::slist(InputIterator first, InputIterator last)
		: base_type(FERMAT_SLIST_DEFAULT_ALLOCATOR) {
		DoInsertAfter(&internalNode(), first, last);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::begin() noexcept {
		return iterator(internalNode().mpNext);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::begin() const noexcept {
		return const_iterator(internalNode().mpNext);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::cbegin() const noexcept {
		return const_iterator(internalNode().mpNext);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::end() noexcept {
		return iterator(NULL);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::end() const noexcept {
		return const_iterator(NULL);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::cend() const noexcept {
		return const_iterator(NULL);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::before_begin() noexcept {
		return iterator(&internalNode());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::before_begin() const noexcept {
		return const_iterator(&internalNode());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::cbefore_begin() const noexcept {
		return const_iterator(&internalNode());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::previous(const_iterator position) {
		return iterator(SListNodeGetPrevious(&internalNode(), position.mpNode));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_iterator
	slist<T, Alignment, Allocator>::previous(const_iterator position) const {
		return const_iterator(SListNodeGetPrevious(&internalNode(), position.mpNode));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::reference
	slist<T, Alignment, Allocator>::front() {
#if FERMAT_ASSERT_ENABLED
		if (FERMAT_UNLIKELY(internalNode().mpNext == NULL))
			FERMAT_FAIL_MSG("slist::front -- empty container");
#endif

		KM_ANALYSIS_ASSUME(internalNode().mpNext != NULL);

		return ((node_type *) internalNode().mpNext)->mValue;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::const_reference
	slist<T, Alignment, Allocator>::front() const {
#if FERMAT_ASSERT_ENABLED
		if (FERMAT_UNLIKELY(internalNode().mpNext == NULL))
			FERMAT_FAIL_MSG("slist::front -- empty container");
#endif

		KM_ANALYSIS_ASSUME(internalNode().mpNext != NULL);

		return static_cast<node_type *>(internalNode().mpNext)->mValue;
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<class... Args>
	typename slist<T, Alignment, Allocator>::reference slist<T, Alignment, Allocator>::emplace_front(Args &&... args) {
		DoInsertValueAfter(&internalNode(), std::forward<Args>(args)...);
		return static_cast<node_type *>(internalNode().mpNext)->mValue; // Same as return front();
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::push_front(const value_type &value) {
		SListNodeInsertAfter(&internalNode(), DoCreateNode(value));
#if FERMAT_SLIST_SIZE_CACHE
		++mSize;
#endif
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::reference
	slist<T, Alignment, Allocator>::push_front() {
		SListNodeInsertAfter(&internalNode(), DoCreateNode());
#if FERMAT_SLIST_SIZE_CACHE
		++mSize;
#endif
		return ((node_type *) internalNode().mpNext)->mValue; // Same as return front();
	}

	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::push_front(value_type &&value) {
		emplace_after(before_begin(), std::move(value));
	}


	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::pop_front() {
#if FERMAT_ASSERT_ENABLED
		if (FERMAT_UNLIKELY(internalNode().mpNext == NULL))
			FERMAT_FAIL_MSG("slist::front -- empty container");
#endif

		KM_ANALYSIS_ASSUME(internalNode().mpNext != NULL);

		node_type *const pNode = static_cast<node_type *>(internalNode().mpNext);
		internalNode().mpNext = pNode->mpNext;
		pNode->~node_type();
		DoFreeNode(pNode);
#if FERMAT_SLIST_SIZE_CACHE
		--mSize;
#endif
	}


	template<typename T, size_t Alignment, typename Allocator>
	typename slist<T, Alignment, Allocator>::this_type &slist<T, Alignment, Allocator>::operator=(const this_type &x) {
		if (&x != this) {
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
	typename slist<T, Alignment, Allocator>::this_type &slist<T, Alignment, Allocator>::operator=(this_type &&x) {
		if (this != &x) {
			clear();
			// To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
			swap(x);
			// member swap handles the case that x has a different allocator than our allocator by doing a copy.
		}
		return *this;
	}


	template<typename T, size_t Alignment, typename Allocator>
	typename slist<T, Alignment, Allocator>::this_type &slist<T, Alignment, Allocator>::operator=(std::initializer_list<value_type> ilist) {
		DoAssign(ilist.begin(), ilist.end(), std::false_type());
		return *this;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::assign(std::initializer_list<value_type> ilist) {
		DoAssign(ilist.begin(), ilist.end(), std::false_type());
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator> // It turns out that the C++ std::list specifies a two argument
	inline void slist<T, Alignment, Allocator>::assign(InputIterator first, InputIterator last)
	// version of assign that takes (int size, int value). These are not
	{
		// iterators, so we need to do a template compiler trick to do the right thing.
		DoAssign(first, last, std::is_integral<InputIterator>());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::assign(size_type n, const value_type &value) {
		// To do: get rid of DoAssignValues and put its implementation directly here.
		DoAssignValues(n, value);
	}


	// does not propagate allocators on swap.
	// in addition, requires T be copy constructible and copy assignable, which isn't required by the standard.
	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::swap(this_type &x) {
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
	inline bool slist<T, Alignment, Allocator>::empty() const noexcept {
		return internalNode().mpNext == NULL;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::size_type
	slist<T, Alignment, Allocator>::size() const noexcept {
		return SListNodeGetSize(internalNode().mpNext);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::clear() noexcept {
		DoEraseAfter(&internalNode(), NULL);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::reset_lose_memory() noexcept {
		// The reset function is a special extension function which unilaterally
		// resets the container to an empty state without freeing the memory of
		// the contained objects. This is useful for very quickly tearing down a
		// container built into scratch memory.
		internalNode().mpNext = NULL;
#if FERMAT_SLIST_SIZE_CACHE
		mSize = 0;
#endif
	}


	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::resize(size_type n, const value_type &value) {
		SListNodeBase *pNode = &internalNode();

		for (; pNode->mpNext && (n > 0); --n)
			pNode = pNode->mpNext;

		if (pNode->mpNext)
			DoEraseAfter(pNode, NULL);
		else
			DoInsertValuesAfter(pNode, n, value);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::resize(size_type n) {
		resize(n, value_type());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert(const_iterator position) {
		return iterator(DoInsertValueAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), value_type()));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert(const_iterator position, const value_type &value) {
		return iterator(DoInsertValueAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), value));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::insert(const_iterator position, size_type n, const value_type &value) {
		// To do: get rid of DoAssignValues and put its implementation directly here.
		DoInsertValuesAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), n, value);
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator>
	inline void slist<T, Alignment, Allocator>::insert(const_iterator position, InputIterator first, InputIterator last) {
		DoInsertAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), first, last);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert_after(const_iterator position) {
		return insert_after(position, value_type());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert_after(const_iterator position, const value_type &value) {
		return iterator(DoInsertValueAfter(position.mpNode, value));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert_after(const_iterator position, size_type n, const value_type &value) {
		return iterator(DoInsertValuesAfter(position.mpNode, n, value));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert_after(const_iterator position, std::initializer_list<value_type> ilist) {
		return iterator(DoInsertAfter(position.mpNode, ilist.begin(), ilist.end(), std::false_type()));
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert_after(const_iterator position, InputIterator first, InputIterator last) {
		return iterator(DoInsertAfter(position.mpNode, first, last));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::insert_after(const_iterator position, value_type &&value) {
		return emplace_after(position, std::move(value));
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<class... Args>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::emplace_after(const_iterator position, Args &&... args) {
		return iterator(DoInsertValueAfter(position.mpNode, std::forward<Args>(args)...));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::erase(const_iterator position) {
		return DoEraseAfter(SListNodeGetPrevious(&internalNode(), position.mpNode));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::erase(const_iterator first, const_iterator last) {
		return DoEraseAfter(SListNodeGetPrevious(&internalNode(), first.mpNode), last.mpNode);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::erase_after(const_iterator position) {
		return iterator(DoEraseAfter(position.mpNode));
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::iterator
	slist<T, Alignment, Allocator>::erase_after(const_iterator before_first, const_iterator last) {
		return iterator(DoEraseAfter(before_first.mpNode, last.mpNode));
	}


	template<typename T, size_t Alignment, typename Allocator>
	typename slist<T, Alignment, Allocator>::size_type slist<T, Alignment, Allocator>::remove(const value_type &value) {
		base_node_type *pNode = &internalNode();
		size_type numErased = 0;

		while (pNode && pNode->mpNext) {
			if (static_cast<node_type *>(pNode->mpNext)->mValue == value) {
				DoEraseAfter(pNode); // This will take care of modifying pNode->mpNext.
				++numErased;
			} else
				pNode = pNode->mpNext;
		}
		return numErased;
	}

	template<typename T, size_t Alignment, typename Allocator>
	template<typename Predicate>
	inline typename slist<T, Alignment, Allocator>::size_type slist<T, Alignment, Allocator>::remove_if(Predicate predicate) {
		base_node_type *pNode = &internalNode();
		size_type numErased = 0;

		while (pNode && pNode->mpNext) {
			if (predicate(static_cast<node_type *>(pNode->mpNext)->mValue)) {
				DoEraseAfter(pNode); // This will take care of modifying pNode->mpNext.
				++numErased;
			} else
				pNode = pNode->mpNext;
		}
		return numErased;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice(const_iterator position, this_type &x) {
		// Splicing operations cannot succeed if the two containers use unequal allocators.
		// This issue is not addressed in the C++ 1998 standard but is discussed in the
		// LWG defect reports, such as #431. There is no simple solution to this problem.
		// One option is to throw an exception. Another option which probably captures the
		// user intent most of the time is to copy the range from the source to the dest and
		// remove it from the source. Until then it's simply disallowed to splice with unequal allocators.
		// FERMAT_ASSERT(internalAllocator() == x.internalAllocator()); // Disabled because our member sort function uses splice but with allocators that may be unequal. There isn't a simple workaround aside from disabling this assert.

		if (x.internalNode().mpNext) // If there is anything to splice...
		{
			if (internalAllocator() == x.internalAllocator()) {
				SListNodeSpliceAfter(SListNodeGetPrevious(&internalNode(), position.mpNode),
				                     &x.internalNode(),
				                     SListNodeGetPrevious(&x.internalNode(), NULL));

#if FERMAT_SLIST_SIZE_CACHE
				mSize += x.mSize;
				x.mSize = 0;
#endif
			} else {
				insert(position, x.begin(), x.end());
				x.clear();
			}
		}
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice(const_iterator position, this_type &x, const_iterator i) {
		if (internalAllocator() == x.internalAllocator()) {
			SListNodeSpliceAfter(SListNodeGetPrevious(&internalNode(), position.mpNode),
			                     SListNodeGetPrevious(&x.internalNode(), i.mpNode),
			                     i.mpNode);

#if FERMAT_SLIST_SIZE_CACHE
			++mSize;
			--x.mSize;
#endif
		} else {
			insert(position, *i);
			x.erase(i);
		}
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice(const_iterator position, this_type &x, const_iterator first,
	                                        const_iterator last) {
		if (first != last) // If there is anything to splice...
		{
			if (internalAllocator() == x.internalAllocator()) {
#if FERMAT_SLIST_SIZE_CACHE
				const size_type n = (size_type) std::distance(first, last);
				mSize += n;
				x.mSize -= n;
#endif

				SListNodeSpliceAfter(SListNodeGetPrevious(&internalNode(), position.mpNode),
				                     SListNodeGetPrevious(&x.internalNode(), first.mpNode),
				                     SListNodeGetPrevious(first.mpNode, last.mpNode));
			} else {
				insert(position, first, last);
				x.erase(first, last);
			}
		}
	}


	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::splice(const_iterator position, this_type &&x) {
		return splice(position, x); // This will splice(const_iterator, this_type&)
	}

	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::splice(const_iterator position, this_type &&x, const_iterator i) {
		return splice(position, x, i); // This will splice_after(const_iterator, this_type&, const_iterator)
	}

	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::splice(const_iterator position, this_type &&x, const_iterator first,
	                                 const_iterator last) {
		return splice(position, x, first, last);
		// This will splice(const_iterator, this_type&, const_iterator, const_iterator)
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice_after(const_iterator position, this_type &x) {
		if (!x.empty()) // If there is anything to splice...
		{
			if (internalAllocator() == x.internalAllocator()) {
				SListNodeSpliceAfter(position.mpNode, &x.internalNode());

#if FERMAT_SLIST_SIZE_CACHE
				mSize += x.mSize;
				x.mSize = 0;
#endif
			} else {
				insert_after(position, x.begin(), x.end());
				x.clear();
			}
		}
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice_after(const_iterator position, this_type &x, const_iterator i) {
		if (internalAllocator() == x.internalAllocator()) {
			SListNodeSpliceAfter(position.mpNode, i.mpNode);

#if FERMAT_SLIST_SIZE_CACHE
			mSize++;
			x.mSize--;
#endif
		} else {
			const_iterator iNext(i);
			insert_after(position, i, ++iNext);
			x.erase(i);
		}
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice_after(const_iterator position, this_type &x, const_iterator first,
	                                              const_iterator last) {
		if (first != last) // If there is anything to splice...
		{
			if (internalAllocator() == x.internalAllocator()) {
#if FERMAT_SLIST_SIZE_CACHE
				const size_type n = (size_type) std::distance(first, last);
				mSize += n;
				x.mSize -= n;
#endif

				SListNodeSpliceAfter(position.mpNode, first.mpNode, last.mpNode);
			} else {
				insert_after(position, first, last);
				x.erase(first, last);
			}
		}
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice_after(const_iterator position, this_type &&x) {
		return splice_after(position, x); // This will call splice_after(const_iterator, this_type&)
	}

	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice_after(const_iterator position, this_type &&x, const_iterator i) {
		return splice_after(position, x, i); // This will call splice_after(const_iterator, this_type&, const_iterator)
	}

	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::splice_after(const_iterator position, this_type &&x, const_iterator first,
	                                              const_iterator last) {
		return splice_after(position, x, first, last);
		// This will call splice_after(const_iterator, this_type&, const_iterator, const_iterator)
	}


	template<typename T, size_t Alignment, typename Allocator>
	typename slist<T, Alignment, Allocator>::size_type slist<T, Alignment, Allocator>::unique() {
		size_type numRemoved = 0;
		iterator first(begin());
		const iterator last(end());

		if (first != last) {
			iterator next(first);

			while (++next != last) {
				if (*first == *next) {
					DoEraseAfter(first.mpNode);
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
	typename slist<T, Alignment, Allocator>::size_type slist<T, Alignment, Allocator>::unique(BinaryPredicate predicate) {
		size_type numRemoved = 0;
		iterator first(begin());
		const iterator last(end());

		if (first != last) {
			iterator next(first);

			while (++next != last) {
				if (predicate(*first, *next)) {
					DoEraseAfter(first.mpNode);
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
	inline void slist<T, Alignment, Allocator>::sort() {
		// To do: look at using a merge sort, which may well be faster.
		fermat::comb_sort(begin(), end());
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<class Compare>
	inline void slist<T, Alignment, Allocator>::sort(Compare compare) {
		// To do: look at using a merge sort, which may well be faster.
		fermat::comb_sort(begin(), end(), compare);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::reverse() noexcept {
		if (internalNode().mpNext)
			internalNode().mpNext = static_cast<node_type *>((base_node_type *)
				SListNodeReverse(internalNode().mpNext));
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename... Args>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoCreateNode(Args &&... args) {
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
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoCreateNode() {
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
	void slist<T, Alignment, Allocator>::DoAssign(Integer n, Integer value, std::true_type) {
		DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator>
	void slist<T, Alignment, Allocator>::DoAssign(InputIterator first, InputIterator last, std::false_type) {
		base_node_type *pNodePrev = &internalNode();
		base_node_type *pNode = internalNode().mpNext;

		for (; pNode && (first != last); ++first) {
			static_cast<node_type *>(pNode)->mValue = *first;
			pNodePrev = pNode;
			pNode = pNode->mpNext;
		}

		if (first == last)
			DoEraseAfter(pNodePrev, NULL);
		else
			DoInsertAfter(pNodePrev, first, last);
	}


	template<typename T, size_t Alignment, typename Allocator>
	void slist<T, Alignment, Allocator>::DoAssignValues(size_type n, const value_type &value) {
		base_node_type *pNodePrev = &internalNode();
		base_node_type *pNode = internalNode().mpNext;

		for (; pNode && (n > 0); --n) {
			static_cast<node_type *>(pNode)->mValue = value;
			pNodePrev = pNode;
			pNode = pNode->mpNext;
		}

		if (n)
			DoInsertValuesAfter(pNodePrev, n, value);
		else
			DoEraseAfter(pNodePrev, NULL);
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoInsertAfter(SListNodeBase *pNode, InputIterator first, InputIterator last) {
		return DoInsertAfter(pNode, first, last, std::is_integral<InputIterator>());
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename Integer>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoInsertAfter(SListNodeBase *pNode, Integer n, Integer value, std::true_type) {
		return DoInsertValuesAfter(pNode, n, value);
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename InputIterator>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoInsertAfter(SListNodeBase *pNode, InputIterator first, InputIterator last, std::false_type) {
		for (; first != last; ++first) {
			pNode = SListNodeInsertAfter(pNode, DoCreateNode(*first));
#if FERMAT_SLIST_SIZE_CACHE
			++mSize;
#endif
		}

		return static_cast<node_type *>((base_node_type *) pNode);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoInsertValueAfter(SListNodeBase *pNode) {
#if FERMAT_SLIST_SIZE_CACHE
		pNode = SListNodeInsertAfter(pNode, DoCreateNode());
		++mSize;
		return static_cast<node_type *>((base_node_type *) pNode);
#else
		return static_cast<node_type *>((base_node_type *) SListNodeInsertAfter(pNode, DoCreateNode()));
#endif
	}


	template<typename T, size_t Alignment, typename Allocator>
	template<typename... Args>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoInsertValueAfter(SListNodeBase *pNode, Args &&... args) {
		SListNodeBase *pNodeNew = DoCreateNode(std::forward<Args>(args)...);
		pNode = SListNodeInsertAfter(pNode, pNodeNew);
		++mSize;
		// Increment the size after the node creation because we need to assume an exception can occur in the creation.
		return static_cast<node_type *>((base_node_type *) pNode);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline typename slist<T, Alignment, Allocator>::node_type *
	slist<T, Alignment, Allocator>::DoInsertValuesAfter(SListNodeBase *pNode, size_type n, const value_type &value) {
		for (size_type i = 0; i < n; ++i) {
			pNode = SListNodeInsertAfter(pNode, DoCreateNode(value));
#if FERMAT_SLIST_SIZE_CACHE
			++mSize;
			// We don't do a single mSize += n at the end because an exception may result in only a partial range insertion.
#endif
		}
		return static_cast<node_type *>((base_node_type *) pNode);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline void slist<T, Alignment, Allocator>::DoSwap(this_type &x) {
		std::swap(internalNode().mpNext, x.internalNode().mpNext);
		std::swap(internalAllocator(), x.internalAllocator()); // We do this even if FERMAT_ALLOCATOR_COPY_ENABLED is 0.
		std::swap(mSize, x.mSize);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline bool slist<T, Alignment, Allocator>::validate() const {
#if FERMAT_SLIST_SIZE_CACHE
		size_type n = 0;

		for (const_iterator i(begin()), iEnd(end()); i != iEnd; ++i)
			++n;

		if (n != mSize)
			return false;
#endif

		// To do: More validation.
		return true;
	}



	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template<typename T, size_t Alignment, typename Allocator>
	bool operator==(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b) {
		typename slist<T, Alignment, Allocator>::const_iterator ia = a.begin();
		typename slist<T, Alignment, Allocator>::const_iterator ib = b.begin();
		typename slist<T, Alignment, Allocator>::const_iterator enda = a.end();

#if FERMAT_SLIST_SIZE_CACHE
		if (a.size() == b.size()) {
			while ((ia != enda) && (*ia == *ib)) {
				++ia;
				++ib;
			}
			return (ia == enda);
		}
		return false;
#else
		typename slist<T, Alignment, Allocator>::const_iterator endb = b.end();

		while ((ia != enda) && (ib != endb) && (*ia == *ib)) {
			++ia;
			++ib;
		}
		return (ia == enda) && (ib == endb);
#endif
	}

#if defined(KM_COMPILER_HAS_THREE_WAY_COMPARISON)
	template<typename T, size_t Alignment, typename Allocator>
	inline synth_three_way_result<T> operator<=>(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b)
	{
		return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(), synth_three_way{});
	}
#else
	template<typename T, size_t Alignment, typename Allocator>
	inline bool operator<(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b) {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline bool operator!=(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b) {
		return !(a == b);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline bool operator>(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b) {
		return b < a;
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline bool operator<=(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b) {
		return !(b < a);
	}


	template<typename T, size_t Alignment, typename Allocator>
	inline bool operator>=(const slist<T, Alignment, Allocator> &a, const slist<T, Alignment, Allocator> &b) {
		return !(a < b);
	}
#endif

	template<typename T, size_t Alignment, typename Allocator>
	inline void swap(slist<T, Alignment, Allocator> &a, slist<T, Alignment, Allocator> &b) {
		a.swap(b);
	}


	/// erase / erase_if
	///
	/// https://en.cppreference.com/w/cpp/container/forward_list/erase2
	template<class T, size_t Alignment, class Allocator, class U>
	typename slist<T, Alignment, Allocator>::size_type erase(slist<T, Alignment, Allocator> &c, const U &value) {
		// Erases all elements that compare equal to value from the container.
		return c.remove(value);
	}

	template<class T, size_t Alignment, class Allocator, class Predicate>
	typename slist<T, Alignment, Allocator>::size_type erase_if(slist<T, Alignment, Allocator> &c, Predicate predicate) {
		// Erases all elements that satisfy the predicate pred from the container.
		return c.remove_if(predicate);
	}

} // namespace fermat

namespace std {
	/// insert_iterator
	///
	/// We borrow a trick from SGI STL here and define an insert_iterator
	/// specialization for slist. This allows slist insertions to be O(1)
	/// instead of O(n/2), due to caching of the previous node.
	///
	template<typename T, size_t Alignment, typename Allocator>
	class insert_iterator<fermat::slist<T, Alignment, Allocator> > {
	public:
		typedef fermat::slist<T, Alignment, Allocator> Container;
		typedef typename Container::const_reference const_reference;
		typedef typename Container::iterator iterator_type;
		typedef std::output_iterator_tag iterator_category;
		typedef void value_type;
		typedef void difference_type;
		typedef void pointer;
		typedef void reference;

	protected:
		Container &container;
		iterator_type it;

	public:
		insert_iterator(Container &x, iterator_type i)
			: container(x) {
			if (i == x.begin())
				it = x.before_begin();
			else
				it = x.previous(i);
		}

		insert_iterator<Container> &operator=(const_reference value) {
			it = container.insert_after(it, value);
			return *this;
		}

		insert_iterator<Container> &operator*() { return *this; }

		insert_iterator<Container> &operator++() { return *this; } // This is by design.

		insert_iterator<Container> &operator++(int) { return *this; } // This is by design.
	}; // insert_iterator<slist>
}  // namespace std

KM_RESTORE_SN_WARNING()

KM_RESTORE_VC_WARNING();


#endif // Header include guard
