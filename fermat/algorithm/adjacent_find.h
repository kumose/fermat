/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_ALGORITHM_ADJACENT_FIND_HPP
#define RANGES_V3_ALGORITHM_ADJACENT_FIND_HPP

#include <fermat/range_fwd.h>

#include <fermat/functional/comparisons.h>
#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges {
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(adjacent_find)
        /// \brief function template \c adjacent_find
        ///
        /// range-based version of the \c adjacent_find std algorithm
        ///
        /// \pre `Rng` is a model of the `range` concept
        /// \pre `C` is a model of the `BinaryPredicate` concept
        template(typename I, typename S, typename C = equal_to, typename P = identity)(
            requires forward_iterator<I> AND sentinel_for<S, I> AND
            indirect_relation<C, projected<I, P>>)
        constexpr I RANGES_FUNC(adjacent_find)(I first, S last, C pred = C{}, P proj = P{}) {
            if (first == last)
                return first;
            auto inext = first;
            for (; ++inext != last; first = inext)
                if (invoke(pred, invoke(proj, *first), invoke(proj, *inext)))
                    return first;
            return inext;
        }

        /// \overload
        template(typename Rng, typename C = equal_to, typename P = identity)(
            requires forward_range<Rng> AND
            indirect_relation<C, projected<iterator_t<Rng>, P>>)
        constexpr borrowed_iterator_t<Rng> //
        RANGES_FUNC(adjacent_find)(Rng && rng, C pred = C{}, P proj = P{}) //
        {
            return (*this)(begin(rng), end(rng), std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(adjacent_find)

    namespace cpp20 {
        using fermat::ranges::adjacent_find;
    }

    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif // RANGE_ALGORITHM_ADJACENT_FIND_HPP
