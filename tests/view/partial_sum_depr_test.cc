/// partial_sum_gtest.cpp
/// Google Test conversion of range-v3 partial_sum view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <fermat/view/partial_sum.h>   /// views::partial_sum

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(PartialSumTest, Basic) {
    using namespace fermat::ranges;

    static int const some_ints[] = {0, 1, 2, 3, 4};
    auto rng = some_ints | views::partial_sum();
    check_equal(rng, {0, 1, 3, 6, 10});
}
