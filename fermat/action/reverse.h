/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//  Copyright Gonzalo Brito Gadeschi 2017
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_ACTION_REVERSE_HPP
#define RANGES_V3_ACTION_REVERSE_HPP

#include <fermat/range_fwd.h>

#include <fermat/action/action.h>
#include <fermat/algorithm/reverse.h>
#include <fermat/iterator/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-actions
    /// @{
    namespace actions
    {
        /// Reversed the source range in-place.
        struct reverse_fn
        {
            template(typename Rng)(
                requires bidirectional_range<Rng> AND permutable<iterator_t<Rng>>)
            Rng operator()(Rng && rng) const
            {
                fermat::ranges::reverse(rng);
                return static_cast<Rng &&>(rng);
            }
        };

        /// \relates actions::reverse_fn
        /// \sa action_closure
        RANGES_INLINE_VARIABLE(action_closure<reverse_fn>, reverse)
    } // namespace actions
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
