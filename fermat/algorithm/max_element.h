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
#ifndef RANGES_V3_ALGORITHM_MAX_ELEMENT_HPP
#define RANGES_V3_ALGORITHM_MAX_ELEMENT_HPP

#include <fermat/range_fwd.h>

#include <fermat/functional/comparisons.h>
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

namespace ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(max_element)

        /// \brief function template \c max_element
        template(typename I, typename S, typename C = less, typename P = identity)(
            requires forward_iterator<I> AND sentinel_for<S, I> AND
            indirect_strict_weak_order<C, projected<I, P>>)
        constexpr I RANGES_FUNC(max_element)(I first, S last, C pred = C{}, P proj = P{})
        {
            if(first != last)
                for(auto tmp = next(first); tmp != last; ++tmp)
                    if(invoke(pred, invoke(proj, *first), invoke(proj, *tmp)))
                        first = tmp;
            return first;
        }

        /// \overload
        template(typename Rng, typename C = less, typename P = identity)(
            requires forward_range<Rng> AND
            indirect_strict_weak_order<C, projected<iterator_t<Rng>, P>>)
        constexpr borrowed_iterator_t<Rng> //
        RANGES_FUNC(max_element)(Rng && rng, C pred = C{}, P proj = P{})
        {
            return (*this)(begin(rng), end(rng), std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(max_element)

    namespace cpp20
    {
        using ranges::max_element;
    }
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
