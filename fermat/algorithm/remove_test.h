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
#ifndef RANGES_V3_ALGORITHM_REMOVE_HPP
#define RANGES_V3_ALGORITHM_REMOVE_HPP

#include <fermat/meta/meta.h>

#include <fermat/range_fwd.h>

#include <fermat/algorithm/find.h>
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

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
    /// \addtogroup group-algorithms
    /// @{
    RANGES_FUNC_BEGIN(remove)

        /// \brief function template \c remove
        template(typename I, typename S, typename T, typename P = identity)(
            requires permutable<I> AND sentinel_for<S, I> AND
            indirect_relation<equal_to, projected<I, P>, T const *>)
        constexpr I RANGES_FUNC(remove)(I first, S last, T const & val, P proj = P{})
        {
            first = find(std::move(first), last, val, fermat::ranges::ref(proj));
            if(first != last)
            {
                for(I i = next(first); i != last; ++i)
                {
                    if(!(invoke(proj, *i) == val))
                    {
                        *first = iter_move(i);
                        ++first;
                    }
                }
            }
            return first;
        }

        /// \overload
        template(typename Rng, typename T, typename P = identity)(
            requires forward_range<Rng> AND permutable<iterator_t<Rng>> AND
            indirect_relation<equal_to, projected<iterator_t<Rng>, P>, T const *>)
        constexpr borrowed_iterator_t<Rng> //
        RANGES_FUNC(remove)(Rng && rng, T const & val, P proj = P{})
        {
            return (*this)(begin(rng), end(rng), val, std::move(proj));
        }

    RANGES_FUNC_END(remove)

    namespace cpp20
    {
        using fermat::ranges::remove;
    }
    /// @}
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
