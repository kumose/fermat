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
#ifndef RANGES_V3_ALGORITHM_PARTIAL_SORT_HPP
#define RANGES_V3_ALGORITHM_PARTIAL_SORT_HPP

#include <functional>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/heap_algorithm.h>
#include <fermat/functional/comparisons.h>
#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/concepts.h>
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
    RANGES_FUNC_BEGIN(partial_sort)

        /// \brief function template \c partial_sort
        template(typename I, typename S, typename C = less, typename P = identity)(
            requires sortable<I, C, P> AND random_access_iterator<I> AND
                sentinel_for<S, I>)
        constexpr I RANGES_FUNC(partial_sort)(
            I first, I middle, S last, C pred = C{}, P proj = P{}) //
        {
            make_heap(first, middle, fermat::ranges::ref(pred), fermat::ranges::ref(proj));
            auto const len = middle - first;
            I i = middle;
            for(; i != last; ++i)
            {
                if(invoke(pred, invoke(proj, *i), invoke(proj, *first)))
                {
                    iter_swap(i, first);
                    detail::sift_down_n(
                        first, len, first, fermat::ranges::ref(pred), fermat::ranges::ref(proj));
                }
            }
            sort_heap(first, middle, fermat::ranges::ref(pred), fermat::ranges::ref(proj));
            return i;
        }

        /// \overload
        template(typename Rng, typename C = less, typename P = identity)(
            requires sortable<iterator_t<Rng>, C, P> AND random_access_range<Rng>)
        constexpr borrowed_iterator_t<Rng> RANGES_FUNC(partial_sort)(
            Rng && rng, iterator_t<Rng> middle, C pred = C{}, P proj = P{}) //
        {
            return (*this)(begin(rng),
                           std::move(middle),
                           end(rng),
                           std::move(pred),
                           std::move(proj));
        }

    RANGES_FUNC_END(partial_sort)

    namespace cpp20
    {
        using fermat::ranges::partial_sort;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
