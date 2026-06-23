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
#ifndef RANGES_V3_ALGORITHM_REVERSE_HPP
#define RANGES_V3_ALGORITHM_REVERSE_HPP

#include <fermat/range_fwd.h>

#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/dangling.h>
#include <fermat/range/traits.h>
#include <fermat/utility/static_const.h>
#include <fermat/utility/swap.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{

    /// \cond
    namespace detail
    {
        template<typename I>
        constexpr void reverse_impl(I first, I last, std::bidirectional_iterator_tag)
        {
            while(first != last)
            {
                if(first == --last)
                    break;
                fermat::ranges::iter_swap(first, last);
                ++first;
            }
        }

        template<typename I>
        constexpr void reverse_impl(I first, I last, std::random_access_iterator_tag)
        {
            if(first != last)
                for(; first < --last; ++first)
                    fermat::ranges::iter_swap(first, last);
        }
    } // namespace detail
    /// \endcond

    RANGES_FUNC_BEGIN(reverse)

        /// \brief function template \c reverse
        template(typename I, typename S)(
            requires bidirectional_iterator<I> AND sentinel_for<S, I> AND permutable<I>)
        constexpr I RANGES_FUNC(reverse)(I first, S end_)
        {
            I last = fermat::ranges::next(first, end_);
            detail::reverse_impl(first, last, iterator_tag_of<I>{});
            return last;
        }

        /// \overload
        template(typename Rng, typename I = iterator_t<Rng>)(
            requires bidirectional_range<Rng> AND permutable<I>)
        constexpr borrowed_iterator_t<Rng> RANGES_FUNC(reverse)(Rng && rng) //
        {
            return (*this)(begin(rng), end(rng));
        }

    RANGES_FUNC_END(reverse)

    namespace cpp20
    {
        using fermat::ranges::reverse;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
