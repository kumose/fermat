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
#ifndef RANGES_V3_ALGORITHM_FIND_IF_NOT_HPP
#define RANGES_V3_ALGORITHM_FIND_IF_NOT_HPP

#include <utility>

#include <fermat/range_fwd.h>

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

namespace ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(find_if_not)
        /// \brief template function \c find_if_not
        ///
        /// range-based version of the \c find_if_not std algorithm
        ///
        /// \pre `Rng` is a model of the `range` concept
        /// \pre `I` is a model of the `input_iterator` concept
        /// \pre `S` is a model of the `sentinel_for<I>` concept
        /// \pre `P` is a model of the `invocable<V>` concept, where `V` is the
        ///      value type of I.
        /// \pre `F` models `predicate<X>`, where `X` is the result type
        ///      of `invocable<P, V>`
        template(typename I, typename S, typename F, typename P = identity)(
            requires input_iterator<I> AND sentinel_for<S, I> AND
            indirect_unary_predicate<F, projected<I, P>>)
        constexpr I RANGES_FUNC(find_if_not)(I first, S last, F pred, P proj = P{})
        {
            for(; first != last; ++first)
                if(!invoke(pred, invoke(proj, *first)))
                    break;
            return first;
        }

        /// \overload
        template(typename Rng, typename F, typename P = identity)(
            requires input_range<Rng> AND
            indirect_unary_predicate<F, projected<iterator_t<Rng>, P>>)
        constexpr borrowed_iterator_t<Rng> //
        RANGES_FUNC(find_if_not)(Rng && rng, F pred, P proj = P{})
        {
            return (*this)(begin(rng), end(rng), std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(find_if_not)

    namespace cpp20
    {
        using ranges::find_if_not;
    }
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
