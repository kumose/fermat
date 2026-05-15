///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef FERMAT_COMPARE_H
#define FERMAT_COMPARE_H


#include <fermat/base/eabase.h>

namespace fermat {
#if defined(KM_COMPILER_HAS_THREE_WAY_COMPARISON)
	struct synth_three_way {
		template<typename T, typename U>
		constexpr auto operator()(const T &t, const U &u) const requires requires {
			{
				t < u
			}
			->
			std::convertible_to<bool>;
			{
				u < t
			}
			->
			std::convertible_to<bool>;
		}

		{
			if constexpr (std::three_way_comparable_with<T, U>)
			{
				return t <=> u;
			}
			else
			{
				return (t < u) ? std::weak_ordering::less :
				       (u < t) ? std::weak_ordering::greater :
                                 std::weak_ordering::equivalent;
			}
		}
	};

	template<typename T, typename U=T>
	using synth_three_way_result = decltype(synth_three_way{}(std::declval<T &>(), std::declval<U &>()));
#endif
} // namespace fermat


#endif // Header include guard
