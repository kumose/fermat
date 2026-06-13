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

#ifndef RANGES_V3_VIEW_GROUP_BY_HPP
#define RANGES_V3_VIEW_GROUP_BY_HPP

#include <type_traits>
#include <utility>

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/find_if_not.h>
#include <fermat/functional/bind_back.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/default_sentinel.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/optional.h>
#include <fermat/utility/semiregular_box.h>
#include <fermat/utility/static_const.h>
#include <fermat/view/facade.h>
#include <fermat/view/subrange.h>
#include <fermat/view/take_while.h>
#include <fermat/view/view.h>

#include <fermat/detail/config.h>
#include <fermat/detail/prologue.h>

namespace fermat::ranges {
    // TODO group_by could support Input ranges by keeping mutable state in
    // the range itself. The group_by view would then be mutable-only and
    // Input.

    /// \addtogroup group-views
    /// @{
    template<typename Rng, typename Fun>
    struct group_by_view
            : view_facade<group_by_view<Rng, Fun>,
                is_finite<Rng>::value ? finite : range_cardinality<Rng>::value> {
    private:
        friend range_access;
        Rng rng_;
        // cached version of the end of the first subrange / start of the second subrange
        detail::non_propagating_cache<iterator_t<Rng> > second_;
        semiregular_box_t<Fun> fun_;

        struct pred {
            iterator_t<Rng> first_;
            semiregular_box_ref_or_val_t<Fun, false> fun_;

            bool operator()(range_reference_t<Rng> r) const {
                return invoke(fun_, *first_, r);
            }
        };

        struct cursor {
        private:
            friend range_access;
            friend group_by_view;
            iterator_t<Rng> cur_;
            iterator_t<Rng> next_cur_;
            sentinel_t<Rng> last_;
            semiregular_box_ref_or_val_t<Fun, false> fun_;

            struct mixin : basic_mixin<cursor> {
                mixin() = default;
#ifndef _MSC_VER
                using basic_mixin<cursor>::basic_mixin;
#else
                constexpr explicit mixin(cursor &&cur)
                    : basic_mixin<cursor>(static_cast<cursor &&>(cur)) {
                }
                constexpr explicit mixin(cursor const &cur)
                    : basic_mixin<cursor>(cur) {
                }
#endif
                iterator_t<Rng> base() const {
                    return this->get().cur_;
                }
            };

#ifdef _MSC_VER
            template<typename I = iterator_t<Rng> >
            subrange<I> read() const {
                return {cur_, next_cur_};
            }
#else
            subrange<iterator_t<Rng> > read() const {
                return {cur_, next_cur_};
            }
#endif
            void next() {
                cur_ = next_cur_;
                next_cur_ = cur_ != last_
                                ? find_if_not(fermat::ranges::next(cur_), last_, pred{cur_, fun_})
                                : cur_;
            }

            bool equal(default_sentinel_t) const {
                return cur_ == last_;
            }

            bool equal(cursor const &that) const {
                return cur_ == that.cur_;
            }

            cursor(semiregular_box_ref_or_val_t<Fun, false> fun, iterator_t<Rng> first,
                   iterator_t<Rng> next_cur, sentinel_t<Rng> last)
                : cur_(first)
                  , next_cur_(next_cur)
                  , last_(last)
                  , fun_(fun) {
            }

        public:
            cursor() = default;
        };

        cursor begin_cursor() {
            auto b = fermat::ranges::begin(rng_);
            auto e = fermat::ranges::end(rng_);
            if (!second_) {
                second_ = b != e ? find_if_not(fermat::ranges::next(b), e, pred{b, fun_}) : b;
            }
            return {fun_, b, *second_, e};
        }

    public:
        group_by_view() = default;

        constexpr group_by_view(Rng rng, Fun fun)
            : rng_(std::move(rng))
              , fun_(std::move(fun)) {
        }

        Rng base() const {
            return rng_;
        }
    };

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    template(typename Rng, typename Fun)(
        requires copy_constructible<Fun>)
    group_by_view(Rng &&, Fun)
        -> group_by_view<views::all_t<Rng>, Fun>;
#endif

    namespace views {
        struct group_by_base_fn {
            template(typename Rng, typename Fun)(
                requires viewable_range<Rng> AND forward_range<Rng> AND
                indirect_relation<Fun, iterator_t<Rng>>)
            RANGES_DEPRECATED(
                "views::group_by is deprecated. Please use views::chunk_by instead. "
                "Note that views::chunk_by evaluates the predicate between adjacent "
                "elements.")
            constexpr group_by_view<all_t<Rng>, Fun> operator()(Rng &&rng, Fun fun) const {
                return {all(static_cast<Rng &&>(rng)), std::move(fun)};
            }
        };

        struct group_by_fn : group_by_base_fn {
            using group_by_base_fn::operator();

            template<typename Fun>
            RANGES_DEPRECATED(
                "views::group_by is deprecated. Please use views::chunk_by instead. "
                "Note that views::chunk_by evaluates the predicate between adjacent "
                "elements.")
            constexpr auto operator()(Fun fun) const {
                return make_view_closure(bind_back(group_by_base_fn{}, std::move(fun)));
            }
        };

        /// \relates group_by_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(group_by_fn, group_by)
    } // namespace views
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>
#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::group_by_view)

#endif
