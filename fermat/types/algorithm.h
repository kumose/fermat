/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements some of the primary algorithms from the C++ STL
// algorithm library. These versions are just like that STL versions and so
// are redundant. They are provided solely for the purpose of projects that
// either cannot use standard C++ STL or want algorithms that have guaranteed
// identical behaviour across platforms.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Definitions
//
// You will notice that we are very particular about the templated typenames
// we use here. You will notice that we follow the C++ standard closely in
// these respects. Each of these typenames have a specific meaning;
// this is why we don't just label templated arguments with just letters
// such as T, U, V, A, B. Here we provide a quick reference for the typenames
// we use. See the C++ standard, section 25-8 for more details.
//    --------------------------------------------------------------
//    typename                     Meaning
//    --------------------------------------------------------------
//    T                            The value type.
//    Compare                      A function which takes two arguments and returns the lesser of the two.
//    Predicate                    A function which takes one argument returns true if the argument meets some criteria.
//    BinaryPredicate              A function which takes two arguments and returns true if some criteria is met (e.g. they are equal).
//    StrickWeakOrdering           A BinaryPredicate that compares two objects, returning true if the first precedes the second. Like Compare but has additional requirements. Used for sorting routines.
//    Function                     A function which takes one argument and applies some operation to the target.
//    Size                         A count or size.
//    Generator                    A function which takes no arguments and returns a value (which will usually be assigned to an object).
//    UnaryOperation               A function which takes one argument and returns a value (which will usually be assigned to second object).
//    BinaryOperation              A function which takes two arguments and returns a value (which will usually be assigned to a third object).
//    InputIterator                An input iterator (iterator you read from) which allows reading each element only once and only in a forward direction.
//    ForwardIterator              An input iterator which is like InputIterator except it can be reset back to the beginning.
//    BidirectionalIterator        An input iterator which is like ForwardIterator except it can be read in a backward direction as well.
//    RandomAccessIterator         An input iterator which can be addressed like an array. It is a superset of all other input iterators.
//    OutputIterator               An output iterator (iterator you write to) which allows writing each element only once in only in a forward direction.
//
// Note that with iterators that a function which takes an InputIterator will
// also work with a ForwardIterator, BidirectionalIterator, or RandomAccessIterator.
// The given iterator type is merely the -minimum- supported functionality the
// iterator must support.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Optimizations
//
// There are a number of opportunities for optimizations that we take here
// in this library. The most obvious kinds are those that subsitute memcpy
// in the place of a conventional loop for data types with which this is
// possible. The algorithms here are optimized to a higher level than currently
// available C++ STL algorithms from vendors such as Microsoft. This is especially
// so for game programming on console devices, as we do things such as reduce
// branching relative to other STL algorithm implementations. However, the
// proper implementation of these algorithm optimizations is a fairly tricky
// thing.
//
// The various things we look to take advantage of in order to implement
// optimizations include:
//    - Taking advantage of random access iterators.
//    - Taking advantage of trivially copyable data types (types for which it is safe to memcpy or memmove).
//    - Taking advantage of type_traits in general.
//    - Reducing branching and taking advantage of likely branch predictions.
//    - Taking advantage of issues related to pointer and reference aliasing.
//    - Improving cache coherency during memory accesses.
//    - Making code more likely to be inlinable by the compiler.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <fermat/types/internal/config.h>
#include <fermat/types/type_traits.h>
#include <fermat/types/internal/copy_help.h>
#include <fermat/types/internal/fill_help.h>
#include <initializer_list>
#include <functional>
#include <utility>
#include <fermat/types/random.h>
#include <fermat/types/compare.h>

KM_DISABLE_ALL_VC_WARNINGS();

#if defined(KM_COMPILER_MSVC) && (defined(KM_PROCESSOR_X86) || defined(KM_PROCESSOR_X86_64))
#include <intrin.h>
#endif

#include <cstddef>
#include <cstring> // memcpy, memcmp, memmove

KM_RESTORE_ALL_VC_WARNINGS();

#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.

#endif


///////////////////////////////////////////////////////////////////////////////
// min/max workaround
//
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. The following may still not completely resolve some kinds of
// problems with MSVC++ #defines, though it deals with most cases in production
// game code.
//
#if FERMAT_NOMINMAX
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif


namespace fermat {
	/// min_alt
	///
	/// This is an alternative version of min that avoids any possible
	/// collisions with Microsoft #defines of min and max.
	///
	/// See min(a, b) for detailed specifications.
	///
	template<typename T>
	inline constexpr typename std::enable_if<std::is_scalar<T>::value, T>::type
	min_alt(T a, T b) {
		return b < a ? b : a;
	}

	template<typename T>
	inline typename std::enable_if<!std::is_scalar<T>::value, const T &>::type
	min_alt(const T &a, const T &b) {
		return b < a ? b : a;
	}

