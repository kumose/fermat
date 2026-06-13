/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//  Copyright Casey Carter 2015
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_ALGORITHM_MIN_HPP
#define RANGES_V3_ALGORITHM_MIN_HPP

#include <initializer_list>

#include <fermat/range_fwd.h>

#include <fermat/functional/comparisons.h>
#include <fermat/functional/identity.h>
#include <fermat/functional/invoke.h>
#include <fermat/iterator/concepts.h>
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
    RANGES_FUNC_BEGIN(min)

        /// \brief function template \c min
        template(typename T, typename C = less, typename P = identity)(
            requires indirect_strict_weak_order<C, projected<T const *, P>>)
        constexpr T const & RANGES_FUNC(min)(
            T const & a, T const & b, C pred = C{}, P proj = P{}) //
        {
            return invoke(pred, invoke(proj, b), invoke(proj, a)) ? b : a;
        }

        /// \overload
        template(typename Rng, typename C = less, typename P = identity)(
            requires input_range<Rng> AND
            indirect_strict_weak_order<C, projected<iterator_t<Rng>, P>> AND
            indirectly_copyable_storable<iterator_t<Rng>, range_value_t<Rng> *>)
        constexpr range_value_t<Rng> //
        RANGES_FUNC(min)(Rng && rng, C pred = C{}, P proj = P{}) //
        {
            auto first = ranges::begin(rng);
            auto last = ranges::end(rng);
            RANGES_EXPECT(first != last);
            range_value_t<Rng> result = *first;
            while(++first != last)
            {
                auto && tmp = *first;
                if(invoke(pred, invoke(proj, tmp), invoke(proj, result)))
                    result = (decltype(tmp) &&)tmp;
            }
            return result;
        }

        /// \overload
        template(typename T, typename C = less, typename P = identity)(
            requires copyable<T> AND
                indirect_strict_weak_order<C, projected<T const *, P>>)
        constexpr T RANGES_FUNC(min)(
            std::initializer_list<T> const && rng, C pred = C{}, P proj = P{}) //
        {
            return (*this)(rng, std::move(pred), std::move(proj));
        }

    RANGES_FUNC_END(min)

    namespace cpp20
    {
        using ranges::min;
    }
    /// @}
} // namespace ranges

#include <fermat/detail/epilogue.h>

#endif
