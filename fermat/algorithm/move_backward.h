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
#ifndef RANGES_V3_ALGORITHM_MOVE_BACKWARD_HPP
#define RANGES_V3_ALGORITHM_MOVE_BACKWARD_HPP

#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
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
    template<typename I, typename O>
    using move_backward_result = detail::in_out_result<I, O>;

    RANGES_FUNC_BEGIN(move_backward)

        /// \brief function template \c move_backward
        template(typename I, typename S, typename O)(
            requires bidirectional_iterator<I> AND sentinel_for<S, I> AND
            bidirectional_iterator<O> AND indirectly_movable<I, O>)
        constexpr move_backward_result<I, O> RANGES_FUNC(move_backward)(I first, S end_, O out) //
        {
            I i = fermat::ranges::next(first, end_), last = i;
            while(first != i)
                *--out = iter_move(--i);
            return {last, out};
        }

        /// \overload
        template(typename Rng, typename O)(
            requires bidirectional_range<Rng> AND bidirectional_iterator<O> AND
            indirectly_movable<iterator_t<Rng>, O>)
        constexpr move_backward_result<borrowed_iterator_t<Rng>, O> //
        RANGES_FUNC(move_backward)(Rng && rng, O out)            //
        {
            return (*this)(begin(rng), end(rng), std::move(out));
        }

    RANGES_FUNC_END(move_backward)

    namespace cpp20
    {
        using fermat::ranges::move_backward;
        using fermat::ranges::move_backward_result;
    } // namespace cpp20
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
