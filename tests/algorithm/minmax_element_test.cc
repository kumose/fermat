#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <array>
#include <fermat/algorithm/minmax_element.h>

std::mt19937 gen;

/// Test minmax_element on a range (iterator pair)
void test_minmax_element_on_range(int* first, int* last) {
    auto res = ranges::minmax_element(first, last);
    if (first != last) {
        for (int* i = first; i != last; ++i) {
            EXPECT_FALSE(*i < *res.min);
            EXPECT_FALSE(*res.max < *i);
        }
    } else {
        EXPECT_EQ(res.min, last);
        EXPECT_EQ(res.max, last);
    }
}

/// Test minmax_element with a custom comparator
void test_minmax_element_comp(int* first, int* last) {
    std::greater<int> comp;
    auto res = ranges::minmax_element(first, last, comp);
    if (first != last) {
        for (int* i = first; i != last; ++i) {
            EXPECT_FALSE(comp(*i, *res.min));
            EXPECT_FALSE(comp(*res.max, *i));
        }
    } else {
        EXPECT_EQ(res.min, last);
        EXPECT_EQ(res.max, last);
    }
}

/// Generate random data of size N and run both tests
void test_minmax_element_for_size(unsigned N) {
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), gen);
    test_minmax_element_on_range(data.data(), data.data() + N);
    test_minmax_element_comp(data.data(), data.data() + N);
}

TEST(MinmaxElementTest, Basic) {
    test_minmax_element_for_size(0);
    test_minmax_element_for_size(1);
    test_minmax_element_for_size(2);
    test_minmax_element_for_size(3);
    test_minmax_element_for_size(10);
    test_minmax_element_for_size(1000);
}

TEST(MinmaxElementTest, AllEqual) {
    const int N = 100;
    std::vector<int> data(N, 5);
    auto res = ranges::minmax_element(data.begin(), data.end());
    EXPECT_EQ(res.min, data.begin());
    EXPECT_EQ(res.max, data.end() - 1);
}

TEST(MinmaxElementTest, Projection) {
    struct S { int i; };
    S arr[] = {{1},{2},{3},{4},{-4},{5},{6},{40},{7},{8},{9}};
    auto res = ranges::minmax_element(arr, std::less<int>(), &S::i);
    EXPECT_EQ(res.min->i, -4);
    EXPECT_EQ(res.max->i, 40);
}

TEST(MinmaxElementTest, Constexpr) {
    constexpr std::array<int,10> a{{1,2,3,4,-4,5,6,40,8,9}};
    static_assert(ranges::minmax_element(a).min == a.begin() + 4, "");
    static_assert(ranges::minmax_element(a).max == a.begin() + 7, "");
}
