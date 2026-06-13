#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/max.h>

std::mt19937 gen;

void test_max_for_size(unsigned N) {
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), gen);
    auto v = fermat::ranges::max(data);
    for (int i : data) {
        EXPECT_FALSE(v < i);
    }
    // test with comparator (greater)
    auto comp = std::greater<int>();
    v = fermat::ranges::max(data, comp);
    for (int i : data) {
        EXPECT_FALSE(comp(v, i));
    }
}

TEST(MaxTest, Basic) {
    test_max_for_size(1);
    test_max_for_size(2);
    test_max_for_size(3);
    test_max_for_size(10);
    test_max_for_size(1000);
}

TEST(MaxTest, Projection) {
    struct S { int i; };
    S arr[] = {{1},{2},{3},{4},{40},{5},{6},{7},{8},{9}};
    auto v = fermat::ranges::max(arr, std::less<int>(), &S::i);
    EXPECT_EQ(v.i, 40);
}

TEST(MaxTest, InitializerList) {
    EXPECT_EQ(fermat::ranges::max({4,3,1,2,6,5}), 6);
}