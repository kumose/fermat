/// semiregular_box_gtest.cpp
/// Google Test conversion of range-v3 semiregular_box test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <fermat/utility/semiregular_box.h>   /// ranges::semiregular_box_t

/// Test for issue #1499: ensure semiregular_box_t can be instantiated
/// with both value types and reference types.
TEST(SemiregularBoxTest, Issue1499) {
    /// instantiate semiregular_box_t for int and int&
    ranges::semiregular_box_t<int> box1;
    ranges::semiregular_box_t<int &> box2;

    /// avoid unused variable warnings
    ranges::detail::ignore_unused(box1, box2);
}
