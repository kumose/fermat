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
#ifndef RANGES_V3_ALGORITHM_GENERATE_HPP
#define RANGES_V3_ALGORITHM_GENERATE_HPP

#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
#include <fermat/functional/invoke.h>
#include <fermat/functional/reference_wrapper.h>
#include <fermat/iterator/concepts.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    template<typename O, typename F>
    using generate_result = detail::out_fun_result<O, F>;

    RANGES_FUNC_BEGIN(generate)

        /// \brief function template \c generate_n
        template(typename O, typename S, typename F)(
            requires invocable<F &> AND output_iterator<O, invoke_result_t<F &>> AND
            sentinel_for<S, O>)
        constexpr generate_result<O, F> RANGES_FUNC(generate)(O first, S last, F fun) //
        {
            for(; first != last; ++first)
                *first = invoke(fun);
            return {detail::move(first), detail::move(fun)};
        }

        /// \overload
        template(typename Rng, typename F)(
            requires invocable<F &> AND output_range<Rng, invoke_result_t<F &>>)
        constexpr generate_result<borrowed_iterator_t<Rng>, F> //
        RANGES_FUNC(generate)(Rng && rng, F fun)
        {
            return {(*this)(begin(rng), end(rng), ref(fun)).out, detail::move(fun)};
        }

    RANGES_FUNC_END(generate)

    namespace cpp20
    {
        using fermat::ranges::generate;
        using fermat::ranges::generate_result;
    } // namespace cpp20
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
