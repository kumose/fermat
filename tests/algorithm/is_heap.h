#pragma once

#include <gtest/gtest.h>
#include <fermat/algorithm/heap_algorithm.h>

TEST(IsHeapTest, Basic) {
    // length 2
    int i1[] = {0, 0};
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1));       ///< empty range
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1 + 1));  ///< single element
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1 + 2) == (std::is_heap(i1, i1 + 2)));

    int i2[] = {0, 1};
    int i3[] = {1, 0};
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1 + 2) == (std::is_heap(i1, i1 + 2)));
    EXPECT_TRUE(fermat::ranges::is_heap(i2, i2 + 2) == (std::is_heap(i2, i2 + 2)));
    EXPECT_TRUE(fermat::ranges::is_heap(i3, i3 + 2) == (std::is_heap(i3, i3 + 2)));

    // length 3
    int i4[] = {0,0,0}, i5[] = {0,0,1}, i6[] = {0,1,0}, i7[] = {0,1,1};
    int i8[] = {1,0,0}, i9[] = {1,0,1}, i10[] = {1,1,0};
    EXPECT_TRUE(fermat::ranges::is_heap(i4, i4+3) == std::is_heap(i4, i4+3));
    EXPECT_TRUE(fermat::ranges::is_heap(i5, i5+3) == std::is_heap(i5, i5+3));
    EXPECT_TRUE(fermat::ranges::is_heap(i6, i6+3) == std::is_heap(i6, i6+3));
    EXPECT_TRUE(fermat::ranges::is_heap(i7, i7+3) == std::is_heap(i7, i7+3));
    EXPECT_TRUE(fermat::ranges::is_heap(i8, i8+3) == std::is_heap(i8, i8+3));
    EXPECT_TRUE(fermat::ranges::is_heap(i9, i9+3) == std::is_heap(i9, i9+3));
    EXPECT_TRUE(fermat::ranges::is_heap(i10, i10+3) == std::is_heap(i10, i10+3));

    // length 4 (first few)
    int i11[] = {0,0,0,0};
    int i12[] = {0,0,0,1};
    int i13[] = {0,0,1,0};
    int i14[] = {0,0,1,1};
    EXPECT_TRUE(fermat::ranges::is_heap(i11, i11+4) == std::is_heap(i11, i11+4));
    EXPECT_TRUE(fermat::ranges::is_heap(i12, i12+4) == std::is_heap(i12, i12+4));
    EXPECT_TRUE(fermat::ranges::is_heap(i13, i13+4) == std::is_heap(i13, i13+4));
    EXPECT_TRUE(fermat::ranges::is_heap(i14, i14+4) == std::is_heap(i14, i14+4));
}

TEST(IsHeapTest, WithComparator) {
    int i1[] = {0, 0};
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1+1, std::greater<int>()));

    int i2[] = {0, 1};
    int i3[] = {1, 0};
    EXPECT_TRUE(fermat::ranges::is_heap(i1, i1+2, std::greater<int>()) == std::is_heap(i1, i1+2, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i2, i2+2, std::greater<int>()) == std::is_heap(i2, i2+2, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i3, i3+2, std::greater<int>()) == std::is_heap(i3, i3+2, std::greater<int>()));

    // length 3 with comparator
    int i4[] = {0,0,0}, i5[] = {0,0,1}, i6[] = {0,1,0}, i7[] = {0,1,1};
    int i8[] = {1,0,0}, i9[] = {1,0,1}, i10[] = {1,1,0};
    EXPECT_TRUE(fermat::ranges::is_heap(i4, i4+3, std::greater<int>()) == std::is_heap(i4, i4+3, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i5, i5+3, std::greater<int>()) == std::is_heap(i5, i5+3, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i6, i6+3, std::greater<int>()) == std::is_heap(i6, i6+3, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i7, i7+3, std::greater<int>()) == std::is_heap(i7, i7+3, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i8, i8+3, std::greater<int>()) == std::is_heap(i8, i8+3, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i9, i9+3, std::greater<int>()) == std::is_heap(i9, i9+3, std::greater<int>()));
    EXPECT_TRUE(fermat::ranges::is_heap(i10, i10+3, std::greater<int>()) == std::is_heap(i10, i10+3, std::greater<int>()));
}

TEST(IsHeapTest, WithProjection) {
    struct S { int i; };
    S arr[] = {S{0}, S{1}, S{1}, S{1}, S{1}, S{1}, S{1}};
    EXPECT_TRUE(fermat::ranges::is_heap(arr, arr+7, std::greater<int>(), &S::i));
}

TEST(IsHeapTest, Constexpr) {
    using IL = std::initializer_list<int>;
    static_assert(fermat::ranges::is_heap(IL{0, 1, 1, 1, 1, 1, 1}, fermat::ranges::greater{}), "");
}
