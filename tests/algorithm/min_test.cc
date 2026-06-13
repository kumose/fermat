#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/min.h>

std::mt19937 gen;

void test_min_for_size(unsigned N) {
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), gen);
    auto v = fermat::ranges::min(data);
    for (int i : data) {
        EXPECT_FALSE(i < v);
    }
    // test with comparator (greater)
    auto comp = std::greater<int>();
    v = fermat::ranges::min(data, comp);
    for (int i : data) {
        EXPECT_FALSE(comp(i, v));
    }
}

TEST(MinTest, Basic) {
    test_min_for_size(1);
    test_min_for_size(2);
    test_min_for_size(3);
    test_min_for_size(10);
    test_min_for_size(1000);
}

TEST(MinTest, Projection) {
    struct S { int i; };
    S arr[] = {{1},{2},{3},{4},{-4},{5},{6},{7},{8},{9}};
    auto v = fermat::ranges::min(arr, std::less<int>(), &S::i);
    EXPECT_EQ(v.i, -4);
}

TEST(MinTest, InitializerList) {
    EXPECT_EQ(fermat::ranges::min({4,3,1,2,6,5}), 1);
}
