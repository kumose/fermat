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
//===-------------------------- algorithm ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef RANGES_V3_ALGORITHM_PARTITION_HPP
#define RANGES_V3_ALGORITHM_PARTITION_HPP

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
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
        template<typename I, typename S, typename C, typename P>
        constexpr I partition_impl(I first, S last, C pred, P proj, std::forward_iterator_tag)
        {
            while(true)
            {
                if(first == last)
                    return first;
                if(!invoke(pred, invoke(proj, *first)))
                    break;
                ++first;
            }
            for(I p = first; ++p != last;)
            {
                if(invoke(pred, invoke(proj, *p)))
                {
                    fermat::ranges::iter_swap(first, p);
                    ++first;
                }
            }
            return first;
        }

        template<typename I, typename S, typename C, typename P>
        constexpr I partition_impl(I first, S end_, C pred, P proj, std::bidirectional_iterator_tag)
        {
            I last = fermat::ranges::next(first, end_);
            while(true)
            {
                while(true)
                {
                    if(first == last)
                        return first;
                    if(!invoke(pred, invoke(proj, *first)))
                        break;
                    ++first;
                }
                do
                {
                    if(first == --last)
                        return first;
                } while(!invoke(pred, invoke(proj, *last)));
                fermat::ranges::iter_swap(first, last);
                ++first;
            }
        }
    } // namespace detail
    /// \endcond

    RANGES_FUNC_BEGIN(partition)

        /// \brief function template \c partition
        template(typename I, typename S, typename C, typename P = identity)(
            requires permutable<I> AND sentinel_for<S, I> AND
            indirect_unary_predicate<C, projected<I, P>>)
        constexpr I RANGES_FUNC(partition)(I first, S last, C pred, P proj = P{})
        {
            return detail::partition_impl(std::move(first),
                                          std::move(last),
                                          std::move(pred),
                                          std::move(proj),
                                          iterator_tag_of<I>());
        }

        /// \overload
        template(typename Rng, typename C, typename P = identity)(
            requires forward_range<Rng> AND permutable<iterator_t<Rng>> AND
            indirect_unary_predicate<C, projected<iterator_t<Rng>, P>>)
        constexpr borrowed_iterator_t<Rng> RANGES_FUNC(partition)(Rng && rng, C pred, P proj = P{})
        {
            return detail::partition_impl(begin(rng),
                                          end(rng),
                                          std::move(pred),
                                          std::move(proj),
                                          iterator_tag_of<iterator_t<Rng>>());
        }

    RANGES_FUNC_END(partition)

    namespace cpp20
    {
        using fermat::ranges::partition;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
