/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef FERMAT_INTERNAL_TYPE_PROPERTIES_H
#define FERMAT_INTERNAL_TYPE_PROPERTIES_H


#include <fermat/base/eabase.h>
#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <limits.h>


namespace fermat {
    ///////////////////////////////////////////////////////////////////////
    // is_bounded_array
    //
    // is_bounded_array<T>::value == true if T is an array type of known bound.
    //
    // is_bounded_array<int>::value is false.
    // is_bounded_array<int[5]>::value is true.
    // is_bounded_array<int[]>::value is false.
    //
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_is_bounded_array_CONFORMANCE 1    // is_bounded_array is conforming.

    template<class T>
    struct is_bounded_array : std::false_type {
    };

    template<class T, size_t N>
    struct is_bounded_array<T[N]> : std::true_type {
    };

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
    template<class T>
    constexpr bool is_bounded_array_v = is_bounded_array<T>::value;
#endif

    ///////////////////////////////////////////////////////////////////////
    // is_unbounded_array
    //
    // is_unbounded_array<T>::value == true if T is an array type of known bound.
    //
    // is_unbounded_array<int>::value is false.
    // is_unbounded_array<int[5]>::value is false.
    // is_unbounded_array<int[]>::value is true.
    //
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_is_unbounded_array_CONFORMANCE 1    // is_unbounded_array is conforming.

    template<class T>
    struct is_unbounded_array : std::false_type {
    };

    template<class T>
    struct is_unbounded_array<T[]> : std::true_type {
    };

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
    template<class T>
    constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;
#endif

    ///////////////////////////////////////////////////////////////////////
    // alignment_of
    //
    // alignment_of<T>::value is an integral value representing, in bytes,
    // the memory alignment of objects of type T.
    //
    // alignment_of may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_alignment_of_CONFORMANCE 1    // alignment_of is conforming.

    template<typename T>
    struct alignment_of_value {
        static const size_t value = FERMAT_ALIGN_OF(T);
    };

    template<typename T>
    struct alignment_of : public integral_constant<size_t, alignment_of_value<T>::value> {
    };

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
    template<class T>
    constexpr size_t alignment_of_v = alignment_of<T>::value;
#endif


    ///////////////////////////////////////////////////////////////////////
    // is_aligned
    //
    // Defined as true if the type has alignment requirements greater
    // than default alignment, which is taken to be 8. This allows for
    // doing specialized object allocation and placement for such types.
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_is_aligned_CONFORMANCE 1    // is_aligned is conforming.

    template<typename T>
    struct is_aligned_value {
        static const bool value = (FERMAT_ALIGN_OF(T) > 8);
    };

    template<typename T>
    struct is_aligned : public integral_constant<bool, is_aligned_value<T>::value> {
    };

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
    template<class T>
    constexpr size_t is_aligned_v = is_aligned<T>::value;
#endif


    ///////////////////////////////////////////////////////////////////////
    // rank
    //
    // rank<T>::value is an integral value representing the number of
    // dimensions possessed by an array type. For example, given a
    // multi-dimensional array type T[M][N], std::tr1::rank<T[M][N]>::value == 2.
    // For a given non-array type T, std::tr1::rank<T>::value == 0.
    //
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_rank_CONFORMANCE 1    // rank is conforming.

    template<typename T>
    struct rank : public fermat::integral_constant<size_t, 0> {
    };

    template<typename T>
    struct rank<T[]> : public fermat::integral_constant<size_t, rank<T>::value + 1> {
    };

    template<typename T, size_t N>
    struct rank<T[N]> : public fermat::integral_constant<size_t, rank<T>::value + 1> {
    };

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
    template<class T>
    constexpr auto rank_v = rank<T>::value;
#endif


    ///////////////////////////////////////////////////////////////////////
    // has_equality
    //
    // Determines if the specified type can be tested for equality.
    //
    ///////////////////////////////////////////////////////////////////////
    template<typename, typename = fermat::void_t<> >
    struct has_equality : std::false_type {
    };

    template<typename T>
    struct has_equality<T, fermat::void_t<decltype(std::declval<T>() == std::declval<T>())> > : std::true_type {
    };

#if FERMAT_VARIABLE_TEMPLATES_ENABLED
    template<class T>
    constexpr auto has_equality_v = has_equality<T>::value;
#endif

    namespace internal {
        ///////////////////////////////////////////////////////////////////////
        // is_complete_type
        //
        // Determines if the specified type is complete
        //
        // Warning: Be careful when using is_complete_type since the value is fixed at first instantiation.
        // Consider the following:
        //
        // struct Foo;
        // is_complete_type_v<Foo> // false
        // struct Foo {};
        // is_complete_type_v<Foo> // still false
        ///////////////////////////////////////////////////////////////////////

        template<typename T, typename = void>
        struct is_complete_type : public std::false_type {
        };

        template<typename T>
        struct is_complete_type<T, fermat::void_t<decltype(sizeof(T) != 0)> > : public std::true_type {
        };

        template<>
        struct is_complete_type<const volatile void> : public std::false_type {
        };

        template<>
        struct is_complete_type<const void> : public std::false_type {
        };

        template<>
        struct is_complete_type<volatile void> : public std::false_type {
        };

        template<>
        struct is_complete_type<void> : public std::false_type {
        };

        template<typename T>
        struct is_complete_type<T, std::enable_if_t<std::is_function_v<T> > > : public std::true_type {
        };

        template<typename T>
		FERMAT_CPP17_INLINE_VARIABLE constexpr bool is_complete_type_v = is_complete_type<T, void>::value;
    }
} // namespace fermat


#endif // Header include guard
