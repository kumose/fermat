// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_UTILITY_FUNCTIONAL_HPP
#define RANGES_V3_UTILITY_FUNCTIONAL_HPP

#include <fermat/detail/config.h>
RANGES_DEPRECATED_HEADER(
    "This header has been deprecated. Please find what you are looking for in the "
    "range/v3/functional/ directory.")

#include <fermat/detail/with_braced_init_args.h>
#include <fermat/functional/arithmetic.h>
#include <fermat/functional/bind.h>
#include <fermat/functional/bind_back.h>
#include <fermat/functional/comparisons.h>
#include <fermat/functional/compose.h>
#include <fermat/functional/concepts.h>
#include <fermat/functional/identity.h>
#include <fermat/functional/indirect.h>
#include <fermat/functional/invoke.h>
#include <fermat/functional/not_fn.h>
#include <fermat/functional/on.h>
#include <fermat/functional/overload.h>
#include <fermat/functional/pipeable.h>
#include <fermat/functional/reference_wrapper.h>

namespace ranges
{
    using detail::with_braced_init_args;
} // namespace ranges

#endif
