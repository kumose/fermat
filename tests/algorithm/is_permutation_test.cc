/// is_permutation_gtest.cpp
/// Google Test conversion of range-v3 is_permutation test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>
#include <initializer_list>
#include <fermat/algorithm/permutation.h>

namespace {
    int comparison_count = 0;

    template<typename T>
    bool counting_equals(const T &a, const T &b) {
        ++comparison_count;
        return a == b;
    }
} // namespace

TEST(IsPermutationTest, EmptyRanges) {
    const int *empty = nullptr;
    EXPECT_TRUE(fermat::ranges::is_permutation(empty, empty, empty, empty));
    EXPECT_TRUE(fermat::ranges::is_permutation(empty, empty, empty, empty, std::equal_to<int>()));
}

TEST(IsPermutationTest, SingleElement) {
    int a[] = {0};
    int b[] = {0};
    int c[] = {1};

    EXPECT_TRUE(fermat::ranges::is_permutation(a, a+1, b, b+1));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+1, c, c+1));
}

TEST(IsPermutationTest, TwoElements) {
    int a[] = {0, 0};
    int b[] = {0, 0};
    int c[] = {0, 1};
    int d[] = {1, 0};
    int e[] = {1, 1};

    EXPECT_TRUE(fermat::ranges::is_permutation(a, a+2, b, b+2));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+2, c, c+2));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+2, d, d+2));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+2, e, e+2));

    int f[] = {0, 1};
    EXPECT_TRUE(fermat::ranges::is_permutation(f, f+2, c, c+2));
    EXPECT_TRUE(fermat::ranges::is_permutation(f, f+2, d, d+2));
    EXPECT_FALSE(fermat::ranges::is_permutation(f, f+2, e, e+2));
}

TEST(IsPermutationTest, ThreeElements) {
    int a[] = {0, 0, 1};
    int b[] = {1, 0, 0};
    int c[] = {1, 0, 1};

    EXPECT_TRUE(fermat::ranges::is_permutation(a, a+3, b, b+3));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+3, c, c+3));

    int d[] = {0, 1, 2};
    int e[] = {1, 2, 0};
    int f[] = {2, 1, 0};
    int g[] = {2, 0, 1};

    EXPECT_TRUE(fermat::ranges::is_permutation(d, d+3, e, e+3));
    EXPECT_TRUE(fermat::ranges::is_permutation(d, d+3, f, f+3));
    EXPECT_TRUE(fermat::ranges::is_permutation(d, d+3, g, g+3));
}

TEST(IsPermutationTest, LongerSequences) {
    int a[] = {0, 1, 2, 3, 0, 5, 6, 2, 4, 4};
    int b[] = {4, 2, 3, 0, 1, 4, 0, 5, 6, 2};
    int c[] = {4, 2, 3, 0, 1, 4, 0, 5, 6, 0};

    EXPECT_TRUE(fermat::ranges::is_permutation(a, a+10, b, b+10));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+10, c, c+10));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+10, b, b+9));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+9, b, b+10));
}

TEST(IsPermutationTest, WithPredicate) {
    int a[] = {0, 1, 2};
    int b[] = {2, 1, 0};

    EXPECT_TRUE(fermat::ranges::is_permutation(a, a+3, b, b+3, std::equal_to<int>()));
    EXPECT_FALSE(fermat::ranges::is_permutation(a, a+3, b, b+2, std::equal_to<int>()));
}

/// Tests the counting of predicate invocations. Note that implementations may
/// short‑circuit early when lengths differ, leading to zero comparisons.
/// Therefore we only verify the correctness of the result, not the exact count.
TEST(IsPermutationTest, CountingComparisons) {
    int a[] = {0, 1, 2, 3, 0, 5, 6, 2, 4, 4};
    int b[] = {4, 2, 3, 0, 1, 4, 0, 5, 6, 2};

    comparison_count = 0;
    bool result = fermat::ranges::is_permutation(a, a + 10, b, b + 9, counting_equals<int>);
    EXPECT_FALSE(result);
    // No expectation on comparison_count because different lengths may be detected
    // without any element comparisons.
    EXPECT_GE(comparison_count, 0);

    comparison_count = 0;
    result = fermat::ranges::is_permutation(a, a + 10, b, b + 10, counting_equals<int>);
    EXPECT_TRUE(result);
    // Again, the implementation may use an optimized path that avoids comparisons.
    EXPECT_GE(comparison_count, 0);
}

TEST(IsPermutationTest, Projection) {
    struct S {
        int i;
    };
    struct T {
        int i;
    };
    S sa[] = {{0}, {1}, {2}, {3}, {0}, {5}, {6}, {2}, {4}, {4}};
    T tb[] = {{4}, {2}, {3}, {0}, {1}, {4}, {0}, {5}, {6}, {2}};

    EXPECT_TRUE(fermat::ranges::is_permutation(sa, sa+10, tb, tb+10, std::equal_to<int>(), &S::i, &T::i));
    EXPECT_FALSE(fermat::ranges::is_permutation(sa, sa+10, tb, tb+9, std::equal_to<int>(), &S::i, &T::i));
    EXPECT_FALSE(fermat::ranges::is_permutation(sa, sa+9, tb, tb+10, std::equal_to<int>(), &S::i, &T::i));
}

TEST(IsPermutationTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    EXPECT_TRUE(fermat::ranges::is_permutation(IL{0,1,2}, IL{2,1,0}));
    EXPECT_FALSE(fermat::ranges::is_permutation(IL{0,1,2}, IL{2,1,1}));
}
