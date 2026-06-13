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
#ifndef RANGES_V3_ALGORITHM_FILL_N_HPP
#define RANGES_V3_ALGORITHM_FILL_N_HPP

#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(fill_n)

        /// \brief function template \c equal
        template(typename O, typename V)(
            requires output_iterator<O, V const &>)
        constexpr O RANGES_FUNC(fill_n)(O first, iter_difference_t<O> n, V const & val)
        {
            RANGES_EXPECT(n >= 0);
            auto norig = n;
            auto b = uncounted(first);
            for(; n != 0; ++b, --n)
                *b = val;
            return recounted(first, b, norig);
        }

    RANGES_FUNC_END(fill_n)

    namespace cpp20
    {
        using ranges::fill_n;
    }
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
