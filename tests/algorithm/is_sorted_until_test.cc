#include <gtest/gtest.h>
#include <initializer_list>
#include <fermat/algorithm/is_sorted_until.h>

TEST(IsSortedUntilTest, Basic) {
    // empty range
    int empty[] = {};
    EXPECT_EQ(ranges::is_sorted_until(empty, empty), empty);
    // single element
    int a1[] = {0};
    EXPECT_EQ(ranges::is_sorted_until(a1, a1 + 1), a1 + 1);
    // length 2
    int a2[] = {0, 0};
    EXPECT_EQ(ranges::is_sorted_until(a2, a2 + 2), a2 + 2);
    int a3[] = {0, 1};
    EXPECT_EQ(ranges::is_sorted_until(a3, a3 + 2), a3 + 2);
    int a4[] = {1, 0};
    EXPECT_EQ(ranges::is_sorted_until(a4, a4 + 2), a4 + 1);
    int a5[] = {1, 1};
    EXPECT_EQ(ranges::is_sorted_until(a5, a5 + 2), a5 + 2);

    // length 3
    int b1[] = {0,0,0};
    EXPECT_EQ(ranges::is_sorted_until(b1, b1+3), b1+3);
    int b2[] = {0,0,1};
    EXPECT_EQ(ranges::is_sorted_until(b2, b2+3), b2+3);
    int b3[] = {0,1,0};
    EXPECT_EQ(ranges::is_sorted_until(b3, b3+3), b3+2);
    int b4[] = {0,1,1};
    EXPECT_EQ(ranges::is_sorted_until(b4, b4+3), b4+3);
    int b5[] = {1,0,0};
    EXPECT_EQ(ranges::is_sorted_until(b5, b5+3), b5+1);
    int b6[] = {1,0,1};
    EXPECT_EQ(ranges::is_sorted_until(b6, b6+3), b6+1);
    int b7[] = {1,1,0};
    EXPECT_EQ(ranges::is_sorted_until(b7, b7+3), b7+2);
    int b8[] = {1,1,1};
    EXPECT_EQ(ranges::is_sorted_until(b8, b8+3), b8+3);

    // length 4 (representative)
    int c1[] = {0,0,0,0};
    EXPECT_EQ(ranges::is_sorted_until(c1, c1+4), c1+4);
    int c2[] = {0,0,0,1};
    EXPECT_EQ(ranges::is_sorted_until(c2, c2+4), c2+4);
    int c3[] = {0,0,1,0};
    EXPECT_EQ(ranges::is_sorted_until(c3, c3+4), c3+3);
    int c4[] = {1,1,1,0};
    EXPECT_EQ(ranges::is_sorted_until(c4, c4+4), c4+3);
    int c5[] = {1,1,1,1};
    EXPECT_EQ(ranges::is_sorted_until(c5, c5+4), c5+4);
}

TEST(IsSortedUntilTest, WithComparator) {
    // greater comparator
    int a1[] = {0};
    EXPECT_EQ(ranges::is_sorted_until(a1, a1+1, std::greater<int>()), a1+1);
    int a2[] = {0,0};
    EXPECT_EQ(ranges::is_sorted_until(a2, a2+2, std::greater<int>()), a2+2);
    int a3[] = {0,1};
    EXPECT_EQ(ranges::is_sorted_until(a3, a3+2, std::greater<int>()), a3+1);
    int a4[] = {1,0};
    EXPECT_EQ(ranges::is_sorted_until(a4, a4+2, std::greater<int>()), a4+2);
    int a5[] = {1,1};
    EXPECT_EQ(ranges::is_sorted_until(a5, a5+2, std::greater<int>()), a5+2);

    int b1[] = {0,0,0};
    EXPECT_EQ(ranges::is_sorted_until(b1, b1+3, std::greater<int>()), b1+3);
    int b2[] = {0,0,1};
    EXPECT_EQ(ranges::is_sorted_until(b2, b2+3, std::greater<int>()), b2+2);
    int b3[] = {0,1,0};
    EXPECT_EQ(ranges::is_sorted_until(b3, b3+3, std::greater<int>()), b3+1);
    int b4[] = {1,0,0};
    EXPECT_EQ(ranges::is_sorted_until(b4, b4+3, std::greater<int>()), b4+3);
    int b5[] = {1,0,1};
    EXPECT_EQ(ranges::is_sorted_until(b5, b5+3, std::greater<int>()), b5+2);
    int b6[] = {1,1,0};
    EXPECT_EQ(ranges::is_sorted_until(b6, b6+3, std::greater<int>()), b6+3);
}

TEST(IsSortedUntilTest, Projection) {
    struct A { int a; };
    A arr[] = {{0},{1},{2},{3},{4}};
    EXPECT_EQ(ranges::is_sorted_until(arr, std::less<int>(), &A::a), arr + 5);
    EXPECT_EQ(ranges::is_sorted_until(arr, std::greater<int>(), &A::a), arr + 1);
}

TEST(IsSortedUntilTest, InitializerList) {
    std::initializer_list<int> il = {0,1,2,3,4,5,6,7,8,9,10};
    EXPECT_EQ(ranges::is_sorted_until(il), il.end());
}

TEST(IsSortedUntilTest, Constexpr) {
    constexpr int arr[] = {1,2,3,4};
    static_assert(ranges::is_sorted_until(arr) == arr + 4, "");
    static_assert(ranges::is_sorted_until(arr, std::less<int>{}) == arr + 4, "");
    static_assert(ranges::is_sorted_until(arr, std::greater<int>{}) == arr + 1, "");
}
