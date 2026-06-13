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
#ifndef RANGES_V3_ALGORITHM_GENERATE_N_HPP
#define RANGES_V3_ALGORITHM_GENERATE_N_HPP

#include <tuple>
#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    template<typename O, typename F>
    using generate_n_result = detail::out_fun_result<O, F>;

    RANGES_FUNC_BEGIN(generate_n)

        /// \brief function template \c generate_n
        template(typename O, typename F)(
            requires invocable<F &> AND output_iterator<O, invoke_result_t<F &>>)
        constexpr generate_n_result<O, F> //
        RANGES_FUNC(generate_n)(O first, iter_difference_t<O> n, F fun)
        {
            RANGES_EXPECT(n >= 0);
            auto norig = n;
            auto b = uncounted(first);
            for(; 0 != n; ++b, --n)
                *b = invoke(fun);
            return {recounted(first, b, norig), detail::move(fun)};
        }

    RANGES_FUNC_END(generate_n)

    namespace cpp20
    {
        using fermat::ranges::generate_n;
        using fermat::ranges::generate_n_result;
    } // namespace cpp20
    // @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
