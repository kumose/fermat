/// \file
// Range v3 library
//
//  Copyright Andrey Diduh 2019
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_ACTION_REMOVE_HPP
#define RANGES_V3_ACTION_REMOVE_HPP

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/action/action.h>
#include <fermat/action/erase.h>
#include <fermat/algorithm/remove_test.h>
#include <fermat/functional/bind_back.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-actions
    /// @{
    namespace actions
    {
        struct remove_fn
        {
            template(typename V, typename P)(
                requires (!range<V>))
            constexpr auto operator()(V && value, P proj) const
            {
                return make_action_closure(
                    bind_back(remove_fn{}, static_cast<V &&>(value), std::move(proj)));
            }

            template<typename V>
            constexpr auto operator()(V && value) const
            {
                return make_action_closure(
                    bind_back(remove_fn{}, static_cast<V &&>(value), identity{}));
            }

            template(typename Rng, typename V, typename P = identity)(
                requires forward_range<Rng> AND permutable<iterator_t<Rng>> AND
                        erasable_range<Rng, iterator_t<Rng>, sentinel_t<Rng>> AND
                            indirect_relation<equal_to, projected<iterator_t<Rng>, P>,
                                              V const *>)
            Rng operator()(Rng && rng, V const & value, P proj = {}) const
            {
                auto it = fermat::ranges::remove(rng, value, std::move(proj));
                fermat::ranges::erase(rng, it, fermat::ranges::end(rng));
                return static_cast<Rng &&>(rng);
            }
        };

        /// \relates actions::remove_fn
        RANGES_INLINE_VARIABLE(remove_fn, remove)
    } // namespace actions
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
