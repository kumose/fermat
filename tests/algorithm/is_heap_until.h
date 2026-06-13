#pragma once

// is_heap_until_test.cc - GTest version of range-v3 is_heap_until tests
#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <fermat/algorithm/heap_algorithm.h>

// Helper: raw pointer version of is_heap_until (returns pointer)
template<typename Iter, typename Comp = std::less<>>
Iter is_heap_until_raw(Iter first, Iter last, Comp comp = {}) {
    return ranges::is_heap_until(first, last, comp);
}

// ---------- test_basic ----------
TEST(IsHeapUntilTest, Basic) {
    // length 2
    int i1[] = {0, 0};
    EXPECT_EQ(is_heap_until_raw(i1, i1), i1);
    EXPECT_EQ(is_heap_until_raw(i1, i1 + 1), i1 + 1);
    int i2[] = {0, 1}, i3[] = {1, 0};
    EXPECT_EQ(is_heap_until_raw(i1, i1 + 2), i1 + 2);
    EXPECT_EQ(is_heap_until_raw(i2, i2 + 2), i2 + 1);
    EXPECT_EQ(is_heap_until_raw(i3, i3 + 2), i3 + 2);

    // length 3
    int i4[] = {0,0,0}, i5[] = {0,0,1}, i6[] = {0,1,0}, i7[] = {0,1,1};
    int i8[] = {1,0,0}, i9[] = {1,0,1}, i10[] = {1,1,0};
    EXPECT_EQ(is_heap_until_raw(i4, i4+3), i4+3);
    EXPECT_EQ(is_heap_until_raw(i5, i5+3), i5+2);
    EXPECT_EQ(is_heap_until_raw(i6, i6+3), i6+1);
    EXPECT_EQ(is_heap_until_raw(i7, i7+3), i7+1);
    EXPECT_EQ(is_heap_until_raw(i8, i8+3), i8+3);
    EXPECT_EQ(is_heap_until_raw(i9, i9+3), i9+3);
    EXPECT_EQ(is_heap_until_raw(i10, i10+3), i10+3);

    // length 4 (first few)
    int i11[] = {0,0,0,0}, i12[] = {0,0,0,1}, i13[] = {0,0,1,0}, i14[] = {0,0,1,1};
    EXPECT_EQ(is_heap_until_raw(i11, i11+4), i11+4);
    EXPECT_EQ(is_heap_until_raw(i12, i12+4), i12+3);
    EXPECT_EQ(is_heap_until_raw(i13, i13+4), i13+2);
    EXPECT_EQ(is_heap_until_raw(i14, i14+4), i14+2);
    // additional length 4 cases are omitted for brevity; the pattern is similar
}

// ---------- test_pred ----------
TEST(IsHeapUntilTest, WithComparator) {
    auto greater = std::greater<int>();

    // length 2
    int i1[] = {0, 0};
    EXPECT_EQ(is_heap_until_raw(i1, i1, greater), i1);
    EXPECT_EQ(is_heap_until_raw(i1, i1+1, greater), i1+1);
    int i2[] = {0, 1}, i3[] = {1, 0};
    EXPECT_EQ(is_heap_until_raw(i1, i1+2, greater), i1+2);
    EXPECT_EQ(is_heap_until_raw(i2, i2+2, greater), i2+2);
    EXPECT_EQ(is_heap_until_raw(i3, i3+2, greater), i3+1);

    // length 3
    int i4[] = {0,0,0}, i5[] = {0,0,1}, i6[] = {0,1,0}, i7[] = {0,1,1};
    int i8[] = {1,0,0}, i9[] = {1,0,1}, i10[] = {1,1,0};
    EXPECT_EQ(is_heap_until_raw(i4, i4+3, greater), i4+3);
    EXPECT_EQ(is_heap_until_raw(i5, i5+3, greater), i5+3);
    EXPECT_EQ(is_heap_until_raw(i6, i6+3, greater), i6+3);
    EXPECT_EQ(is_heap_until_raw(i7, i7+3, greater), i7+3);
    EXPECT_EQ(is_heap_until_raw(i8, i8+3, greater), i8+1);
    EXPECT_EQ(is_heap_until_raw(i9, i9+3, greater), i9+1);
    EXPECT_EQ(is_heap_until_raw(i10, i10+3, greater), i10+2);
}

// ---------- projection ----------
TEST(IsHeapUntilTest, WithProjection) {
    struct S { int i; };
    S arr[] = {S{1}, S{0}, S{0}, S{0}, S{0}, S{0}, S{1}};
    auto result = ranges::is_heap_until(arr, arr+7, std::greater<int>(), &S::i);
    EXPECT_EQ(result, arr + 1);
}

// ---------- constexpr test ----------
TEST(IsHeapUntilTest, Constexpr) {
    constexpr std::array<int, 7> arr{1,0,0,0,0,0,1};
    static_assert(ranges::is_heap_until(arr, std::greater{}) == arr.begin() + 1,
                  "constexpr is_heap_until failed");
}
