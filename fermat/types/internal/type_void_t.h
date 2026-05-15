/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef FERMAT_INTERNAL_TYPE_VOID_T_H
#define FERMAT_INTERNAL_TYPE_VOID_T_H


#include <fermat/base/eabase.h>
#if defined(KM_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

namespace fermat {
    ///////////////////////////////////////////////////////////////////////
    // void_t
    //
    // Maps a sequence of any types to void.  This utility class is used in
    // template meta programming to simplify compile time reflection mechanisms
    // required by the standard library.
    //
    // http://en.cppreference.com/w/cpp/types/void_t
    //
    // Example:
    //    template <typename T, typename = void>
    //    struct is_iterable : std::false_type {};
    //
    //    template <typename T>
    //    struct is_iterable<T, void_t<decltype(std::declval<T>().begin()),
    //                                 decltype(std::declval<T>().end())>> : std::true_type {};
    //
    ///////////////////////////////////////////////////////////////////////
    template<class...>
    using void_t = void;
} // namespace fermat


#endif // Header include guard
