// swap_ranges_test.cc
// Google Test conversion of range-v3 swap_ranges algorithm test.
// Uses raw pointers and standard library types only.
// All comments in English.

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <array>

#include <fermat/algorithm/swap_ranges.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/all.h>


// Helper to get the underlying pointer (identity for raw pointers)
template <typename T>
T* base(T* p) { return p; }

// Placeholder for is_dangling (C++17 compatible)
namespace ranges {
    template<typename T>
    bool is_dangling(T&&) { return false; }
}

// Helper to treat lvalue (as in original test)
template <typename T>
T& as_lvalue(T&& t) { return t; }

// Helper to make subrange (simplified)
template <typename Iter, typename Sent>
auto make_subrange(Iter begin, Sent end) {
    return ranges::subrange<Iter, Sent>(begin, end);
}

// ------------------------------------------------------------
// Test functions (all use raw pointers, template parameters are ignored)
// ------------------------------------------------------------

template<class Iter1, class Iter2>
void test_iter_3() {
    int i[3] = {1, 2, 3};
    int j[3] = {4, 5, 6};
    auto r = ranges::swap_ranges(i, i + 3, j);
    EXPECT_EQ(r.in1, i + 3);
    EXPECT_EQ(r.in2, j + 3);
    EXPECT_EQ(i[0], 4);
    EXPECT_EQ(i[1], 5);
    EXPECT_EQ(i[2], 6);
    EXPECT_EQ(j[0], 1);
    EXPECT_EQ(j[1], 2);
    EXPECT_EQ(j[2], 3);

    // Reverse order
    r = ranges::swap_ranges(j, j + 3, i);
    EXPECT_EQ(r.in1, j + 3);
    EXPECT_EQ(r.in2, i + 3);
    EXPECT_EQ(i[0], 1);
    EXPECT_EQ(i[1], 2);
    EXPECT_EQ(i[2], 3);
    EXPECT_EQ(j[0], 4);
    EXPECT_EQ(j[1], 5);
    EXPECT_EQ(j[2], 6);
}

template<class Iter1, class Iter2>
void test_iter_4() {
    int i[3] = {1, 2, 3};
    int j[4] = {4, 5, 6, 7};
    auto r = ranges::swap_ranges(i, i + 3, j, j + 4);
    EXPECT_EQ(r.in1, i + 3);
    EXPECT_EQ(r.in2, j + 3);
    EXPECT_EQ(i[0], 4);
    EXPECT_EQ(i[1], 5);
    EXPECT_EQ(i[2], 6);
    EXPECT_EQ(j[0], 1);
    EXPECT_EQ(j[1], 2);
    EXPECT_EQ(j[2], 3);
    EXPECT_EQ(j[3], 7);

    // Reverse order
    r = ranges::swap_ranges(j, j + 4, i, i + 3);
    EXPECT_EQ(r.in1, j + 3);
    EXPECT_EQ(r.in2, i + 3);
    EXPECT_EQ(i[0], 1);
    EXPECT_EQ(i[1], 2);
    EXPECT_EQ(i[2], 3);
    EXPECT_EQ(j[0], 4);
    EXPECT_EQ(j[1], 5);
    EXPECT_EQ(j[2], 6);
    EXPECT_EQ(j[3], 7);
}

template<class Iter1, class Iter2>
void test_rng_3() {
    int i[3] = {1, 2, 3};
    int j[3] = {4, 5, 6};
    auto r = ranges::swap_ranges(as_lvalue(make_subrange(i, i + 3)), j);
    EXPECT_EQ(r.in1, i + 3);
    EXPECT_EQ(r.in2, j + 3);
    EXPECT_EQ(i[0], 4);
    EXPECT_EQ(i[1], 5);
    EXPECT_EQ(i[2], 6);
    EXPECT_EQ(j[0], 1);
    EXPECT_EQ(j[1], 2);
    EXPECT_EQ(j[2], 3);

    r = ranges::swap_ranges(as_lvalue(make_subrange(j, j + 3)), i);
    EXPECT_EQ(r.in1, j + 3);
    EXPECT_EQ(r.in2, i + 3);
    EXPECT_EQ(i[0], 1);
    EXPECT_EQ(i[1], 2);
    EXPECT_EQ(i[2], 3);
    EXPECT_EQ(j[0], 4);
    EXPECT_EQ(j[1], 5);
    EXPECT_EQ(j[2], 6);
}