	inline constexpr float min_alt(float a, float b) { return b < a ? b : a; }
	inline constexpr double min_alt(double a, double b) { return b < a ? b : a; }
	inline constexpr long double min_alt(long double a, long double b) { return b < a ? b : a; }


	/// min_alt
	///
	/// This is an alternative version of min that avoids any possible
	/// collisions with Microsoft #defines of min and max.
	///
	/// See min(a, b) for detailed specifications.
	///
	template<typename T, typename Compare>
	inline const T &
	min_alt(const T &a, const T &b, Compare compare) {
		return compare(b, a) ? b : a;
	}

	/// max_alt
	///
	/// This is an alternative version of max that avoids any possible
	/// collisions with Microsoft #defines of min and max.
	///
	template<typename T>
	inline constexpr typename std::enable_if<std::is_scalar<T>::value, T>::type
	max_alt(T a, T b) {
		return a < b ? b : a;
	}

	template<typename T>
	inline constexpr typename std::enable_if<!std::is_scalar<T>::value, const T &>::type
	max_alt(const T &a, const T &b) {
		return a < b ? b : a;
	}

	inline constexpr float max_alt(float a, float b) { return a < b ? b : a; }
	inline constexpr double max_alt(double a, double b) { return a < b ? b : a; }
	inline constexpr long double max_alt(long double a, long double b) { return a < b ? b : a; }


	/// max_alt
	///
	/// This is an alternative version of max that avoids any possible
	/// collisions with Microsoft #defines of min and max.
	///
	template<typename T, typename Compare>
	inline const T &
	max_alt(const T &a, const T &b, Compare compare) {
		return compare(a, b) ? b : a;
	}


	template<typename T>
	inline T &&median_impl(T &&a, T &&b, T &&c) {
		if (a < b) {
			if (b < c)
				return std::forward<T>(b);
			else if (a < c)
				return std::forward<T>(c);
			else
				return std::forward<T>(a);
		} else if (a < c)
			return std::forward<T>(a);
		else if (b < c)
			return std::forward<T>(c);
		return std::forward<T>(b);
	}

	/// median
	///
	/// median finds which element of three (a, b, d) is in-between the other two.
	/// If two or more elements are equal, the first (e.g. a before b) is chosen.
	///
	/// Complexity: Either two or three comparisons will be required, depending
	/// on the values.
	///
	template<typename T>
	inline const T &median(const T &a, const T &b, const T &c) {
		return median_impl(a, b, c);
	}

	/// median
	///
	/// median finds which element of three (a, b, d) is in-between the other two.
	/// If two or more elements are equal, the first (e.g. a before b) is chosen.
	///
	/// Complexity: Either two or three comparisons will be required, depending
	/// on the values.
	///
	template<typename T>
	inline T &&median(T &&a, T &&b, T &&c) {
		return std::forward<T>(median_impl(std::forward<T>(a), std::forward<T>(b), std::forward<T>(c)));
	}


	template<typename T, typename Compare>
	inline T &&median_impl(T &&a, T &&b, T &&c, Compare compare) {
		if (compare(a, b)) {
			if (compare(b, c))
				return std::forward<T>(b);
			else if (compare(a, c))
				return std::forward<T>(c);
			else
				return std::forward<T>(a);
		} else if (compare(a, c))
			return std::forward<T>(a);
		else if (compare(b, c))
			return std::forward<T>(c);
		return std::forward<T>(b);
	}


	/// median
	///
	/// median finds which element of three (a, b, d) is in-between the other two.
	/// If two or more elements are equal, the first (e.g. a before b) is chosen.
	///
	/// Complexity: Either two or three comparisons will be required, depending
	/// on the values.
	///
	template<typename T, typename Compare>
	inline const T &median(const T &a, const T &b, const T &c, Compare compare) {
		return median_impl<const T &, Compare>(a, b, c, compare);
	}

	/// median
	///
	/// median finds which element of three (a, b, d) is in-between the other two.
	/// If two or more elements are equal, the first (e.g. a before b) is chosen.
	///
	/// Complexity: Either two or three comparisons will be required, depending
	/// on the values.
	///
	template<typename T, typename Compare>
	inline T &&median(T &&a, T &&b, T &&c, Compare compare) {
		return std::forward<T>(
			median_impl<T &&, Compare>(std::forward<T>(a), std::forward<T>(b), std::forward<T>(c), compare));
	}


