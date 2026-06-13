#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/fill.h>
#include <fermat/algorithm/fill_n.h>
#include <fermat/algorithm/equal.h>

TEST(FillNTest, Basic) {
    int ia[4] = {0};
    auto it = ranges::fill_n(ia, 2, 1);
    EXPECT_EQ(it, ia + 2);
    EXPECT_EQ(ia[0], 1);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 0);
    EXPECT_EQ(ia[3], 0);
}

TEST(FillNTest, RangeVersion) {
    int ia[4] = {0};
    ranges::fill_n(ranges::begin(ia), 3, 2);
    EXPECT_EQ(ia[0], 2);
    EXPECT_EQ(ia[1], 2);
    EXPECT_EQ(ia[2], 2);
    EXPECT_EQ(ia[3], 0);
}

// Constexpr fill_n test
constexpr auto fives() {
    std::array<int, 4> a{0};
    ranges::fill(a, 5);
    return a;
}

constexpr auto fives(int n) {
    std::array<int, 4> a{0};
    ranges::fill(ranges::begin(a), ranges::begin(a) + n, 5);
    return a;
}

TEST(FillNTest, Constexpr) {
    static_assert(ranges::equal(fives(), std::array<int, 4>{5,5,5,5}), "");
    static_assert(ranges::equal(fives(2), std::array<int, 4>{5,5,0,0}), "");
    static_assert(!ranges::equal(fives(2), std::array<int, 4>{5,5,5,5}), "");
}