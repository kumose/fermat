#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/partition_point.h>
#include <fermat/view/subrange.h>

/// predicate for testing
struct is_odd {
    constexpr bool operator()(const int& i) const { return i & 1; }
};

/// test partition_point with iterator pairs (raw pointers)
void test_iter() {
    // all false
    const int a1[] = {2, 4, 6, 8, 10};
    EXPECT_EQ(fermat::ranges::partition_point(a1, a1 + 5, is_odd{}), a1);

    // first true
    const int a2[] = {1, 2, 4, 6, 8};
    EXPECT_EQ(fermat::ranges::partition_point(a2, a2 + 5, is_odd{}), a2 + 1);

    // two true
    const int a3[] = {1, 3, 2, 4, 6};
    EXPECT_EQ(fermat::ranges::partition_point(a3, a3 + 5, is_odd{}), a3 + 2);

    // three true
    const int a4[] = {1, 3, 5, 2, 4, 6};
    EXPECT_EQ(fermat::ranges::partition_point(a4, a4 + 6, is_odd{}), a4 + 3);

    // four true
    const int a5[] = {1, 3, 5, 7, 2, 4};
    EXPECT_EQ(fermat::ranges::partition_point(a5, a5 + 6, is_odd{}), a5 + 4);

    // five true
    const int a6[] = {1, 3, 5, 7, 9, 2};
    EXPECT_EQ(fermat::ranges::partition_point(a6, a6 + 6, is_odd{}), a6 + 5);

    // all true
    const int a7[] = {1, 3, 5, 7, 9, 11};
    EXPECT_EQ(fermat::ranges::partition_point(a7, a7 + 6, is_odd{}), a7 + 6);

    // empty range
    const int a8[] = {1, 3, 5, 2, 4, 6, 7};
    EXPECT_EQ(fermat::ranges::partition_point(a8, a8, is_odd{}), a8);
}

/// test partition_point with range (using subrange)
void test_range() {
    // all false
    const int a1[] = {2, 4, 6, 8, 10};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a1, a1 + 5), is_odd{}), a1);

    // first true
    const int a2[] = {1, 2, 4, 6, 8};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a2, a2 + 5), is_odd{}), a2 + 1);

    // two true
    const int a3[] = {1, 3, 2, 4, 6};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a3, a3 + 5), is_odd{}), a3 + 2);

    // three true
    const int a4[] = {1, 3, 5, 2, 4, 6};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a4, a4 + 6), is_odd{}), a4 + 3);

    // four true
    const int a5[] = {1, 3, 5, 7, 2, 4};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a5, a5 + 6), is_odd{}), a5 + 4);

    // five true
    const int a6[] = {1, 3, 5, 7, 9, 2};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a6, a6 + 6), is_odd{}), a6 + 5);

    // all true
    const int a7[] = {1, 3, 5, 7, 9, 11};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a7, a7 + 6), is_odd{}), a7 + 6);

    // empty range
    const int a8[] = {1, 3, 5, 2, 4, 6, 7};
    EXPECT_EQ(fermat::ranges::partition_point(fermat::ranges::make_subrange(a8, a8), is_odd{}), a8);
}

/// projection test
struct S { int i; };

TEST(PartitionPointTest, IteratorPair) {
    test_iter();
}

TEST(PartitionPointTest, Range) {
    test_range();
}

TEST(PartitionPointTest, Projection) {
    const S arr[] = {S{1}, S{3}, S{5}, S{2}, S{4}, S{6}};
    const S* result = fermat::ranges::partition_point(arr, is_odd{}, &S::i);
    EXPECT_EQ(result, arr + 3);
}

TEST(PartitionPointTest, Constexpr) {
    constexpr std::array<int, 6> a{{1, 3, 5, 2, 4, 6}};
    static_assert(fermat::ranges::partition_point(a, is_odd{}) == a.begin() + 3, "");
}
