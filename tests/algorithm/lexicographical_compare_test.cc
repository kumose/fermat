/// lexicographical_compare_gtest.cpp
/// Google Test conversion of range-v3 lexicographical_compare test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>
#include <array>
#include <functional>
#include <fermat/algorithm/lexicographical_compare.h>

TEST(LexicographicalCompareTest, Basic) {
    int a[] = {1, 2, 3, 4};
    int b[] = {1, 2, 3};

    // a vs b (shorter)
    EXPECT_FALSE(ranges::lexicographical_compare(a, a+4, b, b+2)); // a >= b
    EXPECT_TRUE(ranges::lexicographical_compare(b, b+2, a, a+4)); // b < a

    // full length compare
    EXPECT_FALSE(ranges::lexicographical_compare(a, a+4, b, b+3)); // a > b
    EXPECT_TRUE(ranges::lexicographical_compare(b, b+3, a, a+4)); // b < a

    // compare from offset
    EXPECT_TRUE(ranges::lexicographical_compare(a, a+4, b+1, b+3)); // a (1,2,3,4) vs (2,3) -> a < b
    EXPECT_FALSE(ranges::lexicographical_compare(b+1, b+3, a, a+4)); // b > a

    // empty ranges
    // Standard says: empty range is less than any non‑empty range.
    // This implementation follows that rule.
    EXPECT_FALSE(ranges::lexicographical_compare(a, a, b, b)); // both empty -> false
    EXPECT_TRUE(ranges::lexicographical_compare(b, b, a, a+4)); // empty < non‑empty -> true
    EXPECT_TRUE(ranges::lexicographical_compare(a, a, b, b+1)); // empty < non‑empty -> true
    EXPECT_FALSE(ranges::lexicographical_compare(b, b+1, a, a)); // non‑empty > empty -> false
}

TEST(LexicographicalCompareTest, WithComparator) {
    int a[] = {1, 2, 3, 4};
    int b[] = {1, 2, 3};
    std::greater<int> cmp;

    EXPECT_FALSE(ranges::lexicographical_compare(a, a+4, b, b+2, cmp));
    EXPECT_TRUE(ranges::lexicographical_compare(b, b+2, a, a+4, cmp));

    EXPECT_FALSE(ranges::lexicographical_compare(a, a+4, b, b+3, cmp));
    EXPECT_TRUE(ranges::lexicographical_compare(b, b+3, a, a+4, cmp));

    EXPECT_FALSE(ranges::lexicographical_compare(a, a+4, b+1, b+3, cmp));
    EXPECT_TRUE(ranges::lexicographical_compare(b+1, b+3, a, a+4, cmp));
}

TEST(LexicographicalCompareTest, RangeVersions) {
    int a[] = {1, 2, 3, 4};
    int b[] = {1, 2, 3};

    // using ranges directly
    EXPECT_FALSE(ranges::lexicographical_compare(a, b));
    EXPECT_TRUE(ranges::lexicographical_compare(b, a));

    // with comparator
    std::greater<int> cmp;
    EXPECT_FALSE(ranges::lexicographical_compare(a, b, cmp));
    EXPECT_TRUE(ranges::lexicographical_compare(b, a, cmp));
}

TEST(LexicographicalCompareTest, Constexpr) {
    constexpr std::array<int, 4> a{{1, 2, 3, 4}};
    constexpr std::array<int, 3> b{{1, 2, 3}};

    // iterator pair version (constexpr capable)
    static_assert(!ranges::lexicographical_compare(a.begin(), a.end(), b.begin(), b.begin() + 2), "");
    static_assert(ranges::lexicographical_compare(b.begin(), b.begin() + 2, a.begin(), a.end()), "");
    static_assert(!ranges::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()), "");
    static_assert(ranges::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()), "");
    static_assert(ranges::lexicographical_compare(a.begin(), a.end(), b.begin() + 1, b.end()), "");
    static_assert(!ranges::lexicographical_compare(b.begin() + 1, b.end(), a.begin(), a.end()), "");

    // with comparator
    constexpr std::greater<int> cmp;
    static_assert(!ranges::lexicographical_compare(a.begin(), a.end(), b.begin(), b.begin() + 2, cmp), "");
    static_assert(ranges::lexicographical_compare(b.begin(), b.begin() + 2, a.begin(), a.end(), cmp), "");
    static_assert(!ranges::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), cmp), "");
    static_assert(ranges::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end(), cmp), "");
    static_assert(!ranges::lexicographical_compare(a.begin(), a.end(), b.begin() + 1, b.end(), cmp), "");
    static_assert(ranges::lexicographical_compare(b.begin() + 1, b.end(), a.begin(), a.end(), cmp), "");
}
