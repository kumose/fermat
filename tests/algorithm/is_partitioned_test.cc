// is_partitioned_test.cc - GTest version
#include <gtest/gtest.h>
#include <initializer_list>
#include <fermat/algorithm/is_partitioned_test.h>

struct is_odd {
    constexpr bool operator()(const int& i) const { return i & 1; }
};

TEST(IsPartitionedTest, IteratorPair) {
    // not partitioned
    const int a1[] = {1, 2, 3, 4, 5, 6};
    EXPECT_FALSE(ranges::is_partitioned(a1, a1 + 6, is_odd{}));
    // partitioned
    const int a2[] = {1, 3, 5, 2, 4, 6};
    EXPECT_TRUE(ranges::is_partitioned(a2, a2 + 6, is_odd{}));
    // not partitioned (first element wrong)
    const int a3[] = {2, 4, 6, 1, 3, 5};
    EXPECT_FALSE(ranges::is_partitioned(a3, a3 + 6, is_odd{}));
    // partitioned after adding extra element
    const int a4[] = {1, 3, 5, 2, 4, 6, 7};
    EXPECT_FALSE(ranges::is_partitioned(a4, a4 + 7, is_odd{}));
    // empty range
    EXPECT_TRUE(ranges::is_partitioned(a4, a4, is_odd{}));
}

TEST(IsPartitionedTest, Range) {
    // not partitioned
    const int a1[] = {1, 2, 3, 4, 5, 6};
    EXPECT_FALSE(ranges::is_partitioned(a1, is_odd{}));
    // partitioned
    const int a2[] = {1, 3, 5, 2, 4, 6};
    EXPECT_TRUE(ranges::is_partitioned(a2, is_odd{}));
    // not partitioned
    const int a3[] = {2, 4, 6, 1, 3, 5};
    EXPECT_FALSE(ranges::is_partitioned(a3, is_odd{}));
    // not partitioned (violation after)
    const int a4[] = {1, 3, 5, 2, 4, 6, 7};
    EXPECT_FALSE(ranges::is_partitioned(a4, is_odd{}));
    // empty range
    EXPECT_TRUE(ranges::is_partitioned(a4, a4, is_odd{}));
}

TEST(IsPartitionedTest, Projection) {
    struct S { int i; };
    const S arr[] = {S{1}, S{3}, S{5}, S{2}, S{4}, S{6}};
    EXPECT_TRUE(ranges::is_partitioned(arr, is_odd{}, &S::i));
}

TEST(IsPartitionedTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    static_assert(ranges::is_partitioned(IL{1, 3, 5, 2, 4, 6}, is_odd{}), "");
    static_assert(!ranges::is_partitioned(IL{1, 3, 1, 2, 5, 6}, is_odd{}), "");
}
