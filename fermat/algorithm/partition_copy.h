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
#ifndef RANGES_V3_ALGORITHM_PARTITION_COPY_HPP
#define RANGES_V3_ALGORITHM_PARTITION_COPY_HPP

#include <tuple>

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
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
    template<typename I, typename O0, typename O1>
    using partition_copy_result = detail::in_out1_out2_result<I, O0, O1>;

    RANGES_FUNC_BEGIN(partition_copy)

        /// \brief function template \c partition_copy
        template(typename I,
                 typename S,
                 typename O0,
                 typename O1,
                 typename C,
                 typename P = identity)(
            requires input_iterator<I> AND sentinel_for<S, I> AND
                weakly_incrementable<O0> AND weakly_incrementable<O1> AND
                indirectly_copyable<I, O0> AND indirectly_copyable<I, O1> AND
                indirect_unary_predicate<C, projected<I, P>>)
        constexpr partition_copy_result<I, O0, O1> RANGES_FUNC(partition_copy)(
            I first, S last, O0 o0, O1 o1, C pred, P proj = P{})
        {
            for(; first != last; ++first)
            {
                auto && x = *first;
                if(invoke(pred, invoke(proj, x)))
                {
                    *o0 = (decltype(x) &&)x;
                    ++o0;
                }
                else
                {
                    *o1 = (decltype(x) &&)x;
                    ++o1;
                }
            }
            return {first, o0, o1};
        }

        /// \overload
        template(typename Rng,
                 typename O0,
                 typename O1,
                 typename C,
                 typename P = identity)(
            requires input_range<Rng> AND weakly_incrementable<O0> AND
                weakly_incrementable<O1> AND indirectly_copyable<iterator_t<Rng>, O0> AND
                indirectly_copyable<iterator_t<Rng>, O1> AND
                indirect_unary_predicate<C, projected<iterator_t<Rng>, P>>)
        constexpr partition_copy_result<borrowed_iterator_t<Rng>, O0, O1> //
        RANGES_FUNC(partition_copy)(Rng && rng, O0 o0, O1 o1, C pred, P proj = P{})
        {
            return (*this)(begin(rng),
                           end(rng),
                           std::move(o0),
                           std::move(o1),
                           std::move(pred),
                           std::move(proj));
        }

    RANGES_FUNC_END(partition_copy)

    namespace cpp20
    {
        using ranges::partition_copy;
        using ranges::partition_copy_result;
    } // namespace cpp20
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
