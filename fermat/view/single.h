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

#ifndef RANGES_V3_VIEW_SINGLE_HPP
#define RANGES_V3_VIEW_SINGLE_HPP

#include <type_traits>
#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/iterator/concepts.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/addressof.h>
#include <fermat/utility/optional.h>
#include <fermat/utility/semiregular_box.h>
#include <fermat/utility/static_const.h>
#include <fermat/view/facade.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-views
    /// @{
    template<typename T>
    struct single_view : view_interface<single_view<T>, (cardinality)1>
    {
    private:
        CPP_assert(copy_constructible<T>);
        static_assert(std::is_object<T>::value,
                      "The template parameter of single_view must be an object type");
        semiregular_box_t<T> value_;
        template<typename... Args>
        constexpr single_view(in_place_t, std::true_type, Args &&... args)
          : value_{static_cast<Args &&>(args)...}
        {}
        template<typename... Args>
        constexpr single_view(in_place_t, std::false_type, Args &&... args)
          : value_{in_place, static_cast<Args &&>(args)...}
        {}

    public:
        single_view() = default;
        constexpr explicit single_view(T const & t)
          : value_(t)
        {}
        constexpr explicit single_view(T && t)
          : value_(std::move(t))
        {}
        template(class... Args)(
            requires constructible_from<T, Args...>)
            constexpr single_view(in_place_t, Args &&... args)
          : single_view{in_place,
                        meta::bool_<(bool)semiregular<T>>{},
                        static_cast<Args &&>(args)...}
        {}
        constexpr T * begin() noexcept
        {
            return data();
        }
        constexpr T const * begin() const noexcept
        {
            return data();
        }
        constexpr T * end() noexcept
        {
            return data() + 1;
        }
        constexpr T const * end() const noexcept
        {
            return data() + 1;
        }
        static constexpr std::size_t size() noexcept
        {
            return 1u;
        }
        constexpr T * data() noexcept
        {
            return detail::addressof(static_cast<T &>(value_));
        }
        constexpr T const * data() const noexcept
        {
            return detail::addressof(static_cast<T const &>(value_));
        }
    };

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
    template<class T>
    explicit single_view(T &&) //
        -> single_view<detail::decay_t<T>>;
#endif

    namespace views
    {
        struct single_fn
        {
            template(typename Val)(
                requires copy_constructible<Val>)
            single_view<Val> operator()(Val value) const
            {
                return single_view<Val>{std::move(value)};
            }
        };

        /// \relates single_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(single_fn, single)
    } // namespace views

    namespace cpp20
    {
        namespace views
        {
            using fermat::ranges::views::single;
        }
        template(typename T)(
            requires std::is_object<T>::value) //
            using single_view = fermat::ranges::single_view<T>;
    } // namespace cpp20
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>
#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::single_view)

#endif
