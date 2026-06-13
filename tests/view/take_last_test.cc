/// take_last_gtest.cpp
/// Google Test conversion of range-v3 take_last view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
/// No main() function – entry point expected to be provided externally.

#include <gtest/gtest.h>

#include <fermat/range/access.h>                 /// ranges::begin, ranges::end
#include <fermat/range/primitives.h>             /// ranges::size
#include <fermat/view/take_last.h>               /// views::take_last
#include <fermat/utility/copy.h>                 /// ranges::copy (if needed)

/// ------------------------------------------------------------
/// has_type: static assertion that a value has given type
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
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

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(TakeLastTest, Basic) {
    using namespace ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5};
    auto rng0 = rgi | views::take_last(3);
    has_type<int&>(*begin(rng0));
    check_equal(rng0, {3, 4, 5});
    EXPECT_EQ(size(rng0), 3u);
}

TEST(TakeLastTest, LargerThanSize) {
    using namespace ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5};
    auto rng1 = rgi | views::take_last(7);
    check_equal(rng1, {0, 1, 2, 3, 4, 5});
}
