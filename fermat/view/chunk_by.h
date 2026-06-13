/// \file
// Range v3 library
//
//  Copyright Hui Xie 2021
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_CHUNK_BY_HPP
#define RANGES_V3_VIEW_CHUNK_BY_HPP

#include <type_traits>
#include <utility>

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/adjacent_find.h>
#include <fermat/functional/bind_back.h>
#include <fermat/functional/not_fn.h>
#include <fermat/iterator/default_sentinel.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/optional.h>
#include <fermat/utility/semiregular_box.h>
#include <fermat/utility/static_const.h>
#include <fermat/view/all.h>
#include <fermat/view/facade.h>
#include <fermat/view/subrange.h>
#include <fermat/view/view.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{

    /// \addtogroup group-views
    /// @{
    template<typename Rng, typename Fun>
    struct chunk_by_view
      : view_facade<chunk_by_view<Rng, Fun>,
                    is_finite<Rng>::value ? finite : range_cardinality<Rng>::value>
    {
    private:
        friend range_access;
        Rng rng_;
        // cached version of the end of the first subrange / start of the second subrange
        detail::non_propagating_cache<iterator_t<Rng>> second_;
        semiregular_box_t<Fun> fun_;

        struct cursor
        {
        private:
            friend range_access;
            friend chunk_by_view;
            iterator_t<Rng> cur_;
            iterator_t<Rng> next_cur_;
            sentinel_t<Rng> last_;
            semiregular_box_ref_or_val_t<Fun, false> fun_;

#ifdef _MSC_VER
            template<typename I = iterator_t<Rng>>
            subrange<I> read() const
            {
                return {cur_, next_cur_};
            }
#else
            subrange<iterator_t<Rng>> read() const
            {
                return {cur_, next_cur_};
            }
#endif
            void next()
            {
                cur_ = next_cur_;
                auto partition_cur = adjacent_find(cur_, last_, not_fn(fun_));
                next_cur_ =
                    partition_cur != last_ ? fermat::ranges::next(partition_cur) : partition_cur;
            }

            bool equal(default_sentinel_t) const
            {
                return cur_ == last_;
            }
            bool equal(cursor const & that) const
            {
                return cur_ == that.cur_;
            }
            cursor(semiregular_box_ref_or_val_t<Fun, false> fun, iterator_t<Rng> first,
                   iterator_t<Rng> next_cur, sentinel_t<Rng> last)
              : cur_(first)
              , next_cur_(next_cur)
              , last_(last)
              , fun_(fun)
            {}

        public:
            cursor() = default;
        };
        cursor begin_cursor()
        {
            auto first = fermat::ranges::begin(rng_);
            auto last = fermat::ranges::end(rng_);
            if(!second_)
            {
                auto partition_cur = adjacent_find(first, last, not_fn(fun_));
                second_ =
                    partition_cur != last ? fermat::ranges::next(partition_cur) : partition_cur;
            }
            return {fun_, first, *second_, last};
        }

    public:
        chunk_by_view() = default;
        constexpr chunk_by_view(Rng rng, Fun fun)
          : rng_(std::move(rng))
          , fun_(std::move(fun))
        {}
    };

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    template(typename Rng, typename Fun)(
        requires copy_constructible<Fun>) chunk_by_view(Rng &&, Fun)
        ->chunk_by_view<views::all_t<Rng>, Fun>;
#endif

    namespace views
    {
        struct chunk_by_base_fn
        {
            template(typename Rng, typename Fun)(
                requires viewable_range<Rng> AND forward_range<Rng> AND //
                    indirect_relation<Fun, iterator_t<Rng>>)            //
            constexpr chunk_by_view<all_t<Rng>, Fun>
            operator()(Rng && rng, Fun fun) const
            {
                return {all(static_cast<Rng &&>(rng)), std::move(fun)};
            }
        };

        struct chunk_by_fn : chunk_by_base_fn
        {
            using chunk_by_base_fn::operator();

            template<typename Fun>
            constexpr auto operator()(Fun fun) const
            {
                return make_view_closure(bind_back(chunk_by_base_fn{}, std::move(fun)));
            }
        };

        /// \relates chunk_by_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(chunk_by_fn, chunk_by)
    } // namespace views
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>
#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::chunk_by_view)

#endif
