/// \file
// Range v3 library
//
//  Copyright Andrew Sutton 2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_ALGORITHM_ALL_OF_HPP
#define RANGES_V3_ALGORITHM_ALL_OF_HPP

#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(all_of)

        /// \brief function template \c all_of
        template(typename I, typename S, typename F, typename P = identity)(
            requires input_iterator<I> AND sentinel_for<S, I> AND
            indirect_unary_predicate<F, projected<I, P>>)
        constexpr bool RANGES_FUNC(all_of)(I first, S last, F pred, P proj = P{}) //
        {
            for(; first != last; ++first)
                if(!invoke(pred, invoke(proj, *first)))
                    break;
            return first == last;
        }

        /// \overload
        template(typename Rng, typename F, typename P = identity)(
            requires input_range<Rng> AND
            indirect_unary_predicate<F, projected<iterator_t<Rng>, P>>)
        constexpr bool RANGES_FUNC(all_of)(Rng && rng, F pred, P proj = P{}) //
        {
            return (*this)(begin(rng), end(rng), std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(all_of)

    namespace cpp20
    {
        using fermat::ranges::all_of;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
