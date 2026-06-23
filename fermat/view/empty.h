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

#ifndef RANGES_V3_VIEW_EMPTY_HPP
#define RANGES_V3_VIEW_EMPTY_HPP

#include <fermat/range_fwd.h>

#include <fermat/view/interface.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-views
    /// @{
    template<typename T>
    struct empty_view : view_interface<empty_view<T>, (cardinality)0>
    {
        static_assert(std::is_object<T>::value,
                      "The template parameter to empty_view must be an object type.");
        empty_view() = default;
        static constexpr T * begin() noexcept
        {
            return nullptr;
        }
        static constexpr T * end() noexcept
        {
            return nullptr;
        }
        static constexpr std::size_t size() noexcept
        {
            return 0u;
        }
        static constexpr T * data() noexcept
        {
            return nullptr;
        }
        RANGES_DEPRECATED(
            "Replace views::empty<T>() with views::empty<T>. "
            "It is now a variable template.")
        empty_view operator()() const
        {
            return *this;
        }
    };

    template<typename T>
    inline constexpr bool enable_borrowed_range<empty_view<T>> = true;

    namespace views
    {
        template<typename T>
        inline constexpr empty_view<T> empty{};
    }

    namespace cpp20
    {
        namespace views
        {
            using fermat::ranges::views::empty;
        }
        template(typename T)(
            requires std::is_object<T>::value) //
            using empty_view = fermat::ranges::empty_view<T>;
    } // namespace cpp20

    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>
#include <fermat/detail/satisfy_boost_range.h>
RANGES_SATISFY_BOOST_RANGE(::fermat::ranges::empty_view)

#endif
