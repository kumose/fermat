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

#ifndef RANGES_V3_ACTION_ERASE_HPP
#define RANGES_V3_ACTION_ERASE_HPP

#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/action/insert.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \cond
    namespace adl_erase_detail
    {
        template(typename Cont, typename I, typename S)(
            requires lvalue_container_like<Cont> AND forward_iterator<I> AND
                sentinel_for<S, I>)
        auto erase(Cont && cont, I first, S last)                            //
            -> decltype(unwrap_reference(cont).erase(first, last))
        {
            return unwrap_reference(cont).erase(first, last);
        }

        struct erase_fn
        {
            template(typename Rng, typename I, typename S)(
                requires range<Rng> AND forward_iterator<I> AND sentinel_for<S, I>)
            auto operator()(Rng && rng, I first, S last) const
                -> decltype(erase((Rng &&) rng, first, last))
            {
                return erase(static_cast<Rng &&>(rng), first, last);
            }
        };
    } // namespace adl_erase_detail
    /// \endcond

    /// \ingroup group-actions
    RANGES_INLINE_VARIABLE(adl_erase_detail::erase_fn, erase)

    namespace actions
    {
        using fermat::ranges::erase;
    }

    /// \addtogroup group-range
    /// @{
    // clang-format off
    /// \concept erasable_range_
    /// \brief The \c erasable_range_ concept
    template<typename Rng, typename I, typename S>
    CPP_requires(erasable_range_,
        requires(Rng && rng, I first, S last)
        (
            fermat::ranges::erase((Rng &&) rng, first, last)
        )
    );
    /// \concept erasable_range
    /// \brief The \c erasable_range concept
    template<typename Rng, typename I, typename S>
    CPP_concept erasable_range =
        range<Rng> && CPP_requires_ref(fermat::ranges::erasable_range_, Rng, I, S);
    // clang-format on
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
