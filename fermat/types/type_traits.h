///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Specification
//
// This file implements C++ type traits as proposed by the emerging C++ update
// as of May, 2005. This update is known as "Proposed Draft Technical Report
// on C++ Library Extensions" and is document number n1745. It can be found
// on the Internet as n1745.pdf and as of this writing it is updated every
// couple months to reflect current thinking.
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Description

// EASTL extension type traits
//    is_aligned                            Defined as true if the type has alignment requirements greater than default alignment, which is taken to be 8. is_aligned is not found in Boost nor C++11, though alignment_of is.
//    union_cast                            Allows for easy-to-read casting between types that are unrelated but have binary equivalence. The classic use case is converting between float and int32_t bit representations.
//    yes_type
//    no_type
//    is_swappable                          Found in <EASTL/utility.h>
//    is_nothrow_swappable                  "
//    is_reference_wrapper                  Found in <EASTL/functional.h>
//    remove_reference_wrapper              "
//    is_detected                           Checks if some supplied arguments (Args) respect a constraint (Op).
//    detected_t                            Check which type we obtain after expanding some arguments (Args) over a constraint (Op).
//    detected_or                           Checks if some supplied arguments (Args) respect a constraint (Op) and allow to overwrite return type.
//    detected_or_t                         Equivalent to detected_or<Default, Op, Args...>::type.
//    is_detected_exact                     Check that the type we obtain after expanding some arguments (Args) over a constraint (Op) is equivalent to Expected.
//    is_detected_convertible               Check that the type we obtain after expanding some arguments (Args) over a constraint (Op) is convertible to Expected.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Requirements
//
// As of this writing (5/2005), type_traits here requires a well-conforming
// C++ compiler with respect to template metaprogramming. To use this library
// you need to have at least one of the following:
//     MSVC++ 7.1       (includes Win32, Win64, and WinCE platforms)
//     GCC 3.2          (includes MacOSX, and Linux platforms)
//     Metrowerks 8.0   (incluees MacOSX, Windows, and other platforms)
//     EDG              (includes any compiler with EDG as a back-end, such as the Intel compiler)
//     Comeau           (this is a C++ to C generator)
//
// It may be useful to list the compilers/platforms the current version of
// type_traits doesn't support:
//     Borland C++      (it simply has too many bugs with respect to templates).
//     GCC 2.96         We used to have a separate set of type traits for this compiler, but removed it due to lack of necessity.
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implementation
//
// The implementation here is almost entirely based on template metaprogramming.
// This is whereby you use the compiler's template functionality to define types
// and values and make compilation decisions based on template declarations.
// Many of the algorithms here are similar to those found in books such as
// "Modern C++ Design" and C++ libraries such as Boost. The implementations here
// are simpler and more straightforward than those found in some libraries, due
// largely to our assumption that the compiler is good at doing template programming.
///////////////////////////////////////////////////////////////////////////////


#ifndef FERMAT_TYPE_TRAITS_H
#define FERMAT_TYPE_TRAITS_H


#include <fermat/types/internal/config.h>
#include <cstddef>                 // Is needed for size_t usage by some traits.

#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.

#endif


namespace fermat {
	///////////////////////////////////////////////////////////////////////
	// integral_constant
	//
	// This is the base class for various type traits, as defined by C++11.
	// This is essentially a utility base class for defining properties
	// as both class constants (value) and as types (type).
	//
	template<typename T, T v>
	struct integral_constant {
		static constexpr T value = v;
		typedef T value_type;
		typedef integral_constant<T, v> type;

		constexpr operator value_type() const noexcept { return value; }
		constexpr value_type operator()() const noexcept { return value; }
	};

	///////////////////////////////////////////////////////////////////////
	// yes_type / no_type
	//
	// These are used as a utility to differentiate between two things.
	//
	typedef char yes_type; // sizeof(yes_type) == 1
	struct no_type {
		char padding[8];
	}; // sizeof(no_type)  != 1


	///////////////////////////////////////////////////////////////////////
	// unused
	//
	// Used internally to denote a special template argument that means
	// it's an unused argument.
	//
	struct unused {
	};


	///////////////////////////////////////////////////////////////////////
	// argument_sink
	//
	// Used as a type which constructs from anything.
	//
	// For compilers that support variadic templates we provide an
	// alternative argument_sink which provides a constructor overload of
	// the variadic pack of arguments by reference.  This avoids issues of
	// object alignment not being respected in Microsoft compilers.  Seen
	// in VS2015 preview.  In general, since arguments are consumed and
	// ignored its cheaper to consume references than passing by value
	// which incurs a construction cost.
	struct argument_sink {
		template<typename... Args>
		argument_sink(Args &&...) {
		}
	};


	///////////////////////////////////////////////////////////////////////
	// first_type_select
	//
	//  Similar to std::conditional<> but unilaterally selects the first type.
	//
	template<typename T, typename = fermat::unused, typename = fermat::unused>
	struct first_type_select {
		typedef T type;
	};

