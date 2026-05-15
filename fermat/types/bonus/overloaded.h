/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <fermat/types/type_traits.h>


// 4512/4626 - 'class' : assignment operator could not be generated.  // This disabling would best be put elsewhere.
KM_DISABLE_VC_WARNING (
4512
4626
);

namespace fermat {
    ///////////////////////////////////////////////////////////////////////////
	/// overloaded
	///
	/// A helper class that permits you to combine multiple function objects into one.
	/// Typically, this helper is really handy when visiting an fermat::variant with multiple lambdas.
	/// Example:
	///
	/// fermat::variant<int, string> v{42};
	///
	/// fermat::visit(
	///	 fermat::overloaded{
	///		  [](const int& x) { std::cout << "Visited an integer: " << x  << "\n"; }, // Will reach that lambda with x == 42.
	///		  [](const string& s) { std::cout << "Visited an string: " << s  << "\n"; }
	///	 },
	///	 v
	/// );
	///////////////////////////////////////////////////////////////////////////
    template<class... T>
    struct overloaded;

    template<class T>
    struct overloaded<T> : T {
        template<class U>
        constexpr overloaded(U &&u) : T(std::forward<U>(u)) {
        }

        using T::operator();
    };

    template<class T, class... R>
    struct overloaded<T, R...> : T, overloaded<R...> {
        template<class U, class... V>
        constexpr overloaded(U &&u, V &&... v) : T(std::forward<U>(u)), overloaded<R...>(std::forward<V>(v)...) {
        }

        using T::operator();
        using overloaded<R...>::operator();
    };

#ifdef __cpp_deduction_guides
    template<class... T>
    overloaded(T...) -> overloaded<T...>;
#endif

    ///////////////////////////////////////////////////////////////////////////
	/// make_overloaded
	///
	/// Helper function to create an overloaded instance when lacking deduction guides.
	/// make_overloaded(f1, f2, f3) == overloaded{f1, f2, f3}
	///////////////////////////////////////////////////////////////////////////
    template<class... T>
    constexpr overloaded<typename fermat::remove_cvref<T>::type...> make_overloaded(T &&... t) {
        return overloaded<typename fermat::remove_cvref<T>::type...>{std::forward<T>(t)...};
    }
} // namespace fermat

KM_RESTORE_VC_WARNING();
