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
#ifndef RANGES_V3_ALGORITHM_COPY_N_HPP
#define RANGES_V3_ALGORITHM_COPY_N_HPP

#include <functional>
#include <tuple>
#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
#include <fermat/functional/identity.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace ranges
{
    /// \addtogroup group-algorithms
    /// @{
    template<typename I, typename O>
    using copy_n_result = detail::in_out_result<I, O>;

    RANGES_FUNC_BEGIN(copy_n)

        /// \brief function template \c copy_n
        template(typename I, typename O, typename P = identity)(
            requires input_iterator<I> AND weakly_incrementable<O> AND
            indirectly_copyable<I, O>)
        constexpr copy_n_result<I, O> RANGES_FUNC(copy_n)(I first, iter_difference_t<I> n, O out)
        {
            RANGES_EXPECT(0 <= n);
            auto norig = n;
            auto b = uncounted(first);
            for(; n != 0; ++b, ++out, --n)
                *out = *b;
            return {recounted(first, b, norig), out};
        }

    RANGES_FUNC_END(copy_n)

    namespace cpp20
    {
        using ranges::copy_n;
        using ranges::copy_n_result;
    } // namespace cpp20
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
