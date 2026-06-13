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
#ifndef RANGES_V3_ALGORITHM_REMOVE_IF_HPP
#define RANGES_V3_ALGORITHM_REMOVE_IF_HPP

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/find_if.h>
#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/traits.h>
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
    RANGES_FUNC_BEGIN(remove_if)

        /// \brief function template \c remove_if
        template(typename I, typename S, typename C, typename P = identity)(
            requires permutable<I> AND sentinel_for<S, I> AND
            indirect_unary_predicate<C, projected<I, P>>)
        constexpr I RANGES_FUNC(remove_if)(I first, S last, C pred, P proj = P{})
        {
            first = find_if(std::move(first), last, fermat::ranges::ref(pred), fermat::ranges::ref(proj));
            if(first != last)
            {
                for(I i = next(first); i != last; ++i)
                {
                    if(!(invoke(pred, invoke(proj, *i))))
                    {
                        *first = iter_move(i);
                        ++first;
                    }
                }
            }
            return first;
        }

        /// \overload
        template(typename Rng, typename C, typename P = identity)(
            requires forward_range<Rng> AND permutable<iterator_t<Rng>> AND
            indirect_unary_predicate<C, projected<iterator_t<Rng>, P>>)
        constexpr borrowed_iterator_t<Rng> RANGES_FUNC(remove_if)(Rng && rng, C pred, P proj = P{})
        {
            return (*this)(begin(rng), end(rng), std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(remove_if)

    namespace cpp20
    {
        using fermat::ranges::remove_if;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
