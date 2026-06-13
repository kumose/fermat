#include <gtest/gtest.h>
#include <initializer_list>
#include <fermat/algorithm/is_sorted.h>

TEST(IsSortedTest, Basic) {
    // empty range
    int empty[] = {};
    EXPECT_TRUE(ranges::is_sorted(empty, empty));
    // single element
    int a1[] = {0};
    EXPECT_TRUE(ranges::is_sorted(a1, a1 + 1));
    // length 2
    int a2[] = {0, 0};
    EXPECT_TRUE(ranges::is_sorted(a2, a2 + 2));
    int a3[] = {0, 1};
    EXPECT_TRUE(ranges::is_sorted(a3, a3 + 2));
    int a4[] = {1, 0};
    EXPECT_FALSE(ranges::is_sorted(a4, a4 + 2));
    int a5[] = {1, 1};
    EXPECT_TRUE(ranges::is_sorted(a5, a5 + 2));

    // length 3
    int b1[] = {0,0,0};
    EXPECT_TRUE(ranges::is_sorted(b1, b1+3));
    int b2[] = {0,0,1};
    EXPECT_TRUE(ranges::is_sorted(b2, b2+3));
    int b3[] = {0,1,0};
    EXPECT_FALSE(ranges::is_sorted(b3, b3+3));
    int b4[] = {0,1,1};
    EXPECT_TRUE(ranges::is_sorted(b4, b4+3));
    int b5[] = {1,0,0};
    EXPECT_FALSE(ranges::is_sorted(b5, b5+3));
    int b6[] = {1,0,1};
    EXPECT_FALSE(ranges::is_sorted(b6, b6+3));
    int b7[] = {1,1,0};
    EXPECT_FALSE(ranges::is_sorted(b7, b7+3));
    int b8[] = {1,1,1};
    EXPECT_TRUE(ranges::is_sorted(b8, b8+3));

    // length 4 (a few representatives)
    int c1[] = {0,0,0,0};
    EXPECT_TRUE(ranges::is_sorted(c1, c1+4));
    int c2[] = {0,0,0,1};
    EXPECT_TRUE(ranges::is_sorted(c2, c2+4));
    int c3[] = {0,0,1,0};
    EXPECT_FALSE(ranges::is_sorted(c3, c3+4));
    int c4[] = {0,0,1,1};
    EXPECT_TRUE(ranges::is_sorted(c4, c4+4));
    int c5[] = {1,1,1,0};
    EXPECT_FALSE(ranges::is_sorted(c5, c5+4));
    int c6[] = {1,1,1,1};
    EXPECT_TRUE(ranges::is_sorted(c6, c6+4));
}

TEST(IsSortedTest, WithComparator) {
    int a1[] = {0};
    EXPECT_TRUE(ranges::is_sorted(a1, a1+1, std::greater<int>()));

    int a2[] = {0,0};
    EXPECT_TRUE(ranges::is_sorted(a2, a2+2, std::greater<int>()));
    int a3[] = {0,1};
    EXPECT_FALSE(ranges::is_sorted(a3, a3+2, std::greater<int>()));
    int a4[] = {1,0};
    EXPECT_TRUE(ranges::is_sorted(a4, a4+2, std::greater<int>()));
    int a5[] = {1,1};
    EXPECT_TRUE(ranges::is_sorted(a5, a5+2, std::greater<int>()));

    int b1[] = {0,0,0};
    EXPECT_TRUE(ranges::is_sorted(b1, b1+3, std::greater<int>()));
    int b2[] = {0,0,1};
    EXPECT_FALSE(ranges::is_sorted(b2, b2+3, std::greater<int>()));
    int b3[] = {0,1,0};
    EXPECT_FALSE(ranges::is_sorted(b3, b3+3, std::greater<int>()));
    int b4[] = {0,1,1};
    EXPECT_FALSE(ranges::is_sorted(b4, b4+3, std::greater<int>()));
    int b5[] = {1,0,0};
    EXPECT_TRUE(ranges::is_sorted(b5, b5+3, std::greater<int>()));
    int b6[] = {1,0,1};
    EXPECT_FALSE(ranges::is_sorted(b6, b6+3, std::greater<int>()));
    int b7[] = {1,1,0};
    EXPECT_TRUE(ranges::is_sorted(b7, b7+3, std::greater<int>()));
    int b8[] = {1,1,1};
    EXPECT_TRUE(ranges::is_sorted(b8, b8+3, std::greater<int>()));

    int c1[] = {1,0,0,0};
    EXPECT_TRUE(ranges::is_sorted(c1, c1+4, std::greater<int>()));
    int c2[] = {1,1,0,0};
    EXPECT_TRUE(ranges::is_sorted(c2, c2+4, std::greater<int>()));
    int c3[] = {1,1,1,0};
    EXPECT_TRUE(ranges::is_sorted(c3, c3+4, std::greater<int>()));
    int c4[] = {1,1,1,1};
    EXPECT_TRUE(ranges::is_sorted(c4, c4+4, std::greater<int>()));
    int c5[] = {0,0,0,1};
    EXPECT_FALSE(ranges::is_sorted(c5, c5+4, std::greater<int>()));
}

TEST(IsSortedTest, Projection) {
    struct A { int a; };
    A arr[] = {{0},{1},{2},{3},{4}};
    EXPECT_TRUE(ranges::is_sorted(arr, std::less<int>(), &A::a));
    EXPECT_FALSE(ranges::is_sorted(arr, std::greater<int>(), &A::a));
}

TEST(IsSortedTest, Constexpr) {
    using IL = std::initializer_list<int>;
    static_assert(ranges::is_sorted(IL{0, 1, 2, 3}), "");
    static_assert(ranges::is_sorted(IL{0, 1, 2, 3}, std::less<>{}), "");
    static_assert(!ranges::is_sorted(IL{3, 2, 1, 0}), "");
    static_assert(ranges::is_sorted(IL{3, 2, 1, 0}, std::greater<>{}), "");
}
