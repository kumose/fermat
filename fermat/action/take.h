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

#ifndef RANGES_V3_ACTION_TAKE_HPP
#define RANGES_V3_ACTION_TAKE_HPP

#include <fermat/range_fwd.h>

#include <fermat/action/action.h>
#include <fermat/action/erase.h>
#include <fermat/functional/bind_back.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-actions
    /// @{
    namespace actions
    {
        struct take_fn
        {
            template(typename Int)(
                requires detail::integer_like_<Int>)
            constexpr auto operator()(Int n) const
            {
                return make_action_closure(bind_back(take_fn{}, n));
            }

            template(typename Rng)(
                requires forward_range<Rng> AND
                    erasable_range<Rng &, iterator_t<Rng>, sentinel_t<Rng>>)
            Rng operator()(Rng && rng, range_difference_t<Rng> n) const
            {
                RANGES_EXPECT(n >= 0);
                fermat::ranges::actions::erase(
                    rng, fermat::ranges::next(begin(rng), n, end(rng)), end(rng));
                return static_cast<Rng &&>(rng);
            }
        };

        /// \relates actions::take_fn
        RANGES_INLINE_VARIABLE(take_fn, take)
    } // namespace actions
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
