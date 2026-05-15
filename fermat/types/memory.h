///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef FERMAT_MEMORY_H
#define FERMAT_MEMORY_H


#include <fermat/types/internal/config.h>
#include <fermat/types/internal/memory_base.h>
#include <fermat/types/internal/memory_uses_allocator.h>
#include <fermat/types/algorithm.h>
#include <fermat/types/type_traits.h>
#include <iterator>
#include <utility>

KM_DISABLE_ALL_VC_WARNINGS()
#include <stdlib.h>
#include <new>
KM_RESTORE_ALL_VC_WARNINGS()


// 4530 - C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
// 4146 - unary minus operator applied to unsigned type, result still unsigned
// 4571 - catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
KM_DISABLE_VC_WARNING(4530 4146 4571);


#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.

#endif


namespace fermat {
	/// FERMAT_TEMP_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
#ifndef FERMAT_TEMP_DEFAULT_NAME
#define FERMAT_TEMP_DEFAULT_NAME FERMAT_DEFAULT_NAME_PREFIX " temp" // Unless the user overrides something, this is "EASTL temp".

#endif

	/// uninitialized_value_construct
	///
	/// Constructs objects in the uninitialized storage range [first, last) by value-initialization.
	///
	/// Value-Initialization:
	/// If T is a class, the object is default-initialized (after being zero-initialized if T's default
	/// constructor is not user-provided/deleted); otherwise, the object is zero-initialized.
	///
	/// http://en.cppreference.com/w/cpp/memory/uninitialized_value_construct
	///
	template<class ForwardIterator>
	void uninitialized_value_construct(ForwardIterator first, ForwardIterator last) {
		typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
		ForwardIterator currentDest(first);

#if FERMAT_EXCEPTIONS_ENABLED
		try {
#endif
		for (; currentDest != last; ++currentDest)
			::new(fermat::addressof(*currentDest)) value_type();
#if FERMAT_EXCEPTIONS_ENABLED
		}
		catch (...) {
			for (; first < currentDest; ++first)
				(*first).~value_type();
			throw;
		}
#endif
	}

	/// uninitialized_value_construct_n
	///
	/// Constructs n objects in the uninitialized storage starting at first by value-initialization.
	///
	/// Value-Initialization:
	/// If T is a class, the object is default-initialized (after being zero-initialized if T's default
	/// constructor is not user-provided/deleted); otherwise, the object is zero-initialized.
	///
	/// http://en.cppreference.com/w/cpp/memory/uninitialized_value_construct_n
	///
	template<class ForwardIterator, class Count>
	ForwardIterator uninitialized_value_construct_n(ForwardIterator first, Count n) {
		typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
		ForwardIterator currentDest(first);

#if FERMAT_EXCEPTIONS_ENABLED
		try {
#endif
		for (; n > 0; --n, ++currentDest)
			::new(fermat::addressof(*currentDest)) value_type();
		return currentDest;
#if FERMAT_EXCEPTIONS_ENABLED
		}
		catch (...) {
			for (; first < currentDest; ++first)
				(*first).~value_type();
			throw;
		}
#endif
	}

	/// uninitialized_copy_copy
	///
	/// Copies [first1, last1) into [result, result + (last1 - first1)) then
	/// copies [first2, last2) into [result, result + (last1 - first1) + (last2 - first2)).
	///
	template<typename InputIterator1, typename InputIterator2, typename ForwardIterator>
	inline ForwardIterator
	uninitialized_copy_copy(InputIterator1 first1, InputIterator1 last1,
	                        InputIterator2 first2, InputIterator2 last2,
	                        ForwardIterator result) {
		const ForwardIterator mid(std::uninitialized_copy(first1, last1, result));

#if FERMAT_EXCEPTIONS_ENABLED
		typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
		try {
#endif
		return std::uninitialized_copy(first2, last2, mid);
#if FERMAT_EXCEPTIONS_ENABLED
			}
			catch(...) {
			for (; result < mid; ++result)
				(*result).~value_type();
			throw;
		}
#endif
	}


