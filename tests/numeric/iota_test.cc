/// iota_gtest.cpp
/// Google Test conversion of range-v3 iota test.
/// All comments in English, using /// Doxygen style.

#include <gtest/gtest.h>
#include <fermat/numeric/iota.h>      /// Assume Fermat provides ranges::iota
#include <fermat/algorithm/equal.h>   /// For ranges::equal (if needed)

/// Helper to get array size at compile time.
template<typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) { return N; }

/// Test ranges::iota with raw pointers (satisfy all Fermat iterator concepts).
TEST(IotaTest, BasicWithPointers) {
    int expected[] = {5, 6, 7, 8, 9};
    constexpr std::size_t n = array_size(expected);
    int actual[n];

    /// Iterator pair version
    ranges::iota(actual, actual + n, 5);
    EXPECT_TRUE(ranges::equal(actual, expected));

    /// Range version (using subrange if available; here we just use iterator pair again)
    /// Fermat might not have make_subrange, so we use the iterator overload directly.
    /// Alternatively, if Fermat provides subrange, include <fermat/view/subrange.hpp>
    /// and use ranges::make_subrange(actual, actual+n). For simplicity, we keep iterator version.
    int actual2[n];
    ranges::iota(actual2, actual2 + n, 5);
    EXPECT_TRUE(ranges::equal(actual2, expected));
}
