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
#ifndef RANGES_V3_ALGORITHM_FILL_HPP
#define RANGES_V3_ALGORITHM_FILL_HPP

#include <fermat/range_fwd.h>

#include <fermat/iterator/concepts.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(fill)

        /// \brief function template \c fill
        template(typename O, typename S, typename V)(
            requires output_iterator<O, V const &> AND sentinel_for<S, O>)
        constexpr O RANGES_FUNC(fill)(O first, S last, V const & val) //
        {
            for(; first != last; ++first)
                *first = val;
            return first;
        }

        /// \overload
        template(typename Rng, typename V)(
            requires output_range<Rng, V const &>)
        constexpr borrowed_iterator_t<Rng> RANGES_FUNC(fill)(Rng && rng, V const & val)
        {
            return (*this)(begin(rng), end(rng), val);
        }

    RANGES_FUNC_END(fill)

    namespace cpp20
    {
        using fermat::ranges::fill;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
