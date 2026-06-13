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

#ifndef RANGES_V3_VIEW_REVERSE_HPP
#define RANGES_V3_VIEW_REVERSE_HPP

#include <iterator>
#include <utility>

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/iterator/operations.h>
#include <fermat/iterator/reverse_iterator.h>
#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/traits.h>
#include <fermat/utility/box.h>
#include <fermat/utility/get.h>
#include <fermat/utility/optional.h>
#include <fermat/utility/static_const.h>
#include <fermat/view/adaptor.h>
#include <fermat/view/all.h>
#include <fermat/view/view.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges {
    /// \addtogroup group-views
    /// @{
    template<typename Rng>
    struct RANGES_EMPTY_BASES reverse_view
            : view_interface<reverse_view<Rng>, range_cardinality<Rng>::value>
              , private detail::non_propagating_cache<iterator_t<Rng>, reverse_view<Rng>,
                  !common_range<Rng>> {
    private:
        static_assert(static_cast<bool>(bidirectional_range<Rng>),
                      "Concept assertion failed : bidirectional_range<Rng>");
        Rng rng_;

        constexpr reverse_iterator<iterator_t<Rng> > begin_(std::true_type) {
            return make_reverse_iterator(fermat::ranges::end(rng_));
        }

        constexpr reverse_iterator<iterator_t<Rng> > begin_(std::false_type) {
            using cache_t =
                    detail::non_propagating_cache<iterator_t<Rng>, reverse_view<Rng> >;
            auto &end_ = static_cast<cache_t &>(*this);
            if (!end_) {
#if defined(_MSC_VER)
                auto tmp = fermat::ranges::begin(rng_);
                auto e = fermat::ranges::end(rng_);
                while (tmp != e)
                    ++tmp;
#else
                auto tmp = fermat::ranges::next(fermat::ranges::begin(rng_), fermat::ranges::end(rng_));
#endif
                end_ = std::move(tmp);
            }
            return make_reverse_iterator(*end_);
        }

    public:
        reverse_view() = default;

        constexpr explicit reverse_view(Rng rng)
            : rng_(detail::move(rng)) {
        }

        Rng base() const {
            return rng_;
        }

        constexpr reverse_iterator<iterator_t<Rng> > begin() {
            return begin_(meta::bool_<(bool) common_range<Rng>>{});
        }

        template(bool Const = true)(
            requires Const AND common_range<meta::const_if_c<Const, Rng>>)
        constexpr reverse_iterator<iterator_t<meta::const_if_c<Const, Rng> > > begin() const {
            return make_reverse_iterator(fermat::ranges::end(rng_));
        }

        constexpr reverse_iterator<iterator_t<Rng> > end() {
            return make_reverse_iterator(fermat::ranges::begin(rng_));
        }

        template(bool Const = true)(
            requires Const AND common_range<meta::const_if_c<Const, Rng>>)
        constexpr reverse_iterator<iterator_t<meta::const_if_c<Const, Rng> > > end() const {
            return make_reverse_iterator(fermat::ranges::begin(rng_));
        }

        CPP_auto_member
        constexpr auto CPP_fun(size)()(
            requires sized_range<Rng>) {
            return fermat::ranges::size(rng_);
        }

        CPP_auto_member
        constexpr auto CPP_fun(size)()(const //
            requires sized_range<Rng const>) {
            return fermat::ranges::size(rng_);
        }
    };

    template<typename Rng>
    struct reverse_view<reverse_view<Rng> > : Rng {
        static_assert(static_cast<bool>(bidirectional_range<Rng>),
                      "Concept assertion failed : bidirectional_range<Rng>");
        static_assert(static_cast<bool>(
                          same_as<detail::decay_t<decltype(std::declval<reverse_view<Rng> >().base())>,
                              Rng>),
                      "Concept assertion failed : same_as<detail::decay_t<decltype(std::declval<reverse_view<Rng>>().base())>, Rng>")
        ;

        reverse_view() = default;

        constexpr explicit reverse_view(reverse_view<Rng> rng)
            : Rng(rng.base()) {
        }

        constexpr reverse_view<Rng> base() const {
            return reverse_view<Rng>{*this};
        }
    };

    template<typename Rng>
    inline constexpr bool enable_borrowed_range<reverse_view<Rng> > =
            enable_borrowed_range<Rng>;

    template<typename Rng>
    reverse_view(Rng &&) //
        -> reverse_view<views::all_t<Rng> >;

    template<typename Rng>
    reverse_view(reverse_view<Rng>)
        -> reverse_view<reverse_view<Rng> >;

    namespace views {
        struct reverse_fn {
            template(typename Rng)(
                requires viewable_range<Rng> AND bidirectional_range<Rng>)
            constexpr reverse_view<all_t<Rng> > operator()(Rng &&rng) const {
                return reverse_view<all_t<Rng> >{all(static_cast<Rng &&>(rng))};
            }
        };

        /// \relates reverse_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(view_closure<reverse_fn>, reverse)
    } // namespace views

    namespace cpp20 {
        namespace views {
            using fermat::ranges::views::reverse;
        }

        template(typename Rng)(
            requires view_<Rng> AND bidirectional_range<Rng>)
        using reverse_view = fermat::ranges::reverse_view<Rng>;
    } // namespace cpp20
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>
#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::reverse_view)

#endif
