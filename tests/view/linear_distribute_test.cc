/// linear_distribute_gtest.cpp
/// Google Test conversion of range-v3 linear_distribute view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Note: The view provides only forward iteration (not random access),
/// so tests avoid arithmetic on iterators.

#include <gtest/gtest.h>

#include <vector>
#include <cmath>
#include <type_traits>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/linear_distribute.h>

using namespace ranges;

/// Helper: check_equal for ranges vs initializer_list
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Helper: fuzzy equality for floating point comparisons
template<typename T>
bool fuzzy_equal(T a, T b, T eps = T(1e-6)) {
    return std::abs(a - b) <= eps;
}

template<typename Rng>
void check_float_equal(Rng &&rng, std::initializer_list<double> expected, double eps = 1e-6) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_TRUE(fuzzy_equal(*it, val, eps));
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

TEST(LinearDistributeTest, IntegerTwoPoints) {
    auto irng = views::linear_distribute(0, 1, 2);
    static_assert(view_<decltype(irng)>);
    static_assert(sized_range<decltype(irng)>);
    EXPECT_EQ(size(irng), 2u);
    check_equal(irng, {0, 1});
}

TEST(LinearDistributeTest, IntegerThreePoints) {
    auto irng = views::linear_distribute(1, 3, 3);
    static_assert(view_<decltype(irng)>);
    static_assert(sized_range<decltype(irng)>);
    EXPECT_EQ(size(irng), 3u);
    check_equal(irng, {1, 2, 3});
}

TEST(LinearDistributeTest, IntegerToDoubleConversion) {
    auto irng = views::linear_distribute(0, 21, 22);
    static_assert(std::is_same_v<range_value_t<decltype(irng)>, int>);
    EXPECT_EQ(size(irng), 22u);
    std::vector<int> expected;
    for (int i = 0; i <= 21; ++i) expected.push_back(i);
    auto it = ranges::begin(irng);
    for (int d: expected) {
        EXPECT_EQ(*it, d);
        ++it;
    }
}

TEST(LinearDistributeTest, FloatingPointRange) {
    auto frng = views::linear_distribute(0.0, 1.0, 11);
    static_assert(view_<decltype(frng)>);
    static_assert(sized_range<decltype(frng)>);
    EXPECT_EQ(size(frng), 11u);
    std::initializer_list<double> expected = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    check_float_equal(frng, expected);
}

TEST(LinearDistributeTest, FloatingPointRangeWithStartNotZero) {
    using namespace ranges;
    auto frng = views::linear_distribute(1.0, 3.0, 21);
    EXPECT_EQ(size(frng), 21u);

    // Check first element
    auto it = begin(frng);
    EXPECT_DOUBLE_EQ(*it, 1.0);

    // Check third element (index 2)
    it = begin(frng);
    ++it; ++it;
    EXPECT_NEAR(*it, 1.2, 1e-12);

    // Check last element (index 20)
    auto last = begin(frng);
    for (std::size_t i = 0; i < size(frng) - 1; ++i) ++last;
    EXPECT_DOUBLE_EQ(*last, 3.0);
}

TEST(LinearDistributeTest, EmptyIntervalInteger) {
    auto irng = views::linear_distribute(0, 0, 1);
    EXPECT_EQ(size(irng), 1u);
    check_equal(irng, {0});
}

TEST(LinearDistributeTest, EmptyIntervalFloat) {
    auto frng = views::linear_distribute(0., 0., 3);
    EXPECT_EQ(size(frng), 3u);
    check_equal(frng, {0., 0., 0.});
}

TEST(LinearDistributeTest, Regression1088) {
    auto ld = views::linear_distribute(1, 10, 10);
    EXPECT_EQ(ld.size(), 10u);
    check_equal(ld, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    // Use std::next to advance (forward iterator only)
    auto it = ranges::begin(ld);
    it = std::next(it, 3);
    EXPECT_EQ(*it, 4);
}

TEST(LinearDistributeTest, IntegralSpacing) {
    auto irng = views::linear_distribute(0, 10, 22);
    EXPECT_EQ(size(irng), 22u);
    EXPECT_EQ(*ranges::begin(irng), 0);
    // Check last element: iterate to end and back up one (forward only)
    auto it = ranges::begin(irng);
    for (std::size_t i = 0; i < size(irng) - 1; ++i) ++it;
    EXPECT_EQ(*it, 10);
    // Check monotonic increase
    auto prev = ranges::begin(irng);
    auto cur = std::next(prev);
    for (std::size_t i = 1; i < 22; ++i) {
        EXPECT_LE(*prev, *cur);
        prev = cur;
        ++cur;
    }
}

// Additional test: ensure the view can be used in range-based for loops
TEST(LinearDistributeTest, RangeBasedFor) {
    int sum = 0;
    for (int x: views::linear_distribute(1, 5, 5)) {
        sum += x;
    }
    EXPECT_EQ(sum, 15); // 1+2+3+4+5 = 15
}
