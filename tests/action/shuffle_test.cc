#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/stride.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/is_sorted.h>
#include <fermat/algorithm/equal.h>
#include <fermat/algorithm/sort.h>
#include <fermat/action/shuffle.h>
#include <fermat/range/conversion.h>

using namespace ranges;

TEST(ActionShuffleTest, ShuffleVsInts) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    auto v2 = v | copy | actions::shuffle(gen);

    EXPECT_TRUE(is_sorted(v));
    EXPECT_FALSE(is_sorted(v2));
    EXPECT_EQ(v2.size(), v.size());
    static_assert(std::is_same_v<decltype(v), decltype(v2)>);
    EXPECT_FALSE(equal(v, v2));
}

TEST(ActionShuffleTest, ShuffleThenSort) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    auto v2 = v | copy | actions::shuffle(gen);

    sort(v2);
    EXPECT_TRUE(is_sorted(v2));
    EXPECT_TRUE(equal(v, v2));
}

TEST(ActionShuffleTest, TwoShuffled) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    v |= actions::shuffle(gen);

    auto v2 = views::ints(0, 100) | to<std::vector>();
    v2 = std::move(v2) | actions::shuffle(gen);

    EXPECT_FALSE(is_sorted(v));
    EXPECT_FALSE(is_sorted(v2));
    EXPECT_EQ(v2.size(), v.size());
    EXPECT_FALSE(equal(v, v2));
}

TEST(ActionShuffleTest, DirectCallByReference) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    auto& v3 = actions::shuffle(v, gen);

    EXPECT_FALSE(is_sorted(v));
    EXPECT_EQ(&v3, &v);
}

TEST(ActionShuffleTest, ShuffleContainerReference) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    auto r = views::ref(v);
    r |= actions::shuffle(gen);

    EXPECT_FALSE(is_sorted(v));
}

TEST(ActionShuffleTest, PipeViewToShuffle) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    v | views::stride(2) | actions::shuffle(gen);

    EXPECT_FALSE(is_sorted(v));
}