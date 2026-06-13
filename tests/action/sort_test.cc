#include <gtest/gtest.h>
#include <array>
#include <random>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/stride.h>
#include <fermat/view/take.h>
#include <fermat/algorithm/shuffle.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/is_sorted.h>
#include <fermat/algorithm/equal.h>
#include <fermat/action/shuffle.h>
#include <fermat/action/sort.h>
#include <fermat/range/conversion.h>

using namespace fermat::ranges;

TEST(ActionSortShuffleTest, ShuffleThenSort) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    v |= actions::shuffle(gen);
    EXPECT_FALSE(is_sorted(v));

    auto v2 = v | copy | actions::sort;
    EXPECT_EQ(v2.size(), v.size());
    EXPECT_TRUE(is_sorted(v2));
    EXPECT_FALSE(is_sorted(v));
    static_assert(std::is_same_v<decltype(v), decltype(v2)>);

    v |= actions::sort;
    EXPECT_TRUE(is_sorted(v));

    v |= actions::shuffle(gen);
    EXPECT_FALSE(is_sorted(v));

    v = std::move(v) | actions::sort(std::less<int>());
    EXPECT_TRUE(is_sorted(v));
    EXPECT_TRUE(equal(v, v2));
}

TEST(ActionSortShuffleTest, DirectCallByReference) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    shuffle(v, gen);
    EXPECT_FALSE(is_sorted(v));

    auto& v3 = actions::sort(v);
    EXPECT_TRUE(is_sorted(v));
    EXPECT_EQ(&v3, &v);
}

TEST(ActionSortShuffleTest, PipeViewToSort) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    actions::sort(v, std::greater<int>());

    v | views::stride(2) | actions::sort;
    auto result = views::take(v, 10) | to<std::vector>();
    std::vector<int> expected = {1, 98, 3, 96, 5, 94, 7, 92, 9, 90};
    EXPECT_EQ(result, expected);
}

TEST(ActionSortShuffleTest, RefView) {
    auto v = views::ints(0, 100) | to<std::vector>();
    auto r = views::ref(v);
    r |= actions::sort;
    EXPECT_TRUE(is_sorted(v));
}