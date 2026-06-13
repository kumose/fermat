#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <array>
#include <fermat/algorithm/min_element_test.h>

std::mt19937 gen;

/// Test min_element on a range (iterator pair)
void test_min_element_on_range(int* first, int* last) {
    auto it = fermat::ranges::min_element(first, last);
    if (first != last) {
        for (int* i = first; i != last; ++i)
            EXPECT_FALSE(*i < *it);
    } else {
        EXPECT_EQ(it, last);
    }
}

/// Test min_element with a custom comparator
void test_min_element_comp(int* first, int* last) {
    auto it = fermat::ranges::min_element(first, last, std::greater<int>());
    if (first != last) {
        for (int* i = first; i != last; ++i)
            EXPECT_FALSE(std::greater<int>()(*i, *it));
    } else {
        EXPECT_EQ(it, last);
    }
}

/// Generate random data of size N and run both tests
void test_min_element_for_size(unsigned N) {
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), gen);
    test_min_element_on_range(data.data(), data.data() + N);
    test_min_element_comp(data.data(), data.data() + N);
}

TEST(MinElementTest, Basic) {
    test_min_element_for_size(0);
    test_min_element_for_size(1);
    test_min_element_for_size(2);
    test_min_element_for_size(3);
    test_min_element_for_size(10);
    test_min_element_for_size(1000);
}

TEST(MinElementTest, Projection) {
    struct S { int i; };
    S arr[] = {{1},{2},{3},{4},{-4},{5},{6},{7},{8},{9}};
    const S* ps = fermat::ranges::min_element(arr, std::less<int>(), &S::i);
    EXPECT_EQ(ps->i, -4);
}

TEST(MinElementTest, Constexpr) {
    constexpr std::array<int,10> a{{1,2,3,4,-4,5,6,7,8,9}};
    static_assert(fermat::ranges::min_element(a) == a.begin() + 4, "");
}
