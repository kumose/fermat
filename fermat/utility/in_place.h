/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_UTILITY_IN_PLACE_HPP
#define RANGES_V3_UTILITY_IN_PLACE_HPP

#include <fermat/range_fwd.h>

#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \ingroup group-utility
    struct in_place_t
    {};
    RANGES_INLINE_VARIABLE(in_place_t, in_place)
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
