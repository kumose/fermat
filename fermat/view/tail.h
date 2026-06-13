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

#ifndef RANGES_V3_VIEW_TAIL_HPP
#define RANGES_V3_VIEW_TAIL_HPP

#include <type_traits>
#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/iterator/operations.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/primitives.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>
#include <fermat/view/all.h>
#include <fermat/view/interface.h>
#include <fermat/view/view.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    namespace detail
    {
        template<typename T>
        constexpr T prev_or_zero_(T n)
        {
            return n == 0 ? T(0) : T(n - 1);
        }
    } // namespace detail

    /// \addtogroup group-views
    /// @{
    template<typename Rng>
    struct tail_view
      : view_interface<tail_view<Rng>,
                       (range_cardinality<Rng>::value >= 0)
                           ? detail::prev_or_zero_(range_cardinality<Rng>::value)
                           : range_cardinality<Rng>::value>
    {
    private:
        Rng rng_;

    public:
        tail_view() = default;
        tail_view(Rng rng)
          : rng_(static_cast<Rng &&>(rng))
        {
            CPP_assert(input_range<Rng>);
        }
        iterator_t<Rng> begin()
        {
            return next(fermat::ranges::begin(rng_), 1, fermat::ranges::end(rng_));
        }
        template(bool Const = true)(
            requires Const AND range<meta::const_if_c<Const, Rng>>)
        iterator_t<meta::const_if_c<Const, Rng>> begin() const
        {
            return next(fermat::ranges::begin(rng_), 1, fermat::ranges::end(rng_));
        }
        sentinel_t<Rng> end()
        {
            return fermat::ranges::end(rng_);
        }
        template(bool Const = true)(
            requires Const AND range<meta::const_if_c<Const, Rng>>)
        sentinel_t<meta::const_if_c<Const, Rng>> end() const
        {
            return fermat::ranges::end(rng_);
        }
        // Strange cast to bool in the requires clause is to work around gcc bug.
        CPP_auto_member
        constexpr auto CPP_fun(size)()(
            requires(bool(sized_range<Rng>)))
        {
            using size_type = range_size_t<Rng>;
            return range_cardinality<Rng>::value >= 0
                       ? detail::prev_or_zero_((size_type)range_cardinality<Rng>::value)
                       : detail::prev_or_zero_(fermat::ranges::size(rng_));
        }
        CPP_auto_member
        constexpr auto CPP_fun(size)()(const //
            requires(bool(sized_range<Rng const>)))
        {
            using size_type = range_size_t<Rng>;
            return range_cardinality<Rng>::value >= 0
                       ? detail::prev_or_zero_((size_type)range_cardinality<Rng>::value)
                       : detail::prev_or_zero_(fermat::ranges::size(rng_));
        }
        Rng base() const
        {
            return rng_;
        }
    };

    template<typename Rng>
    inline constexpr bool enable_borrowed_range<tail_view<Rng>> = //
        enable_borrowed_range<Rng>;

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    template(typename Rng)(
        requires viewable_range<Rng>)
        tail_view(Rng &&)
            ->tail_view<views::all_t<Rng>>;
#endif

    namespace views
    {
        struct tail_fn
        {
            template(typename Rng)(
                requires viewable_range<Rng> AND input_range<Rng>)
            meta::if_c<range_cardinality<Rng>::value == 0,
                       all_t<Rng>,
                       tail_view<all_t<Rng>>> //
            operator()(Rng && rng) const
            {
                return all(static_cast<Rng &&>(rng));
            }
        };

        /// \relates tail_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(view_closure<tail_fn>, tail)
    } // namespace views
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>
#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::tail_view)

#endif
