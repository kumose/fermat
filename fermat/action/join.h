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

#ifndef RANGES_V3_ACTION_JOIN_HPP
#define RANGES_V3_ACTION_JOIN_HPP

#include <vector>

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/action/action.h>
#include <fermat/action/concepts.h>
#include <fermat/action/push_back.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-actions
    /// @{
    namespace actions
    {
        template<typename Rng>
        using join_action_value_t_ =
            meta::if_c<(bool)fermat::ranges::container<range_value_t<Rng>>, //
                       range_value_t<Rng>,                          //
                       std::vector<range_value_t<range_value_t<Rng>>>>;

        struct join_fn
        {
            template(typename Rng)(
                requires input_range<Rng> AND input_range<range_value_t<Rng>> AND
                    semiregular<join_action_value_t_<Rng>>)
            join_action_value_t_<Rng> operator()(Rng && rng) const
            {
                join_action_value_t_<Rng> ret;
                auto last = fermat::ranges::end(rng);
                for(auto it = begin(rng); it != last; ++it)
                    push_back(ret, *it);
                return ret;
            }
        };

        /// \relates actions::join_fn
        /// \sa action_closure
        RANGES_INLINE_VARIABLE(action_closure<join_fn>, join)
    } // namespace actions
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
