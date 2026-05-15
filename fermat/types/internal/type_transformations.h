/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef FERMAT_INTERNAL_TYPE_TRANFORMATIONS_H
#define FERMAT_INTERNAL_TYPE_TRANFORMATIONS_H


#include <fermat/base/eabase.h>
#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <limits.h>


namespace fermat {
    ///////////////////////////////////////////////////////////////////////
    // add_pointer
    //
    // Add pointer to a type.
    // Provides the member typedef type which is the type T*.
    //
    // If T is a reference type,
    //		type member is a pointer to the referred type.
    // If T is an object type, a function type that is not cv- or ref-qualified,
    // or a (possibly cv-qualified) void type,
    //		type member is T*.
    // Otherwise (T is a cv- or ref-qualified function type),
    //		type member is T (ie. not a pointer).
    //
    // cv- and ref-qualified function types are invalid, which is why there is a specific clause for it.
    // See https://cplusplus.github.io/LWG/issue2101 for more.
    //
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_add_pointer_CONFORMANCE 1

    namespace internal {
        template<typename T>
        auto try_add_pointer(int) -> type_identity<typename fermat::remove_reference<T>::type *>;

        template<typename T>
        auto try_add_pointer(...) -> type_identity<T>;
    }

    template<typename T>
    struct add_pointer : decltype(internal::try_add_pointer<T>(0)) {
    };

    template<class T>
    using add_pointer_t = typename add_pointer<T>::type;


    ///////////////////////////////////////////////////////////////////////
    // aligned_storage
    //
    // Deprecated in C++23.
    //
    // The aligned_storage transformation trait provides a type that is
    // suitably aligned to store an object whose size is does not exceed length
    // and whose alignment is a divisor of alignment. When using aligned_storage,
    // length must be non-zero, and alignment must >= alignment_of<T>::value
    // for some type T. We require the alignment value to be a power-of-two.
    //
    // GCC versions prior to 4.4 don't properly support this with stack-based
    // variables. The EABase KM_ALIGN_MAX_AUTOMATIC define identifies the
    // extent to which stack (automatic) variables can be aligned for the
    // given compiler/platform combination.
    //
    // Example usage:
    //     aligned_storage<sizeof(Widget), alignment_of(Widget)>::type widget;
    //     Widget* pWidget = new(&widget) Widget;
    //
    //     aligned_storage<sizeof(Widget), 64>::type widgetAlignedTo64;
    //     Widget* pWidget = new(&widgetAlignedTo64) Widget;
    //
    //     aligned_storage<sizeof(Widget), alignment_of(Widget)>::type widgetArray[37];
    //     Widget* pWidgetArray = new(widgetArray) Widget[37];
    ///////////////////////////////////////////////////////////////////////

#define FERMAT_TYPE_TRAIT_aligned_storage_CONFORMANCE 1    // aligned_storage is conforming.

#if defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION >= 4008)
    // New versions of GCC do not support using 'alignas' with a value greater than 128.
    // However, this code using the GNU standard alignment attribute works properly.
    template<size_t N, size_t Align = FERMAT_ALIGN_OF(double)>
    struct aligned_storage {
        struct type {
            unsigned char mCharData[N];
        } KM_ALIGN(Align);
    };
#elif (EABASE_VERSION_N >= 20040) && !defined(KM_COMPILER_NO_ALIGNAS) // If C++11 alignas is supported...
    template<size_t N, size_t Align = FERMAT_ALIGN_OF(double)>
    struct aligned_storage {
        typedef struct {
            alignas(Align) unsigned char mCharData[N];
        } type;
    };

#elif defined(KM_COMPILER_MSVC) || (defined(KM_COMPILER_GNUC) && (KM_COMPILER_VERSION < 4007)) || defined(KM_COMPILER_EDG) // At some point GCC fixed their attribute(align) to support non-literals, though it's not clear what version aside from being no later than 4.7 and no earlier than 4.2.