	/// destruct
	///
	/// Calls the destructor of a given object.
	///
	/// Note that we don't have a specialized version of this for objects
	/// with trivial destructors, such as integers. This is because the
	/// compiler can already see in our version here that the destructor
	/// is a no-op.
	///
	template<typename T>
	inline void destruct(T *p) {
		// https://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k(C4100)&rd=true
		// "C4100 can also be issued when code calls a destructor on a otherwise unreferenced parameter
		//  of primitive type. This is a limitation of the Visual C++ compiler."
		KM_UNUSED(p);
		p->~T();
	}


	// destruct(first, last)
	//
	template<typename ForwardIterator>
	inline void destruct_impl(ForwardIterator /*first*/, ForwardIterator /*last*/, std::true_type)
	// true means the type has a trivial destructor.
	{
		// Empty. The type has a trivial destructor.
	}

	template<typename ForwardIterator>
	inline void destruct_impl(ForwardIterator first, ForwardIterator last, std::false_type)
	// false means the type has a significant destructor.
	{
		typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;

		for (; first != last; ++first)
			(*first).~value_type();
	}

	/// destruct
	///
	/// Calls the destructor on a range of objects.
	///
	/// We have a specialization for objects with trivial destructors, such as
	/// PODs. In this specialization the destruction of the range is a no-op.
	///
	template<typename ForwardIterator>
	inline void destruct(ForwardIterator first, ForwardIterator last) {
		typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
		destruct_impl(first, last, std::is_trivially_destructible<value_type>());
	}



	/// align
	///
	/// Same as C++11 std::align. http://en.cppreference.com/w/cpp/memory/align
	/// If it is possible to fit size bytes of storage aligned by alignment into the buffer pointed to by
	/// ptr with length space, the function updates ptr to point to the first possible address of such storage,
	/// decreases space by the number of bytes used for alignment, and returns the new ptr value. Otherwise,
	/// the function returns NULL and leaves ptr and space unmodified.
	///
	/// Example usage:
	///     char   buffer[512];
	///     size_t space = sizeof(buffer);
	///     void*  p  = buffer;
	///     void*  p1 = fermat::align(16,  3, p, space); p = (char*)p +  3; space -=  3;
	///     void*  p2 = fermat::align(32, 78, p, space); p = (char*)p + 78; space -= 78;
	///     void*  p3 = fermat::align(64,  9, p, space); p = (char*)p +  9; space -=  9;

	inline void *align(size_t alignment, size_t size, void *&ptr, size_t &space) {
		if (space >= size) {
			char *ptrAligned = (char *) (((size_t) ptr + (alignment - 1)) & -alignment);
			size_t offset = (size_t) (ptrAligned - (char *) ptr);

			if ((space - size) >= offset)
			// Have to implement this in terms of subtraction instead of addition in order to handle possible overflow.
			{
				ptr = ptrAligned;
				space -= offset;

				return ptrAligned;
			}
		}

		return NULL;
	}


	/// align_advance
	///
	/// Same as align except ptr and space can be adjusted to reflect remaining space.
	/// Not present in the C++ Standard.
	/// Note that the example code here is similar to align but simpler.
	///
	/// Example usage:
	///     char   buffer[512];
	///     size_t space = sizeof(buffer);
	///     void*  p  = buffer;
	///     void*  p1 = fermat::align_advance(16,  3, p, space, &p, &space); // p is advanced and space reduced accordingly.
	///     void*  p2 = fermat::align_advance(32, 78, p, space, &p, &space);
	///     void*  p3 = fermat::align_advance(64,  9, p, space, &p, &space);
	///     void*  p4 = fermat::align_advance(16, 33, p, space);

	inline void *align_advance(size_t alignment, size_t size, void *ptr, size_t space, void **ptrAdvanced = NULL,
	                           size_t *spaceReduced = NULL) {
		if (space >= size) {
			char *ptrAligned = (char *) (((size_t) ptr + (alignment - 1)) & -alignment);
			size_t offset = (size_t) (ptrAligned - (char *) ptr);

			if ((space - size) >= offset)
			// Have to implement this in terms of subtraction instead of addition in order to handle possible overflow.
			{
				if (ptrAdvanced)
					*ptrAdvanced = (ptrAligned + size);
				if (spaceReduced)
					*spaceReduced = (space - (offset + size));

				return ptrAligned;
			}
		}

		return NULL;
	}
} // namespace fermat


KM_RESTORE_VC_WARNING();


#endif // Header include guard
