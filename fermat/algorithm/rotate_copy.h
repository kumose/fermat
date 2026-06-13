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
#ifndef RANGES_V3_ALGORITHM_ROTATE_COPY_HPP
#define RANGES_V3_ALGORITHM_ROTATE_COPY_HPP

#include <functional>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/result_types.h>
#include <fermat/functional/identity.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace ranges
{
    /// \addtogroup group-algorithms
    /// @{
    template<typename I, typename O>
    using rotate_copy_result = detail::in_out_result<I, O>;

    RANGES_FUNC_BEGIN(rotate_copy)

        /// \brief function template \c rotate_copy
        template(typename I, typename S, typename O, typename P = identity)(
            requires forward_iterator<I> AND sentinel_for<S, I> AND
            weakly_incrementable<O> AND indirectly_copyable<I, O>)
        constexpr rotate_copy_result<I, O> //
        RANGES_FUNC(rotate_copy)(I first, I middle, S last, O out) //
        {
            auto res = ranges::copy(middle, std::move(last), std::move(out));
            return {std::move(res.in),
                    ranges::copy(std::move(first), middle, std::move(res.out)).out};
        }

        /// \overload
        template(typename Rng, typename O, typename P = identity)(
            requires range<Rng> AND weakly_incrementable<O> AND
            indirectly_copyable<iterator_t<Rng>, O>)
        constexpr rotate_copy_result<borrowed_iterator_t<Rng>, O> //
        RANGES_FUNC(rotate_copy)(Rng && rng, iterator_t<Rng> middle, O out) //
        {
            return (*this)(begin(rng), std::move(middle), end(rng), std::move(out));
        }

    RANGES_FUNC_END(rotate_copy)

    namespace cpp20
    {
        using ranges::rotate_copy;
        using ranges::rotate_copy_result;
    } // namespace cpp20
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
