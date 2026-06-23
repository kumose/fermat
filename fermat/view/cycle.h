/// \file cycle.hpp
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//  Copyright Gonzalo Brito Gadeschi 2015
//  Copyright Casey Carter 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_CYCLE_HPP
#define RANGES_V3_VIEW_CYCLE_HPP

#include <type_traits>
#include <utility>

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/iterator/operations.h>
#include <fermat/iterator/unreachable_sentinel.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/primitives.h>
#include <fermat/range/traits.h>
#include <fermat/utility/box.h>
#include <fermat/utility/get.h>
#include <fermat/utility/optional.h>
#include <fermat/utility/static_const.h>
#include <fermat/view/all.h>
#include <fermat/view/facade.h>
#include <fermat/view/view.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges {
    /// \addtogroup group-views
    /// @{
    template<typename Rng, bool /* = (bool) is_infinite<Rng>() */>
    struct RANGES_EMPTY_BASES cycled_view
            : view_facade<cycled_view<Rng>, infinite>
              , private detail::non_propagating_cache<iterator_t<Rng>, cycled_view<Rng>,
                  !common_range<Rng>> {
    private:
        static_assert(static_cast<bool>(forward_range<Rng> && !is_infinite<Rng>::value),
                      "Concept assertion failed : forward_range<Rng> && !is_infinite<Rng>::value");
        friend range_access;
        Rng rng_;

        using cache_t = detail::non_propagating_cache<iterator_t<Rng>, cycled_view<Rng>,
            !common_range<Rng>>;

        template<bool IsConst>
        struct cursor {
        private:
            friend struct cursor<!IsConst>;
            template<typename T>
            using constify_if = meta::const_if_c<IsConst, T>;
            using cycled_view_t = constify_if<cycled_view>;
            using CRng = constify_if<Rng>;
            using iterator = iterator_t<CRng>;

            cycled_view_t *rng_{};
            iterator it_{};
            std::intmax_t n_ = 0;

            iterator get_end_(std::true_type, bool = false) const {
                return fermat::ranges::end(rng_->rng_);
            }

            template<bool CanBeEmpty = false>
            iterator get_end_(std::false_type, meta::bool_<CanBeEmpty> = {}) const {
                auto &end_ = static_cast<cache_t &>(*rng_);
                RANGES_EXPECT(CanBeEmpty || end_);
                if (CanBeEmpty && !end_)
                    end_ = fermat::ranges::next(it_, fermat::ranges::end(rng_->rng_));
                return *end_;
            }

            void set_end_(std::true_type) const {
            }

            void set_end_(std::false_type) const {
                auto &end_ = static_cast<cache_t &>(*rng_);
                if (!end_)
                    end_ = it_;
            }

        public:
            cursor() = default;

            cursor(cycled_view_t *rng)
                : rng_(rng)
                  , it_(fermat::ranges::begin(rng->rng_)) {
            }

            template(bool Other)(
                requires IsConst AND CPP_NOT(Other)) //
            cursor(cursor<Other> that)
                : rng_(that.rng_)
                  , it_(std::move(that.it_)) {
            }

            // clang-format off
            auto CPP_auto_fun(read)()(const)
            (
                return *it_
            )
            // clang-format on
            CPP_member
            auto equal(cursor const &pos) const //
                -> CPP_ret(bool)(
                    requires equality_comparable<iterator>) {
                RANGES_EXPECT(rng_ == pos.rng_);
                return n_ == pos.n_ && it_ == pos.it_;
            }

            void next() {
                auto const last = fermat::ranges::end(rng_->rng_);
                RANGES_EXPECT(it_ != last);
                if (++it_ == last) {
                    ++n_;
                    this->set_end_(meta::bool_<(bool) common_range<CRng>>{});
                    it_ = fermat::ranges::begin(rng_->rng_);
                }
            }

            CPP_member
            auto prev() //
                -> CPP_ret(void)(
                    requires bidirectional_range<CRng>) {
                if (it_ == fermat::ranges::begin(rng_->rng_)) {
                    RANGES_EXPECT(n_ > 0); // decrementing the begin iterator?!
                    --n_;
                    it_ = this->get_end_(meta::bool_<(bool) common_range<CRng>>{});
                }
                --it_;
            }

            template(typename Diff)(
                requires random_access_range<CRng> AND
                detail::integer_like_<Diff>)
            void advance(Diff n) {
                auto const first = fermat::ranges::begin(rng_->rng_);
                auto const last = this->get_end_(meta::bool_<(bool) common_range<CRng>>{},
                                                 meta::bool_<true>());
                auto const dist = last - first;
                auto const d = it_ - first;
                auto const off = (d + n) % dist;
                n_ += (d + n) / dist;
                RANGES_EXPECT(n_ >= 0);
                using D = range_difference_t<Rng>;
                it_ = first + static_cast<D>(off < 0 ? off + dist : off);
            }

            CPP_auto_member
            auto CPP_fun(distance_to)(cursor const & that)(const //
                                                           requires sized_sentinel_for<iterator, iterator>) {
                RANGES_EXPECT(that.rng_ == rng_);
                auto const first = fermat::ranges::begin(rng_->rng_);
                auto const last = this->get_end_(meta::bool_<(bool) common_range<Rng>>{},
                                                 meta::bool_<true>());
                auto const dist = last - first;
                return (that.n_ - n_) * dist + (that.it_ - it_);
            }
        };

        CPP_member
        auto begin_cursor() //
            -> CPP_ret(cursor<false>)(
                requires (!simple_view<Rng>() || !common_range<Rng const>)) {
            return {this};
        }

        CPP_member
        auto begin_cursor() const //
            -> CPP_ret(cursor<true>)(
                requires common_range<Rng const>) {
            return {this};
        }

        unreachable_sentinel_t end_cursor() const {
            return unreachable;
        }

    public:
        cycled_view() = default;

        /// \pre <tt>!empty(rng)</tt>
        explicit cycled_view(Rng rng)
            : rng_(std::move(rng)) {
            RANGES_EXPECT(!fermat::ranges::empty(rng_));
        }
    };

    template<typename Rng>
    struct cycled_view<Rng, true> : identity_adaptor<Rng> {
        static_assert(static_cast<bool>(is_infinite<Rng>::value),
                      "Concept assertion failed : is_infinite<Rng>::value");
        using identity_adaptor<Rng>::identity_adaptor;
    };

    template<typename Rng>
    cycled_view(Rng &&) //
        -> cycled_view<views::all_t<Rng> >;

    namespace views {
        /// Returns an infinite range that endlessly repeats the source
        /// range.
        struct cycle_fn {
            /// \pre <tt>!empty(rng)</tt>
            template(typename Rng)(
                requires viewable_range<Rng> AND forward_range<Rng>)
            cycled_view<all_t<Rng> > operator()(Rng &&rng) const {
                return cycled_view<all_t<Rng> >{all(static_cast<Rng &&>(rng))};
            }
        };

        /// \relates cycle_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(view_closure<cycle_fn>, cycle)
    } // namespace views
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::cycled_view)

#endif
