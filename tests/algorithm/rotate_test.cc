#include <gtest/gtest.h>
#include <vector>
#include <fermat/algorithm/rotate.h>
#include <fermat/view/all.h>

/// test rotate with iterator pairs (raw pointers)
void test_rotate_iter() {
    // 0 elements (empty)
    int a[] = {0};
    auto r = ranges::rotate(a, a, a);
    EXPECT_EQ(r.begin(), a);
    EXPECT_EQ(r.end(), a);
    EXPECT_EQ(a[0], 0);

    // single element, rotate by 0
    r = ranges::rotate(a, a, a + 1);
    EXPECT_EQ(r.begin(), a + 1);
    EXPECT_EQ(r.end(), a + 1);
    EXPECT_EQ(a[0], 0);

    // single element, rotate by 1 (same as end)
    r = ranges::rotate(a, a + 1, a + 1);
    EXPECT_EQ(r.begin(), a);
    EXPECT_EQ(r.end(), a + 1);
    EXPECT_EQ(a[0], 0);

    // 2 elements
    int b[] = {0, 1};
    // rotate by 0
    r = ranges::rotate(b, b, b + 2);
    EXPECT_EQ(r.begin(), b + 2);
    EXPECT_EQ(r.end(), b + 2);
    EXPECT_EQ(b[0], 0);
    EXPECT_EQ(b[1], 1);
    // rotate by 1
    r = ranges::rotate(b, b + 1, b + 2);
    EXPECT_EQ(r.begin(), b + 1);
    EXPECT_EQ(r.end(), b + 2);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 0);
    // rotate by 2 (full cycle, same as end)
    r = ranges::rotate(b, b + 2, b + 2);
    EXPECT_EQ(r.begin(), b);
    EXPECT_EQ(r.end(), b + 2);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 0);

    // 3 elements
    int c[] = {0, 1, 2};
    r = ranges::rotate(c, c, c + 3);
    EXPECT_EQ(r.begin(), c + 3);
    EXPECT_EQ(r.end(), c + 3);
    EXPECT_EQ(c[0], 0);
    EXPECT_EQ(c[1], 1);
    EXPECT_EQ(c[2], 2);
    // rotate by 1
    r = ranges::rotate(c, c + 1, c + 3);
    EXPECT_EQ(r.begin(), c + 2);
    EXPECT_EQ(r.end(), c + 3);
    EXPECT_EQ(c[0], 1);
    EXPECT_EQ(c[1], 2);
    EXPECT_EQ(c[2], 0);
    // rotate by 2
    r = ranges::rotate(c, c + 2, c + 3);
    EXPECT_EQ(r.begin(), c + 1);
    EXPECT_EQ(r.end(), c + 3);
    EXPECT_EQ(c[0], 0);
    EXPECT_EQ(c[1], 1);
    EXPECT_EQ(c[2], 2);
    // rotate by 3 (full)
    r = ranges::rotate(c, c + 3, c + 3);
    EXPECT_EQ(r.begin(), c);
    EXPECT_EQ(r.end(), c + 3);
    EXPECT_EQ(c[0], 0);
    EXPECT_EQ(c[1], 1);
    EXPECT_EQ(c[2], 2);

    // 4 elements
    int d[] = {0, 1, 2, 3};
    r = ranges::rotate(d, d, d + 4);
    EXPECT_EQ(r.begin(), d + 4);
    EXPECT_EQ(r.end(), d + 4);
    EXPECT_EQ(d[0], 0);
    EXPECT_EQ(d[1], 1);
    EXPECT_EQ(d[2], 2);
    EXPECT_EQ(d[3], 3);
    // rotate by 1
    r = ranges::rotate(d, d + 1, d + 4);
    EXPECT_EQ(r.begin(), d + 3);
    EXPECT_EQ(r.end(), d + 4);
    EXPECT_EQ(d[0], 1);
    EXPECT_EQ(d[1], 2);
    EXPECT_EQ(d[2], 3);
    EXPECT_EQ(d[3], 0);
    // rotate by 2
    r = ranges::rotate(d, d + 2, d + 4);
    EXPECT_EQ(r.begin(), d + 2);
    EXPECT_EQ(r.end(), d + 4);
    EXPECT_EQ(d[0], 3);
    EXPECT_EQ(d[1], 0);
    EXPECT_EQ(d[2], 1);
    EXPECT_EQ(d[3], 2);
    // rotate by 3
    r = ranges::rotate(d, d + 3, d + 4);
    EXPECT_EQ(r.begin(), d + 1);
    EXPECT_EQ(r.end(), d + 4);
    EXPECT_EQ(d[0], 2);
    EXPECT_EQ(d[1], 3);
    EXPECT_EQ(d[2], 0);
    EXPECT_EQ(d[3], 1);
    // rotate by 4 (full)
    r = ranges::rotate(d, d + 4, d + 4);
    EXPECT_EQ(r.begin(), d);
    EXPECT_EQ(r.end(), d + 4);
    EXPECT_EQ(d[0], 2);
    EXPECT_EQ(d[1], 3);
    EXPECT_EQ(d[2], 0);
    EXPECT_EQ(d[3], 1);

    // 5 elements
    int e[] = {0, 1, 2, 3, 4};
    r = ranges::rotate(e, e, e + 5);
    EXPECT_EQ(r.begin(), e + 5);
    EXPECT_EQ(r.end(), e + 5);
    EXPECT_EQ(e[0], 0);
    EXPECT_EQ(e[1], 1);
    EXPECT_EQ(e[2], 2);
    EXPECT_EQ(e[3], 3);
    EXPECT_EQ(e[4], 4);
    // rotate by 1
    r = ranges::rotate(e, e + 1, e + 5);
    EXPECT_EQ(r.begin(), e + 4);
    EXPECT_EQ(r.end(), e + 5);
    EXPECT_EQ(e[0], 1);
    EXPECT_EQ(e[1], 2);
    EXPECT_EQ(e[2], 3);
    EXPECT_EQ(e[3], 4);
    EXPECT_EQ(e[4], 0);
    // rotate by 2
    r = ranges::rotate(e, e + 2, e + 5);
    EXPECT_EQ(r.begin(), e + 3);
    EXPECT_EQ(r.end(), e + 5);
    EXPECT_EQ(e[0], 3);
    EXPECT_EQ(e[1], 4);
    EXPECT_EQ(e[2], 0);
    EXPECT_EQ(e[3], 1);
    EXPECT_EQ(e[4], 2);
    // rotate by 3
    r = ranges::rotate(e, e + 3, e + 5);
    EXPECT_EQ(r.begin(), e + 2);
    EXPECT_EQ(r.end(), e + 5);
    EXPECT_EQ(e[0], 1);
    EXPECT_EQ(e[1], 2);
    EXPECT_EQ(e[2], 3);
    EXPECT_EQ(e[3], 4);
    EXPECT_EQ(e[4], 0);
    // rotate by 4
    r = ranges::rotate(e, e + 4, e + 5);
    EXPECT_EQ(r.begin(), e + 1);
    EXPECT_EQ(r.end(), e + 5);
    EXPECT_EQ(e[0], 0);
    EXPECT_EQ(e[1], 1);
    EXPECT_EQ(e[2], 2);
    EXPECT_EQ(e[3], 3);
    EXPECT_EQ(e[4], 4);
    // rotate by 5 (full)
    r = ranges::rotate(e, e + 5, e + 5);
    EXPECT_EQ(r.begin(), e);
    EXPECT_EQ(r.end(), e + 5);
    EXPECT_EQ(e[0], 0);
    EXPECT_EQ(e[1], 1);
    EXPECT_EQ(e[2], 2);
    EXPECT_EQ(e[3], 3);
    EXPECT_EQ(e[4], 4);

    // 6 elements (a sample)
    int f[] = {0, 1, 2, 3, 4, 5};
    r = ranges::rotate(f, f, f + 6);
    EXPECT_EQ(r.begin(), f + 6);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 0);
    EXPECT_EQ(f[1], 1);
    EXPECT_EQ(f[2], 2);
    EXPECT_EQ(f[3], 3);
    EXPECT_EQ(f[4], 4);
    EXPECT_EQ(f[5], 5);
    // rotate by 1
    r = ranges::rotate(f, f + 1, f + 6);
    EXPECT_EQ(r.begin(), f + 5);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 1);
    EXPECT_EQ(f[1], 2);
    EXPECT_EQ(f[2], 3);
    EXPECT_EQ(f[3], 4);
    EXPECT_EQ(f[4], 5);
    EXPECT_EQ(f[5], 0);
    // rotate by 2
    r = ranges::rotate(f, f + 2, f + 6);
    EXPECT_EQ(r.begin(), f + 4);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 3);
    EXPECT_EQ(f[1], 4);
    EXPECT_EQ(f[2], 5);
    EXPECT_EQ(f[3], 0);
    EXPECT_EQ(f[4], 1);
    EXPECT_EQ(f[5], 2);
    // rotate by 3
    r = ranges::rotate(f, f + 3, f + 6);
    EXPECT_EQ(r.begin(), f + 3);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 0);
    EXPECT_EQ(f[1], 1);
    EXPECT_EQ(f[2], 2);
    EXPECT_EQ(f[3], 3);
    EXPECT_EQ(f[4], 4);
    EXPECT_EQ(f[5], 5);
    // rotate by 4
    r = ranges::rotate(f, f + 4, f + 6);
    EXPECT_EQ(r.begin(), f + 2);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 4);
    EXPECT_EQ(f[1], 5);
    EXPECT_EQ(f[2], 0);
    EXPECT_EQ(f[3], 1);
    EXPECT_EQ(f[4], 2);
    EXPECT_EQ(f[5], 3);
    // rotate by 5
    r = ranges::rotate(f, f + 5, f + 6);
    EXPECT_EQ(r.begin(), f + 1);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 3);
    EXPECT_EQ(f[1], 4);
    EXPECT_EQ(f[2], 5);
    EXPECT_EQ(f[3], 0);
    EXPECT_EQ(f[4], 1);
    EXPECT_EQ(f[5], 2);
    // rotate by 6 (full)
    r = ranges::rotate(f, f + 6, f + 6);
    EXPECT_EQ(r.begin(), f);
    EXPECT_EQ(r.end(), f + 6);
    EXPECT_EQ(f[0], 3);
    EXPECT_EQ(f[1], 4);
    EXPECT_EQ(f[2], 5);
    EXPECT_EQ(f[3], 0);
    EXPECT_EQ(f[4], 1);
    EXPECT_EQ(f[5], 2);
}

