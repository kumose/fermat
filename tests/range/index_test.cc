#include <vector>
#include <limits>
#include <gtest/gtest.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/c_str.h>
#include <fermat/view/iota.h>
#include <fermat/core.h>

TEST(RangeAtTest, BasicVector) {
    std::vector<int> vi{1, 2, 3, 4};

    EXPECT_EQ(fermat::ranges::index(vi, 0), 1);
    EXPECT_EQ(fermat::ranges::index(vi, 1), 2);
    EXPECT_EQ(fermat::ranges::index(vi, 2), 3);
    EXPECT_EQ(fermat::ranges::index(vi, 3), 4);

    EXPECT_EQ(fermat::ranges::at(vi, 0), 1);
    EXPECT_EQ(fermat::ranges::at(vi, 1), 2);
    EXPECT_EQ(fermat::ranges::at(vi, 2), 3);
    EXPECT_EQ(fermat::ranges::at(vi, 3), 4);

    // Out of range – should throw std::out_of_range with specific message
    EXPECT_THROW(fermat::ranges::at(vi, 4), std::out_of_range);
    EXPECT_THROW(fermat::ranges::at(vi, static_cast<std::size_t>(-1)), std::out_of_range);
}

TEST(RangeAtTest, Subrange) {
    std::vector<int> vi{1, 2, 3, 4};
    auto viv = fermat::ranges::make_subrange(vi.begin(), vi.end());

    EXPECT_EQ(viv.at(0), 1);
    EXPECT_EQ(viv.at(1), 2);
    EXPECT_EQ(viv.at(2), 3);
    EXPECT_EQ(viv.at(3), 4);

    EXPECT_THROW(viv.at(4), std::out_of_range);
    EXPECT_THROW(viv.at(-1), std::out_of_range);
}

TEST(RangeAtTest, ConstSubrange) {
    std::vector<int> vi{1, 2, 3, 4};
    auto viv = fermat::ranges::make_subrange(vi.begin(), vi.end());
    const auto cviv = viv;

    EXPECT_EQ(cviv.at(0), 1);
    EXPECT_EQ(cviv.at(1), 2);
    EXPECT_EQ(cviv.at(2), 3);
    EXPECT_EQ(cviv.at(3), 4);

    EXPECT_THROW(cviv.at(4), std::out_of_range);
    EXPECT_THROW(cviv.at(-1), std::out_of_range);
}

TEST(RangeAtTest, LargeIota) {
    auto rng = fermat::ranges::views::ints(std::int64_t{0},
                                   std::numeric_limits<std::int64_t>::max());
    constexpr std::int64_t last_idx = std::numeric_limits<std::int64_t>::max() - 1;
    EXPECT_EQ(fermat::ranges::index(rng, last_idx), last_idx);
    EXPECT_EQ(fermat::ranges::at(rng, last_idx), last_idx);
}

#if RANGES_CXX_CONSTEXPR >= RANGES_CXX_CONSTEXPR_14
TEST(RangeAtTest, ConstexprArray) {
    constexpr int vi[4] = {1, 2, 3, 4};
    constexpr int vi0 = fermat::ranges::index(vi, 0);
    static_assert(vi0 == 1, "fermat::ranges::index on constexpr array");
    constexpr int vi1 = fermat::ranges::at(vi, 1);
    static_assert(vi1 == 2, "fermat::ranges::at on constexpr array");
}
#endif