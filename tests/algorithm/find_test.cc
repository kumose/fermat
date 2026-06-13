// fill_test.cc
#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/fill.h>
#include <fermat/algorithm/fill_n.h>   // needed for fill_n in fives(int)
#include <fermat/algorithm/equal.h>

TEST(FillTest, Char) {
    char ca[4] = {0};
    ranges::fill(ca, ca + 4, char(1));
    EXPECT_EQ(ca[0], 1);
    EXPECT_EQ(ca[1], 1);
    EXPECT_EQ(ca[2], 1);
    EXPECT_EQ(ca[3], 1);

    ranges::fill(ca, char(2));
    EXPECT_EQ(ca[0], 2);
    EXPECT_EQ(ca[1], 2);
    EXPECT_EQ(ca[2], 2);
    EXPECT_EQ(ca[3], 2);
}

TEST(FillTest, Int) {
    int ia[4] = {0};
    ranges::fill(ia, ia + 4, 1);
    EXPECT_EQ(ia[0], 1);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 1);
    EXPECT_EQ(ia[3], 1);

    ranges::fill(ia, 2);
    EXPECT_EQ(ia[0], 2);
    EXPECT_EQ(ia[1], 2);
    EXPECT_EQ(ia[2], 2);
    EXPECT_EQ(ia[3], 2);
}

constexpr auto fives() {
    std::array<int, 4> a{0};
    ranges::fill(a, 5);
    return a;
}

constexpr auto fives(int n) {
    std::array<int, 4> a{0};
    ranges::fill_n(ranges::begin(a), n, 5);
    return a;
}

TEST(FillTest, Constexpr) {
    static_assert(ranges::equal(fives(), std::array<int, 4>{5,5,5,5}), "");
    static_assert(ranges::equal(fives(2), std::array<int, 4>{5,5,0,0}), "");
    static_assert(!ranges::equal(fives(2), std::array<int, 4>{5,5,5,5}), "");
}
