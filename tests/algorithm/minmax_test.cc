#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/minmax.h>

std::mt19937 gen;

void test_minmax_for_size(unsigned N) {
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), gen);
    auto res = ranges::minmax(data);
    for (int i : data) {
        EXPECT_FALSE(i < res.min);
        EXPECT_FALSE(res.max < i);
    }
    // test with comparator (greater)
    auto comp = std::greater<int>();
    res = ranges::minmax(data, comp);
    for (int i : data) {
        EXPECT_FALSE(comp(i, res.min));
        EXPECT_FALSE(comp(res.max, i));
    }
}

TEST(MinmaxTest, Basic) {
    test_minmax_for_size(1);
    test_minmax_for_size(2);
    test_minmax_for_size(3);
    test_minmax_for_size(10);
    test_minmax_for_size(1000);
}

TEST(MinmaxTest, Projection) {
    struct S { int value; int index; };
    S arr[] = {
        {1,0},{2,1},{3,2},{4,3},{-4,4},{40,5},{-4,6},{40,7},{7,8},{8,9},{9,10}
    };
    auto res = ranges::minmax(arr, std::less<int>(), &S::value);
    EXPECT_EQ(res.min.value, -4);
    EXPECT_EQ(res.min.index, 4);
    EXPECT_EQ(res.max.value, 40);
    EXPECT_EQ(res.max.index, 7);
}

TEST(MinmaxTest, InitializerList) {
    auto res = ranges::minmax({4,3,1,2,6,5});
    EXPECT_EQ(res.min, 1);
    EXPECT_EQ(res.max, 6);
}
