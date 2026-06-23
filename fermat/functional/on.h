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
#ifndef RANGES_V3_FUNCTIONAL_ON_HPP
#define RANGES_V3_FUNCTIONAL_ON_HPP

#include <fermat/concepts/concepts.hpp>

#include <fermat/detail/config.h>
#include <fermat/functional/invoke.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-functional
    /// @{
    template<typename Fn1, typename Fn2>
    struct transformed
    {
    private:
        RANGES_NO_UNIQUE_ADDRESS
        Fn1 first_;
        RANGES_NO_UNIQUE_ADDRESS
        Fn2 second_;

    public:
        transformed() = default;
        constexpr transformed(Fn1 fn1, Fn2 fn2)
          : first_(static_cast<Fn1 &&>(fn1))
          , second_(static_cast<Fn2 &&>(fn2))
        {}
        // clang-format off
        template<typename... Args>
        auto CPP_auto_fun(operator())(Args &&... args)
        (
            return invoke(first_, invoke(second_, static_cast<Args &&>(args)...))
        )
        template<typename... Args>
        auto CPP_auto_fun(operator())(Args &&... args)(const)
        (
            return invoke((Fn1 const &)first_,
                          invoke((Fn2 const &)second_, static_cast<Args &&>(args))...)
        )
        // clang-format on
    };

    struct on_fn
    {
        template<typename Fn1, typename Fn2>
        constexpr transformed<Fn1, Fn2> operator()(Fn1 fn1, Fn2 fn2) const
        {
            return transformed<Fn1, Fn2>{detail::move(fn1), detail::move(fn2)};
        }
    };

    /// \ingroup group-functional
    /// \sa `on_fn`
    RANGES_INLINE_VARIABLE(on_fn, on)
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
