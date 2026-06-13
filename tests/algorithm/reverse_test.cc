#include <gtest/gtest.h>
#include <fermat/algorithm/reverse.h>
#include <fermat/view/subrange.h>

/// test reverse with iterator pairs (raw pointers)
void test_reverse_iter() {
    // empty range
    int a[] = {0};
    auto i0 = ranges::reverse(a, a);
    EXPECT_EQ(i0, a);
    EXPECT_EQ(a[0], 0);

    // single element
    auto i1 = ranges::reverse(a, a + 1);
    EXPECT_EQ(i1, a + 1);
    EXPECT_EQ(a[0], 0);

    // two elements
    int b[] = {0, 1};
    auto i2 = ranges::reverse(b, b + 2);
    EXPECT_EQ(i2, b + 2);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 0);

    // three elements
    int c[] = {0, 1, 2};
    auto i3 = ranges::reverse(c, c + 3);
    EXPECT_EQ(i3, c + 3);
    EXPECT_EQ(c[0], 2);
    EXPECT_EQ(c[1], 1);
    EXPECT_EQ(c[2], 0);

    // four elements
    int d[] = {0, 1, 2, 3};
    auto i4 = ranges::reverse(d, d + 4);
    EXPECT_EQ(i4, d + 4);
    EXPECT_EQ(d[0], 3);
    EXPECT_EQ(d[1], 2);
    EXPECT_EQ(d[2], 1);
    EXPECT_EQ(d[3], 0);
}

/// test reverse with range (subrange)
void test_reverse_range() {
    // empty range
    int a[] = {0};
    auto r0 = ranges::reverse(ranges::make_subrange(a, a));
    EXPECT_EQ(r0, a);
    EXPECT_EQ(a[0], 0);

    // single element
    auto r1 = ranges::reverse(ranges::make_subrange(a, a + 1));
    EXPECT_EQ(r1, a + 1);
    EXPECT_EQ(a[0], 0);

    // two elements
    int b[] = {0, 1};
    auto r2 = ranges::reverse(ranges::make_subrange(b, b + 2));
    EXPECT_EQ(r2, b + 2);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 0);

    // three elements
    int c[] = {0, 1, 2};
    auto r3 = ranges::reverse(ranges::make_subrange(c, c + 3));
    EXPECT_EQ(r3, c + 3);
    EXPECT_EQ(c[0], 2);
    EXPECT_EQ(c[1], 1);
    EXPECT_EQ(c[2], 0);

    // four elements
    int d[] = {0, 1, 2, 3};
    auto r4 = ranges::reverse(ranges::make_subrange(d, d + 4));
    EXPECT_EQ(r4, d + 4);
    EXPECT_EQ(d[0], 3);
    EXPECT_EQ(d[1], 2);
    EXPECT_EQ(d[2], 1);
    EXPECT_EQ(d[3], 0);

    // rvalue range (same effect, modifies underlying array)
    int e[] = {0, 1, 2, 3};
    auto r5 = ranges::reverse(ranges::make_subrange(e, e + 4));
    EXPECT_EQ(r5, e + 4);
    EXPECT_EQ(e[0], 3);
    EXPECT_EQ(e[1], 2);
    EXPECT_EQ(e[2], 1);
    EXPECT_EQ(e[3], 0);
}

TEST(ReverseTest, IteratorPair) {
    test_reverse_iter();
}

TEST(ReverseTest, Range) {
    test_reverse_range();
}

/// constexpr test (compile‑time)
TEST(ReverseTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int arr[] = {0, 1, 2, 3, 4};
        constexpr std::size_t N = sizeof(arr) / sizeof(arr[0]);
        int* r = ranges::reverse(arr);
        bool ok = (r == arr + N) &&
                  (arr[0] == 4) && (arr[1] == 3) &&
                  (arr[2] == 2) && (arr[3] == 1) && (arr[4] == 0);
        return ok;
    };
    static_assert(test(), "");
}
