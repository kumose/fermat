/// remove_gtest.cpp
/// Google Test conversion of range-v3 remove view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>

#include <fermat/view/remove.h>       /// views::remove

/// ------------------------------------------------------------
/// Test type with projection
/// ------------------------------------------------------------
struct Int {
    int i;
};

bool operator==(Int left, Int right) {
    return left.i == right.i;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for Int
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<Int> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(it->i, val.i);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(RemoveTest, Straight) {
    using namespace ranges;

    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto out = vec | views::remove(2);
    check_equal(out, {1, 3, 4, 5});
}

TEST(RemoveTest, Projection) {
    using namespace ranges;

    const std::vector<Int> vec{ Int{1}, Int{2}, Int{3}, Int{4}, Int{5} };
    auto out = vec | views::remove(2, &Int::i);
    check_equal(out, {Int{1}, Int{3}, Int{4}, Int{5}});
}
