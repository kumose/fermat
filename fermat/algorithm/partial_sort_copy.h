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
#ifndef RANGES_V3_ALGORITHM_PARTIAL_SORT_COPY_HPP
#define RANGES_V3_ALGORITHM_PARTIAL_SORT_COPY_HPP

#include <fermat/meta/meta.h>

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
    RANGES_FUNC_BEGIN(partial_sort_copy)

        /// \brief function template \c partial_sort_copy
        template(typename I,
                 typename SI,
                 typename O,
                 typename SO,
                 typename C = less,
                 typename PI = identity,
                 typename PO = identity)(
            requires input_iterator<I> AND sentinel_for<SI, I> AND
                random_access_iterator<O> AND sentinel_for<SO, O> AND
                indirectly_copyable<I, O> AND sortable<O, C, PO> AND
                indirect_strict_weak_order<C, projected<I, PI>, projected<O, PO>>)
        constexpr O RANGES_FUNC(partial_sort_copy)(I first,
                                                   SI last,
                                                   O out_begin,
                                                   SO out_end,
                                                   C pred = C{},
                                                   PI in_proj = PI{},
                                                   PO out_proj = PO{}) //
        {
            O r = out_begin;
            if(r != out_end)
            {
                for(; first != last && r != out_end; ++first, ++r)
                    *r = *first;
                make_heap(out_begin, r, fermat::ranges::ref(pred), fermat::ranges::ref(out_proj));
                auto len = r - out_begin;
                for(; first != last; ++first)
                {
                    auto && x = *first;
                    if(invoke(pred, invoke(in_proj, x), invoke(out_proj, *out_begin)))
                    {
                        *out_begin = (decltype(x) &&)x;
                        detail::sift_down_n(out_begin,
                                            len,
                                            out_begin,
                                            fermat::ranges::ref(pred),
                                            fermat::ranges::ref(out_proj));
                    }
                }
                sort_heap(out_begin, r, fermat::ranges::ref(pred), fermat::ranges::ref(out_proj));
            }
            return r;
        }

        /// \overload
        template(typename InRng,
                 typename OutRng,
                 typename C = less,
                 typename PI = identity,
                 typename PO = identity)(
            requires input_range<InRng> AND random_access_range<OutRng> AND
                indirectly_copyable<iterator_t<InRng>, iterator_t<OutRng>> AND
                sortable<iterator_t<OutRng>, C, PO> AND
                indirect_strict_weak_order<C,
                                           projected<iterator_t<InRng>, PI>,
                                           projected<iterator_t<OutRng>, PO>>)
        constexpr borrowed_iterator_t<OutRng> RANGES_FUNC(partial_sort_copy)(InRng && in_rng,
                                                                             OutRng && out_rng,
                                                                             C pred = C{},
                                                                             PI in_proj = PI{},
                                                                             PO out_proj = PO{}) //
        {
            return (*this)(begin(in_rng),
                           end(in_rng),
                           begin(out_rng),
                           end(out_rng),
                           std::move(pred),
                           std::move(in_proj),
                           std::move(out_proj));
        }

    RANGES_FUNC_END(partial_sort_copy)

    namespace cpp20
    {
        using fermat::ranges::partial_sort_copy;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
