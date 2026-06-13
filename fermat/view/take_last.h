/// \file
// Range v3 library
//
//  Copyright Barry Revzin 2019-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_TAKE_LAST_HPP
#define RANGES_V3_VIEW_TAKE_LAST_HPP

#include <fermat/concepts/concepts.hpp>

#include <fermat/range_fwd.h>

#include <fermat/functional/bind_back.h>
#include <fermat/range/concepts.h>
#include <fermat/range/operations.h>
#include <fermat/view/drop_exactly.h>

#include <fermat/detail/prologue.h>

namespace ranges
{
    /// \addtogroup group-views
    /// @{

    namespace views
    {
        struct take_last_base_fn
        {
            template(typename Rng)(
                requires viewable_range<Rng> AND sized_range<Rng>)
            auto operator()(Rng && rng, range_difference_t<Rng> n) const
            {
                auto sz = ranges::distance(rng);
                return drop_exactly(static_cast<Rng &&>(rng), sz > n ? sz - n : 0);
            }
        };

        struct take_last_fn : take_last_base_fn
        {
            using take_last_base_fn::operator();

            template(typename Int)(
                requires detail::integer_like_<Int>)
            constexpr auto operator()(Int n) const
            {
                return make_view_closure(bind_back(take_last_base_fn{}, n));
            }
        };

        /// \relates take_last_fn
        RANGES_INLINE_VARIABLE(take_last_fn, take_last)
    } // namespace views
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
