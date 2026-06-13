#include <gtest/gtest.h>
#include <fermat/algorithm/contains.h>

TEST(ContainsTest, Basic) {
    using ranges::contains;

    constexpr int rng[] = {4, 2};
    const auto first = rng;
    const auto last = rng + 2;

    EXPECT_FALSE(contains(first, first, 0));
    EXPECT_FALSE(contains(first, last, 1));
    EXPECT_TRUE(contains(first, last, 2));
    EXPECT_FALSE(contains(first, last, 3));
    EXPECT_TRUE(contains(first, last, 4));

#ifndef RANGES_WORKAROUND_CLANG_23135
    static_assert(!contains(rng, 1), "");
    static_assert(contains(rng, 2), "");
    static_assert(!contains(rng, 3), "");
    static_assert(contains(rng, 4), "");
#endif
}