	/// identical
	///
	/// Returns true if the two input ranges are equivalent.
	/// There is a subtle difference between this algorithm and
	/// the 'equal' algorithm. The equal algorithm assumes the
	/// two ranges are of equal length. This algorithm efficiently
	/// compares two ranges for both length equality and for
	/// element equality. There is no other standard algorithm
	/// that can do this.
	///
	/// Returns: true if the sequence of elements defined by the range
	/// [first1, last1) is of the same length as the sequence of
	/// elements defined by the range of [first2, last2) and if
	/// the elements in these ranges are equal as per the
	/// equal algorithm.
	///
	/// Complexity: At most 'min((last1 - first1), (last2 - first2))' applications
	/// of the corresponding comparison.
	///
	template<typename InputIterator1, typename InputIterator2>
	bool identical(InputIterator1 first1, InputIterator1 last1,
	               InputIterator2 first2, InputIterator2 last2) {
		while ((first1 != last1) && (first2 != last2) && (*first1 == *first2)) {
			++first1;
			++first2;
		}
		return (first1 == last1) && (first2 == last2);
	}


	/// identical
	///
	template<typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
	bool identical(InputIterator1 first1, InputIterator1 last1,
	               InputIterator2 first2, InputIterator2 last2, BinaryPredicate predicate) {
		while ((first1 != last1) && (first2 != last2) && predicate(*first1, *first2)) {
			++first1;
			++first2;
		}
		return (first1 == last1) && (first2 == last2);
	}

	/// apply_and_remove_if
	///
	/// Calls the Function function for all elements referred to  my iterator i in the range
	/// [first, last) for which the following corresponding condition holds:
	/// predicate(*i) == true
	/// and then left shift moves potential non-matching elements over it.
	///
	/// Returns: a past-the-end iterator for the new end of the range.
	///
	/// Complexity: Exactly 'last - first' applications of the corresponding predicate + applies
	/// function once for every time the condition holds.
	///
	/// Note: Since removing is done by shifting (by means of copy move assignment) the elements
	/// in the range in such a way that the elements that are not to be removed appear in the
	/// beginning of the range doesn't actually remove it from the given container, the user must call
	/// the container erase function if the user wants to erase the element
	/// from the container. I.e. in the same they as for remove_if the excess elements
	/// are left in a valid but possibly moved from state.
	///
	template<typename ForwardIterator, typename Function, typename Predicate>
	inline ForwardIterator apply_and_remove_if(ForwardIterator first,
	                                           ForwardIterator last,
	                                           Function function,
	                                           Predicate predicate) {
		first = std::find_if(first, last, predicate);
		if (first != last) {
			function(*first);
			for (auto i = next(first); i != last; ++i) {
				if (predicate(*i)) {
					function(*i);
					continue;
				}
				*first = std::move(*i);
				++first;
			}
		}
		return first;
	}


	/// apply_and_remove
	///
	/// Calls the Function function for all elements referred to my iterator i in the range
	/// [first, last) for which the following corresponding condition holds:
	/// value == *i
	/// and then left shift moves potential non-matching elements over it.
	///
	/// Returns: a past-the-end iterator for the new end of the range.
	///
	/// Complexity: Exactly 'last - first' applications of the corresponding equality test
	/// + applies function once for every time the condition holds.
	///
	/// Note: Since removing is done by shifting (by means of copy move assignment) the elements
	/// in the range in such a way that the elements that are not to be removed appear in the
	/// beginning of the range doesn't actually remove it from the given container, the user must call
	/// the container erase function if the user wants to erase the element
	/// from the container. I.e. in the same they as for remove_if the excess elements
	/// are left in a valid but possibly moved from state.
	///
	template<typename ForwardIterator, typename Function, typename T>
	inline ForwardIterator apply_and_remove(ForwardIterator first,
	                                        ForwardIterator last,
	                                        Function function,
	                                        const T &value) {
		first = std::find(first, last, value);
		if (first != last) {
			function(*first);
			for (auto i = next(first); i != last; ++i) {
				if (value == *i) {
					function(*i);
					continue;
				}
				*first = std::move(*i);
				++first;
			}
		}
		return first;
	}

	/// binary_search_i
	///
	/// Returns: iterator if there is an iterator i in the range [first last) that
	/// satisfies the corresponding conditions: !(*i < value) && !(value < *i).
	/// Returns last if the value is not found.
	///
	/// Complexity: At most 'log(last - first) + 2' comparisons.
	///
	template<typename ForwardIterator, typename T>
	inline ForwardIterator
	binary_search_i(ForwardIterator first, ForwardIterator last, const T &value) {
		// To do: This can be made slightly faster by not using lower_bound.
		ForwardIterator i(std::lower_bound<ForwardIterator, T>(first, last, value));
		if ((i != last) && !(value < *i)) // Note that we always express value comparisons in terms of < or ==.
			return i;
		return last;
	}


