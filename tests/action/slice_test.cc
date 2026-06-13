#include <gtest/gtest.h>
#include <array>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/stride.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/equal.h>
#include <fermat/action/slice.h>
#include <fermat/range/conversion.h>

using namespace ranges;

TEST(ActionSliceTest, SliceFromCopy) {
    auto v = views::ints(0, 100) | to<std::vector>();

    auto v2 = v | copy | actions::slice(10, 20);
    EXPECT_EQ(v2.size(), 10u);
    static_assert(std::is_same_v<decltype(v), decltype(v2)>);
    EXPECT_TRUE((v2 == std::vector<int>{10, 11, 12, 13, 14, 15, 16, 17, 18, 19}));

    v2 = std::move(v2) | actions::slice(2, 8);
    EXPECT_TRUE((v2 == std::vector<int>{12, 13, 14, 15, 16, 17}));

    v2 |= actions::slice(0, 0);
    EXPECT_EQ(v2.size(), 0u);

    auto& v3 = actions::slice(v, 90, 100);
    EXPECT_EQ(&v3, &v);
    EXPECT_TRUE((v == std::vector<int>{90, 91, 92, 93, 94, 95, 96, 97, 98, 99}));
}

TEST(ActionSliceTest, SliceWithEndMinus) {
    auto rng = views::ints(0, 100) | to<std::vector>();

    rng |= actions::slice(20, end - 70);
    EXPECT_EQ(rng.size(), 10u);
    EXPECT_TRUE((rng == std::vector<int>{20, 21, 22, 23, 24, 25, 26, 27, 28, 29}));

    rng |= actions::slice(end - 10, end - 5);
    EXPECT_EQ(rng.size(), 5u);
    EXPECT_TRUE((rng == std::vector<int>{20, 21, 22, 23, 24}));
}

TEST(ActionSliceTest, SliceToEnd) {
    auto rng = views::ints(0, 100) | to<std::vector>();

    auto& rng_copy = actions::slice(rng, 90, end);
    EXPECT_EQ(&rng_copy, &rng);
    EXPECT_EQ(rng_copy.size(), 10u);
    EXPECT_TRUE((rng == std::vector<int>{90, 91, 92, 93, 94, 95, 96, 97, 98, 99}));

    rng |= actions::slice(end - 5, end);
    EXPECT_EQ(&rng_copy, &rng);
    EXPECT_EQ(rng_copy.size(), 5u);
    EXPECT_TRUE((rng == std::vector<int>{95, 96, 97, 98, 99}));
}