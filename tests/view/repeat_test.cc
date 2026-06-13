/// repeat_gtest.cpp
/// Google Test conversion of range-v3 repeat view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/repeat.h>        /// views::repeat
#include <fermat/view/take.h>          /// views::take

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

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(RepeatTest, Basic) {
    using namespace ranges;

    auto rng = views::repeat(9) | views::take(10);

    static_assert(view_<decltype(rng)>, "");
    static_assert(sized_range<decltype(rng)>, "");
    static_assert(random_access_iterator<decltype(rng.begin())>, "");

    check_equal(rng, {9, 9, 9, 9, 9, 9, 9, 9, 9, 9});
}