	///////////////////////////////////////////////////////////////////////
	// type_identity
	//
	// The purpose of this is typically to deal with non-deduced template
	// contexts. See the C++11 Standard, 14.8.2.5 p5.
	// Also: http://cppquiz.org/quiz/question/109?result=CE&answer=&did_answer=Answer
	//
	// https://en.cppreference.com/w/cpp/types/type_identity
	//
	template<typename T>
	struct type_identity {
		using type = T;
	};

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
	template<typename T>
	using type_identity_t = typename type_identity<T>::type;
#endif


	///////////////////////////////////////////////////////////////////////
	// remove_reference
	//
	// Remove reference from a type.
	//
	// The remove_reference transformation trait removes top-level of
	// indirection by reference (if any) from the type to which it is applied.
	// For a given type T, remove_reference<T&>::type is equivalent to T.
	//
	///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_remove_reference_CONFORMANCE 1

	template<typename T>
	struct remove_reference {
		typedef T type;
	};

	template<typename T>
	struct remove_reference<T &> {
		typedef T type;
	};

	template<typename T>
	struct remove_reference<T &&> {
		typedef T type;
	};

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
	template<typename T>
	using remove_reference_t = typename remove_reference<T>::type;
#endif


	///////////////////////////////////////////////////////////////////////
	// remove_cvref
	//
	// Remove const and volatile from a reference type.
	//
	// The remove_cvref transformation trait removes top-level const and/or volatile
	// qualification (if any) from the reference type to which it is applied. For a given type T&,
	// remove_cvref<T& const volatile>::type is equivalent to T. For example,
	// std::remove_cv<int& volatile>::type is equivalent to int.
	//
	///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_remove_cvref_CONFORMANCE 1    // remove_cvref is conforming.

	template<typename T>
	struct remove_cvref {
		typedef typename std::remove_volatile<typename std::remove_const<typename fermat::remove_reference<
			T>::type>::type>::type type;
	};

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
	template<typename T>
	using remove_cvref_t = typename remove_cvref<T>::type;
#endif


	///////////////////////////////////////////////////////////////////////
	// static_min / static_max
	//
	// These are primarily useful in templated code for meta programming.
	// Currently we are limited to size_t, as C++ doesn't allow integral
	// template parameters to be generic. We can expand the supported types
	// to include additional integers if needed.
	//
	// These are not in the C++ Standard.
	//
	// Example usage:
	//     Printf("%zu", static_max<3, 7, 1, 5>::value); // prints "7"
	//
	///////////////////////////////////////////////////////////////////////
#define FERMAT_TYPE_TRAIT_static_min_CONFORMANCE 1
#define FERMAT_TYPE_TRAIT_static_max_CONFORMANCE 1

	template<size_t I0, size_t ... in>
	struct static_min;

	template<size_t I0>
	struct static_min<I0> {
		static const size_t value = I0;
	};

	template<size_t I0, size_t I1, size_t ... in>
	struct static_min<I0, I1, in...> {
		static const size_t value = ((I0 <= I1) ? static_min<I0, in...>::value : static_min<I1, in...>::value);
	};

	template<size_t I0, size_t ... in>
	struct static_max;

	template<size_t I0>
	struct static_max<I0> {
		static const size_t value = I0;
	};

	template<size_t I0, size_t I1, size_t ... in>
	struct static_max<I0, I1, in...> {
		static const size_t value = ((I0 >= I1) ? static_max<I0, in...>::value : static_max<I1, in...>::value);
	};

	///////////////////////////////////////////////////////////////////////
	/// This enum class is useful for detecting whether a system is little
	/// or big endian. Mixed or middle endian is not modeled here as described
	/// by the C++20 spec.
	///////////////////////////////////////////////////////////////////////
	KM_DISABLE_VC_WARNING(4472)
	// 'endian' is a native enum: add an access specifier (private/public) to declare a managed enum
	enum class endian {
#ifdef KM_SYSTEM_LITTLE_ENDIAN
		little = 1,
		big = 0,
		native = little
#else
		little=0,
		big=1,
		native= big
#endif
	};

	KM_RESTORE_VC_WARNING();
} // namespace fermat


// The following files implement the type traits themselves.
#include <fermat/types/internal/type_transformations.h>
#include <fermat/types/internal/type_void_t.h>
#include <fermat/types/internal/type_properties.h>
#include <fermat/types/internal/type_detected.h>

namespace fermat {
	namespace detail {
		template<typename, typename = void>
		struct is_transparent_comparison : std::false_type {
		};

		template<typename T>
		struct is_transparent_comparison<T, fermat::void_t<typename T::is_transparent> > : std::true_type {
		};

		template<typename T>
		constexpr bool is_transparent_comparison_v = is_transparent_comparison<T>::value;
	} // namespace detail
} // namespace fermat


#endif // Header include guard
