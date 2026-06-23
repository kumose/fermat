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

#ifndef RANGES_V3_VIEW_ADDRESSOF_HPP
#define RANGES_V3_VIEW_ADDRESSOF_HPP

#include <type_traits>
#include <utility>

#include <fermat/meta/meta.h>

#include <fermat/utility/addressof.h>
#include <fermat/view/transform.h>
#include <fermat/view/view.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-views
    /// @{
    namespace views
    {
        struct addressof_fn
        {
        private:
            struct take_address
            {
                template<typename V>
                constexpr V * operator()(V & value) const noexcept
                {
                    return detail::addressof(value);
                }
            };

        public:
            template(typename Rng)(
                requires viewable_range<Rng> AND input_range<Rng> AND
                    std::is_lvalue_reference<range_reference_t<Rng>>::value) //
            constexpr auto CPP_auto_fun(operator())(Rng && rng)(const) //
            (
                return transform(all(static_cast<Rng &&>(rng)), take_address{}) //
            )
        };

        /// \relates addressof_fn
        /// \ingroup group-views
        RANGES_INLINE_VARIABLE(view_closure<addressof_fn>, addressof)
    } // namespace views
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif // RANGES_V3_VIEW_ADDRESSOF_HPP
