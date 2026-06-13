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
#ifndef RANGES_V3_NUMERIC_IOTA_HPP
#define RANGES_V3_NUMERIC_IOTA_HPP

#include <fermat/iterator/concepts.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-numerics
    /// @{
    struct iota_fn
    {
        template(typename O, typename S, typename T)(
            requires output_iterator<O, T const &> AND sentinel_for<S, O> AND
                weakly_incrementable<T>)
        O operator()(O first, S last, T val) const
        {
            for(; first != last; ++first, ++val)
                *first = detail::as_const(val);
            return first;
        }

        template(typename Rng, typename T)(
            requires output_range<Rng, T const &> AND weakly_incrementable<T>)
        borrowed_iterator_t<Rng> operator()(Rng && rng, T val) const //
        {
            return (*this)(begin(rng), end(rng), detail::move(val));
        }
    };

    RANGES_INLINE_VARIABLE(iota_fn, iota)
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
