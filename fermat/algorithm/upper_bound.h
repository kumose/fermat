/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_ALGORITHM_UPPER_BOUND_HPP
#define RANGES_V3_ALGORITHM_UPPER_BOUND_HPP

#include <fermat/range_fwd.h>

#include <fermat/algorithm/aux_/upper_bound_n.h>
#include <fermat/algorithm/partition_point.h>
#include <fermat/functional/comparisons.h>
#include <fermat/functional/identity.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(upper_bound)

        /// \brief function template \c upper_bound
        template(typename I,
                 typename S,
                 typename V,
                 typename C = less,
                 typename P = identity)(
            requires forward_iterator<I> AND sentinel_for<S, I> AND
                indirect_strict_weak_order<C, V const *, projected<I, P>>)
        constexpr I RANGES_FUNC(upper_bound)(
            I first, S last, V const & val, C pred = C{}, P proj = P{}) //
        {
            return partition_point(std::move(first),
                                   std::move(last),
                                   detail::make_upper_bound_predicate(pred, val),
                                   std::move(proj));
        }

        /// \overload
        template(typename Rng, typename V, typename C = less, typename P = identity)(
            requires forward_range<Rng> AND
                indirect_strict_weak_order<C, V const *, projected<iterator_t<Rng>, P>>)
        constexpr borrowed_iterator_t<Rng> RANGES_FUNC(upper_bound)(
            Rng && rng, V const & val, C pred = C{}, P proj = P{}) //
        {
            return partition_point(
                rng, detail::make_upper_bound_predicate(pred, val), std::move(proj));
        }

    RANGES_FUNC_END(upper_bound)

    namespace cpp20
    {
        using fermat::ranges::upper_bound;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