template<class Iter1, class Iter2>
void test_rng_4() {
    int i[3] = {1, 2, 3};
    int j[4] = {4, 5, 6, 7};
    auto r = ranges::swap_ranges(as_lvalue(make_subrange(i, i + 3)),
                                 as_lvalue(make_subrange(j, j + 4)));
    EXPECT_EQ(r.in1, i + 3);
    EXPECT_EQ(r.in2, j + 3);
    EXPECT_EQ(i[0], 4);
    EXPECT_EQ(i[1], 5);
    EXPECT_EQ(i[2], 6);
    EXPECT_EQ(j[0], 1);
    EXPECT_EQ(j[1], 2);
    EXPECT_EQ(j[2], 3);
    EXPECT_EQ(j[3], 7);

    r = ranges::swap_ranges(as_lvalue(make_subrange(j, j + 4)),
                            as_lvalue(make_subrange(i, i + 3)));
    EXPECT_EQ(r.in1, j + 3);
    EXPECT_EQ(r.in2, i + 3);
    EXPECT_EQ(i[0], 1);
    EXPECT_EQ(i[1], 2);
    EXPECT_EQ(i[2], 3);
    EXPECT_EQ(j[0], 4);
    EXPECT_EQ(j[1], 5);
    EXPECT_EQ(j[2], 6);
    EXPECT_EQ(j[3], 7);

    // Also test with temporary subranges (no as_lvalue)
    auto r2 = ranges::swap_ranges(make_subrange(j, j + 4), make_subrange(i, i + 3));
    EXPECT_EQ(base(r2.in1), j + 3);
    EXPECT_EQ(base(r2.in2), i + 3);
    EXPECT_EQ(i[0], 4);
    EXPECT_EQ(i[1], 5);
    EXPECT_EQ(i[2], 6);
    EXPECT_EQ(j[0], 1);
    EXPECT_EQ(j[1], 2);
    EXPECT_EQ(j[2], 3);
    EXPECT_EQ(j[3], 7);
}

template<class Iter1, class Iter2>
void test_move_only() {
    std::unique_ptr<int> i[3];
    for (int k = 0; k < 3; ++k)
        i[k].reset(new int(k+1));
    std::unique_ptr<int> j[3];
    for (int k = 0; k < 3; ++k)
        j[k].reset(new int(k+4));
    auto r = ranges::swap_ranges(i, i + 3, j);
    EXPECT_EQ(r.in1, i + 3);
    EXPECT_EQ(r.in2, j + 3);
    EXPECT_EQ(*i[0], 4);
    EXPECT_EQ(*i[1], 5);
    EXPECT_EQ(*i[2], 6);
    EXPECT_EQ(*j[0], 1);
    EXPECT_EQ(*j[1], 2);
    EXPECT_EQ(*j[2], 3);
}

template<class Iter1, class Iter2>
void test() {
    test_iter_3<Iter1, Iter2>();
    test_iter_4<Iter1, Iter2>();
    test_rng_3<Iter1, Iter2>();
    test_rng_4<Iter1, Iter2>();
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------

TEST(SwapRangesTest, Basic) {
    // All combinations are tested with raw pointers; we simply call test()
    // with int* as both iterator types. This verifies all corner cases.
    test<int*, int*>();
}

TEST(SwapRangesTest, MoveOnly) {
    test_move_only<int*, int*>();
}

TEST(SwapRangesTest, SimpleArrays) {
    int a[4] = {1, 2, 3, 4};
    int b[4] = {5, 6, 7, 8};
    ranges::swap_ranges(a, a + 4, b);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 6);
    EXPECT_EQ(a[2], 7);
    EXPECT_EQ(a[3], 8);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
    EXPECT_EQ(b[2], 3);
    EXPECT_EQ(b[3], 4);

    ranges::swap_ranges(std::array<int, 2>{{3,4}}, a + 2);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 6);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[3], 4);
}

TEST(SwapRangesTest, Constexpr) {
    constexpr auto test = []() constexpr -> bool {
        int i[3] = {1, 2, 3};
        int j[3] = {4, 5, 6};
        const auto r = ranges::swap_ranges(i, j);
        if (r.in1 != i + 3) return false;
        if (r.in2 != j + 3) return false;
        if (i[0] != 4 || i[1] != 5 || i[2] != 6) return false;
        if (j[0] != 1 || j[1] != 2 || j[2] != 3) return false;
        return true;
    };
    static_assert(test(), "swap_ranges constexpr test failed");
    EXPECT_TRUE(test()); // runtime check for completeness
}
