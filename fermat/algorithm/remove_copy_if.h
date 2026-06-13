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
#ifndef RANGES_V3_ALGORITHM_REMOVE_COPY_IF_HPP
#define RANGES_V3_ALGORITHM_REMOVE_COPY_IF_HPP

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
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
    using remove_copy_if_result = detail::in_out_result<I, O>;

    RANGES_FUNC_BEGIN(remove_copy_if)

        /// \brief function template \c remove_copy_if
        template(typename I, typename S, typename O, typename C, typename P = identity)(
            requires input_iterator<I> AND sentinel_for<S, I> AND
            weakly_incrementable<O> AND indirect_unary_predicate<C, projected<I, P>> AND
            indirectly_copyable<I, O>)
        constexpr remove_copy_if_result<I, O> //
        RANGES_FUNC(remove_copy_if)(I first, S last, O out, C pred, P proj = P{}) //
        {
            for(; first != last; ++first)
            {
                auto && x = *first;
                if(!(invoke(pred, invoke(proj, x))))
                {
                    *out = (decltype(x) &&)x;
                    ++out;
                }
            }
            return {first, out};
        }

        /// \overload
        template(typename Rng, typename O, typename C, typename P = identity)(
            requires input_range<Rng> AND weakly_incrementable<O> AND
            indirect_unary_predicate<C, projected<iterator_t<Rng>, P>> AND
            indirectly_copyable<iterator_t<Rng>, O>)
        constexpr remove_copy_if_result<borrowed_iterator_t<Rng>, O> //
        RANGES_FUNC(remove_copy_if)(Rng && rng, O out, C pred, P proj = P{}) //
        {
            return (*this)(
                begin(rng), end(rng), std::move(out), std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(remove_copy_if)

    namespace cpp20
    {
        using ranges::remove_copy_if;
        using ranges::remove_copy_if_result;
    } // namespace cpp20
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