/// test rotate with range (subrange)
void test_rotate_range() {
    // 3 elements (example)
    int arr[] = {0, 1, 2};
    auto rng = ranges::make_subrange(arr, arr + 3);
    // rotate by 0
    auto r = ranges::rotate(rng, arr);
    EXPECT_EQ(r.begin(), arr + 3);
    EXPECT_EQ(r.end(), arr + 3);
    EXPECT_EQ(arr[0], 0);
    EXPECT_EQ(arr[1], 1);
    EXPECT_EQ(arr[2], 2);
    // rotate by 1
    r = ranges::rotate(rng, arr + 1);
    EXPECT_EQ(r.begin(), arr + 2);
    EXPECT_EQ(r.end(), arr + 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 0);
    // rotate by 2
    r = ranges::rotate(rng, arr + 2);
    EXPECT_EQ(r.begin(), arr + 1);
    EXPECT_EQ(r.end(), arr + 3);
    EXPECT_EQ(arr[0], 0);
    EXPECT_EQ(arr[1], 1);
    EXPECT_EQ(arr[2], 2);
}

TEST(RotateTest, IteratorPair) {
    test_rotate_iter();
}

TEST(RotateTest, Range) {
    test_rotate_range();
}

/// test rvalue ranges (just verify that the operation compiles and modifies the original array)
TEST(RotateTest, RvalueRange) {
    int arr[] = {0, 1, 2, 3, 4, 5};
    // lvalue view
    auto r = ranges::rotate(ranges::views::all(arr), arr + 2);
    EXPECT_EQ(r.begin(), arr + 4);
    EXPECT_EQ(r.end(), arr + 6);
    EXPECT_EQ(arr[0], 2);
    EXPECT_EQ(arr[1], 3);
    EXPECT_EQ(arr[2], 4);
    EXPECT_EQ(arr[3], 5);
    EXPECT_EQ(arr[4], 0);
    EXPECT_EQ(arr[5], 1);

    // rvalue range (same array, but we just test that it compiles)
    int arr2[] = {0, 1, 2, 3, 4, 5};
    auto r2 = ranges::rotate(std::move(arr2), arr2 + 2);
    // r2 is dangling, but the array should be rotated
    EXPECT_EQ(arr2[0], 2);
    EXPECT_EQ(arr2[1], 3);
    EXPECT_EQ(arr2[2], 4);
    EXPECT_EQ(arr2[3], 5);
    EXPECT_EQ(arr2[4], 0);
    EXPECT_EQ(arr2[5], 1);

    // with vector
    std::vector<int> vec{0, 1, 2, 3, 4, 5};
    auto r3 = ranges::rotate(std::move(vec), vec.begin() + 2);
    (void)r3;   // dangling
    EXPECT_EQ(vec[0], 2);
    EXPECT_EQ(vec[1], 3);
    EXPECT_EQ(vec[2], 4);
    EXPECT_EQ(vec[3], 5);
    EXPECT_EQ(vec[4], 0);
    EXPECT_EQ(vec[5], 1);
}

/// constexpr test (compile‑time)
TEST(RotateTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int arr[] = {0, 1, 2, 3, 4, 5};
        auto r = ranges::rotate(arr, arr + 2);
        bool ok = (r.begin() == arr + 4) &&
                  (r.end() == arr + 6) &&
                  (arr[0] == 2) && (arr[1] == 3) &&
                  (arr[2] == 4) && (arr[3] == 5) &&
                  (arr[4] == 0) && (arr[5] == 1);
        return ok;
    };
    static_assert(test(), "");
}
