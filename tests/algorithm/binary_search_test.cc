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
//  Copyright 2005 - 2007 Adobe Systems Incorporated
//  Distributed under the MIT License(see accompanying file LICENSE_1_0_0.txt
//  or a copy at http://stlab.adobe.com/licenses.html)

#include <utility>
#include <gtest/gtest.h>
#include <fermat/core.h>
#include <fermat/algorithm/binary_search.h>

TEST(BinarySearchTest, Basic) {
    using ranges::begin;
    using ranges::end;
    using ranges::less;

    constexpr std::pair<int, int> a[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};
    constexpr const std::pair<int, int> c[] = {
        {0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};

    EXPECT_TRUE(ranges::binary_search(begin(a), end(a), a[0]));
    EXPECT_TRUE(ranges::binary_search(begin(a), end(a), a[1], less()));
    EXPECT_TRUE(ranges::binary_search(begin(a), end(a), 1, less(), &std::pair<int, int>::first));

    EXPECT_TRUE(ranges::binary_search(a, a[2]));
    EXPECT_TRUE(ranges::binary_search(c, c[3]));

    EXPECT_TRUE(ranges::binary_search(a, a[4], less()));
    EXPECT_TRUE(ranges::binary_search(c, c[5], less()));

    EXPECT_TRUE(ranges::binary_search(a, 1, less(), &std::pair<int, int>::first));
    EXPECT_TRUE(ranges::binary_search(c, 1, less(), &std::pair<int, int>::first));

    EXPECT_TRUE(ranges::binary_search(a, 0, less(), &std::pair<int, int>::first));
    EXPECT_TRUE(ranges::binary_search(c, 0, less(), &std::pair<int, int>::first));

    EXPECT_FALSE(ranges::binary_search(a, -1, less(), &std::pair<int, int>::first));
    EXPECT_FALSE(ranges::binary_search(c, -1, less(), &std::pair<int, int>::first));

    EXPECT_FALSE(ranges::binary_search(a, 4, less(), &std::pair<int, int>::first));
    EXPECT_FALSE(ranges::binary_search(c, 4, less(), &std::pair<int, int>::first));
}

TEST(BinarySearchTest, Constexpr) {
    constexpr std::pair<int, int> a[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};
    static_assert(ranges::binary_search(std::begin(a), std::end(a), a[0]), "");
    static_assert(ranges::binary_search(std::begin(a), std::end(a), a[1], ranges::less()), "");
    static_assert(ranges::binary_search(a, a[2]), "");
    static_assert(ranges::binary_search(a, a[4], ranges::less()), "");
    static_assert(!ranges::binary_search(a, std::make_pair(-1, -1), ranges::less()), "");
}