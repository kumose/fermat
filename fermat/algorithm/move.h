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
#ifndef RANGES_V3_ALGORITHM_MOVE_HPP
#define RANGES_V3_ALGORITHM_MOVE_HPP

#include <utility>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/result_types.h>
#include <fermat/iterator/concepts.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/move.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    template<typename I, typename O>
    using move_result = detail::in_out_result<I, O>;

    RANGES_HIDDEN_DETAIL(namespace _move CPP_PP_LBRACE())
    RANGES_FUNC_BEGIN(move)

        /// \brief function template \c move
        template(typename I, typename S, typename O)(
            requires input_iterator<I> AND sentinel_for<S, I> AND
            weakly_incrementable<O> AND indirectly_movable<I, O>)
        constexpr move_result<I, O> RANGES_FUNC(move)(I first, S last, O out) //
        {
            for(; first != last; ++first, ++out)
                *out = iter_move(first);
            return {first, out};
        }

        /// \overload
        template(typename Rng, typename O)(
            requires input_range<Rng> AND weakly_incrementable<O> AND
            indirectly_movable<iterator_t<Rng>, O>)
        constexpr move_result<borrowed_iterator_t<Rng>, O> //
        RANGES_FUNC(move)(Rng && rng, O out)            //
        {
            return (*this)(begin(rng), end(rng), std::move(out));
        }

    RANGES_FUNC_END(move)
    RANGES_HIDDEN_DETAIL(CPP_PP_RBRACE())

#ifndef RANGES_DOXYGEN_INVOKED
    struct RANGES_EMPTY_BASES move_fn
      : aux::move_fn
      , _move::move_fn
    {
        using aux::move_fn::operator();
        using _move::move_fn::operator();
    };

    RANGES_INLINE_VARIABLE(move_fn, move)
#endif

    namespace cpp20
    {
        using fermat::ranges::move_result;
        using fermat::ranges::RANGES_HIDDEN_DETAIL(_move::) move;
    } // namespace cpp20
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
