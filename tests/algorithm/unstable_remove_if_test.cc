// unstable_remove_if_gtest.cpp
// Google Test conversion of range-v3 unstable_remove_if algorithm test.
// Uses fermat::range and standard C++ array.
// All comments in English.

#include <gtest/gtest.h>
#include <array>

#include <fermat/algorithm/equal.h>
#include <fermat/algorithm/unstable_remove_if.h>
#include <fermat/view/subrange.h>


constexpr bool is_even(int i) {
    return i % 2 == 0;
}

constexpr bool test_constexpr() {
    using IL = std::initializer_list<int>;

    // Use std::array instead of test::array
    std::array<int, 5> arr{{1, 2, 3, 4, 5}};
    const auto it = ranges::unstable_remove_if(arr, is_even);
    if (it != arr.begin() + 3) return false;
    if (!ranges::equal(ranges::make_subrange(arr.begin(), it), IL{1, 5, 3})) return false;
    return true;
}

TEST(UnstableRemoveIfTest, Constexpr) {
    static_assert(test_constexpr(), "unstable_remove_if constexpr test failed");
    EXPECT_TRUE(test_constexpr()); // runtime check for completeness
}