	/// binary_search_i
	///
	/// Returns: iterator if there is an iterator i in the range [first last) that
	/// satisfies the corresponding conditions: !(*i < value) && !(value < *i).
	/// Returns last if the value is not found.
	///
	/// Complexity: At most 'log(last - first) + 2' comparisons.
	///
	template<typename ForwardIterator, typename T, typename Compare>
	inline ForwardIterator
	binary_search_i(ForwardIterator first, ForwardIterator last, const T &value, Compare compare) {
		// To do: This can be made slightly faster by not using lower_bound.
		ForwardIterator i(std::lower_bound<ForwardIterator, T, Compare>(first, last, value, compare));
		if ((i != last) && !compare(value, *i))
			return i;
		return last;
	}

	/// set_difference_2
	///
	/// set_difference_2 iterates over both input ranges and copies elements present
	/// in the first range but not the second to the first output range and copies
	/// elements present in the second range but not in the first to the second output
	/// range.
	///
	/// Effects: Copies the elements of the range [first1, last1) which are not
	/// present in the range [first2, last2) to the first output range beginning at
	/// result1 AND copies the element of range [first2, last2) which are not present
	/// in the range [first1, last) to the second output range beginning at result2.
	/// The elements in the constructed range are sorted.
	///
	/// Requires: The input ranges must be sorted.
	/// Requires: The output ranges shall not overlap with either of the original ranges.
	///
	/// Returns:  Nothing.
	///
	/// Complexity: At most (2 * ((last1 - first1) + (last2 - first2)) - 1) comparisons.
	///
	template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
	void set_difference_2(InputIterator1 first1, InputIterator1 last1,
	                      InputIterator2 first2, InputIterator2 last2,
	                      OutputIterator result1, OutputIterator result2, Compare compare) {
		while ((first1 != last1) && (first2 != last2)) {
			if (compare(*first1, *first2)) {
				FERMAT_VALIDATE_COMPARE(!compare(*first2, *first1)); // Validate that the compare function is sane.
				*result1++ = *first1++;
			} else if (compare(*first2, *first1)) {
				FERMAT_VALIDATE_COMPARE(!compare(*first1, *first2)); // Validate that the compare function is sane.
				*result2++ = *first2++;
			} else {
				++first1;
				++first2;
			}
		}

		std::copy(first2, last2, result2);
		std::copy(first1, last1, result1);
	}

	/// set_difference_2
	///
	///  set_difference_2 with the default comparison object is std::less<>.
	///
	template<typename InputIterator1, typename InputIterator2, typename OutputIterator>
	void set_difference_2(InputIterator1 first1, InputIterator1 last1,
	                      InputIterator2 first2, InputIterator2 last2,
	                      OutputIterator result1, OutputIterator result2) {
		fermat::set_difference_2(first1, last1, first2, last2, result1, result2, std::less<>{});
	}


	/// set_decomposition
	///
	/// set_decomposition iterates over both ranges and copies elements to one of the three
	/// categories of output ranges.
	///
	/// Effects: Constructs three sorted containers of the elements from the two ranges.
	///             * OutputIterator1 is elements only in Container1.
	///             * OutputIterator2 is elements only in Container2.
	///             * OutputIterator3 is elements that are in both Container1 and Container2.
	///
	/// Requires: The input ranges must be sorted.
	/// Requires: The resulting ranges shall not overlap with either of the original ranges.
	///
	/// Returns: The end of the constructed range of elements in both Container1 and Container2.
	///
	/// Complexity: At most (2 * ((last1 - first1) + (last2 - first2)) - 1) comparisons.
	///
	template<typename InputIterator1, typename InputIterator2,
		typename OutputIterator1, typename OutputIterator2, typename OutputIterator3, typename Compare>
	OutputIterator3 set_decomposition(InputIterator1 first1, InputIterator1 last1,
	                                  InputIterator2 first2, InputIterator2 last2,
	                                  OutputIterator1 result1, OutputIterator2 result2, OutputIterator3 result3,
	                                  Compare compare) {
		while ((first1 != last1) && (first2 != last2)) {
			if (compare(*first1, *first2)) {
				FERMAT_VALIDATE_COMPARE(!compare(*first2, *first1)); // Validate that the compare function is sane.
				*result1++ = *first1++;
			} else if (compare(*first2, *first1)) {
				FERMAT_VALIDATE_COMPARE(!compare(*first1, *first2)); // Validate that the compare function is sane.
				*result2++ = *first2++;
			} else {
				*result3++ = *first1++;
				++first2;
			}
		}

		std::copy(first1, last1, result1);
		std::copy(first2, last2, result2);

		return result3;
	}

	/// set_decomposition
	///
	///  set_decomposition with the default comparison object is std::less<>.
	///
	template<typename InputIterator1, typename InputIterator2,
		typename OutputIterator1, typename OutputIterator2, typename OutputIterator3>
	OutputIterator3 set_decomposition(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
	                                  InputIterator2 last2,
	                                  OutputIterator1 result1, OutputIterator2 result2, OutputIterator3 result3) {
		return fermat::set_decomposition(first1, last1, first2, last2, result1, result2, result3, std::less<>{});
	}
} // namespace fermat