    // Some compilers don't allow you to to use KM_ALIGNED with anything by a numeric literal,
    // so we can't use the simpler code like we do further below for other compilers. We support
    // only up to so much of an alignment value here.
    template<size_t N, size_t Align>
    struct aligned_storage_helper {
        struct type {
            unsigned char mCharData[N];
        };
    };

    template<size_t N>
    struct aligned_storage_helper<N, 2> {
        struct KM_ALIGN (
        2
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 4> {
        struct KM_ALIGN (
        4
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 8> {
        struct KM_ALIGN (
        8
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 16> {
        struct KM_ALIGN (
        16
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 32> {
        struct KM_ALIGN (
        32
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 64> {
        struct KM_ALIGN (
        64
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 128> {
        struct KM_ALIGN (
        128
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 256> {
        struct KM_ALIGN (
        256
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 512> {
        struct KM_ALIGN (
        512
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 1024> {
        struct KM_ALIGN (
        1024
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 2048> {
        struct KM_ALIGN (
        2048
        )
        type { unsigned char mCharData[N]; };
    };
    template<size_t N>
    struct aligned_storage_helper<N, 4096> {
        struct KM_ALIGN (
        4096
        )
        type { unsigned char mCharData[N]; };
    };

    template<size_t N, size_t Align = FERMAT_ALIGN_OF(double)>
    struct aligned_storage {
        typedef typename aligned_storage_helper<N, Align>::type type;
    };

#else
    template<size_t N, size_t Align = FERMAT_ALIGN_OF(double)>
    struct aligned_storage {
        union type {
            unsigned char mCharData[N];
            struct KM_ALIGN (Align) mStruct{};
        };
    };
#endif

#if defined(KM_COMPILER_NO_TEMPLATE_ALIASES)
#define FERMAT_ALIGNED_STORAGE_T(N, Align) typename fermat::aligned_storage_t<N, Align>::type
#else
    template<size_t N, size_t Align = FERMAT_ALIGN_OF(double)>
    using aligned_storage_t = typename aligned_storage<N, Align>::type;
#define FERMAT_ALIGNED_STORAGE_T(N, Align) fermat::aligned_storage_t<N, Align>
#endif


    ///////////////////////////////////////////////////////////////////////
    // aligned_union
    //
    // Deprecated in C++23.
    //
    // The member typedef type shall be a POD type suitable for use as
    // uninitialized storage for any object whose type is listed in Types;
    // its size shall be at least Len. The static member alignment_value
    // shall be an integral constant of type std::size_t whose value is
    // the strictest alignment of all types listed in Types.
    // Note that the resulting type is not a C/C++ union, but simply memory
    // block (of pod type) that can be used to placement-new an actual
    // C/C++ union of the types. The actual union you declare can be a non-POD union.
    //
    // Example usage:
    //     union MyUnion {
    //         char  c;
    //         int   i;
    //         float f;
    //
    //         MyUnion(float fValue) : f(fValue) {}
    //     };
    //
    //     aligned_union<sizeof(MyUnion), char, int, float>::type myUnionStorage;
    //     MyUnion* pMyUnion = new(&myUnionStorage) MyUnion(21.4f);
    //     pMyUnion->i = 37;
    //
    ///////////////////////////////////////////////////////////////////////

#if defined(KM_COMPILER_NO_VARIADIC_TEMPLATES) || !FERMAT_TYPE_TRAIT_static_max_CONFORMANCE
#define FERMAT_TYPE_TRAIT_aligned_union_CONFORMANCE 0    // aligned_union is not conforming, as it supports only a two-member unions.


    // To consider: Expand this to include more possible types. We may want to convert this to be a recursive
    //              template instead of like below.
    template<size_t minSize, typename Type0, typename Type1 = char, typename Type2 = char, typename Type3 = char>
    struct aligned_union {
        static const size_t size0 = fermat::static_max < minSize,
        sizeof
        (Type0)
        >
        ::value;
        static const size_t size1 = fermat::static_max < size0,
        sizeof
        (Type1)
        >
        ::value;
        static const size_t size2 = fermat::static_max < size1,
        sizeof
        (Type2)
        >
        ::value;
        static const size_t size = fermat::static_max < size2,
        sizeof
        (Type3)
        >
        ::value;

        static const size_t alignment0 = fermat::static_max < KM_ALIGN_OF(Type0), KM_ALIGN_OF(Type1)
        >
        ::value;
        static const size_t alignment1 = fermat::static_max < alignment0, KM_ALIGN_OF(Type2)
        >
        ::value;
        static const size_t alignment_value = fermat::static_max < alignment1, KM_ALIGN_OF(Type3)
        >
        ::value;

        typedef typename fermat::aligned_storage<size, alignment_value>::type type;
    };

#if defined(KM_COMPILER_NO_TEMPLATE_ALIASES)
    // To do: define macro.
#else
    template<size_t minSize, typename Type0, typename Type1 = char, typename Type2 = char, typename Type3 = char>
    using aligned_union_t = typename aligned_union<minSize, Type0, Type1, Type2, Type3>::type;
#endif
#else
#define FERMAT_TYPE_TRAIT_aligned_union_CONFORMANCE 1    // aligned_union is conforming.

    template<size_t minSize, typename Type0, typename... TypeN>
    struct aligned_union {
        static const size_t size = fermat::static_max < minSize,
        sizeof
        (Type0),
        sizeof
        (TypeN)
        ...
        >
        ::value;
        static const size_t alignment_value = fermat::static_max < KM_ALIGN_OF(Type0), KM_ALIGN_OF(TypeN)
        ...
        >
        ::value;

        typedef typename fermat::aligned_storage<size, alignment_value>::type type;
    };

#if defined(KM_COMPILER_NO_TEMPLATE_ALIASES)
    // To do: define macro.
#else
    template<size_t minSize, typename... TypeN>
    using aligned_union_t = typename aligned_union<minSize, TypeN...>::type;
#endif

#endif


    ///////////////////////////////////////////////////////////////////////
    // union_cast
    //
    // Safely converts between unrelated types that have a binary equivalency.
    // This appoach is required by strictly conforming C++ compilers because
    // directly using a C or C++ cast between unrelated types is fraught with
    // the possibility of undefined runtime behavior due to type aliasing.
    // The Source and Dest types must be POD types due to the use of a union
    // in C++ versions prior to C++11. C++11 relaxes the definition of a POD
    // such that it allows a classes with trivial default constructors whereas
    // previous versions did not, so beware of this when writing portable code.
    //
    // Example usage:
    //    float f32 = 1.234f;
    //    uint32_t n32 = union_cast<uint32_t>(f32);
    //
    // Example possible mis-usage:
    // The following is valid only if you are aliasing the pointer value and
    // not what it points to. Most of the time the user intends the latter,
    // which isn't strictly possible.
    //    Widget* pWidget = CreateWidget();
    //    Foo*    pFoo    = union_cast<Foo*>(pWidget);
    ///////////////////////////////////////////////////////////////////////

    template<typename DestType, typename SourceType>
    DestType union_cast(SourceType sourceValue) {
        FERMAT_CT_ASSERT((sizeof(DestType) == sizeof(SourceType)) &&
                         (KM_ALIGN_OF(DestType) == KM_ALIGN_OF(SourceType)));
        // To support differening alignments, we would need to use a memcpy-based solution or find a way to make the two union members align with each other.
        //FERMAT_CT_ASSERT(is_pod<DestType>::value && is_pod<SourceType>::value);           // Disabled because we don't want to restrict what the user can do, as some compiler's definitions of is_pod aren't up to C++11 Standards.
        //FERMAT_CT_ASSERT(!std::is_pointer<DestType>::value && !std::is_pointer<SourceType>::value); // Disabled because it's valid to alias pointers as long as you are aliasong the pointer value and not what it points to.

        union {
            SourceType sourceValue;
            DestType destValue;
        } u;
        u.sourceValue = sourceValue;

        return u.destValue;
    }
} // namespace fermat


#endif // Header include guard





















