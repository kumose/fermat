#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <memory>
#include <vector>
#include <array>
#include <fermat/algorithm/partial_sort.h>

std::mt19937 gen;

void test_partial_sort_sizes(int N, int M) {
    std::vector<int> data(N);
    for (int i = 0; i < N; ++i) data[i] = i;
    std::shuffle(data.begin(), data.end(), gen);

    // Iterator version: partial_sort(first, middle, last)
    auto res = fermat::ranges::partial_sort(data.data(), data.data() + M, data.data() + N);
    EXPECT_EQ(res, data.data() + N);
    for (int i = 0; i < M; ++i) EXPECT_EQ(data[i], i);

    // Range version: partial_sort(rng, middle)
    std::shuffle(data.begin(), data.end(), gen);
    auto res2 = fermat::ranges::partial_sort(data, data.begin() + M);
    EXPECT_EQ(res2, data.end());
    for (int i = 0; i < M; ++i) EXPECT_EQ(data[i], i);

    // With comparator (std::greater)
    std::shuffle(data.begin(), data.end(), gen);
    auto res3 = fermat::ranges::partial_sort(data.data(), data.data() + M, data.data() + N, std::greater<int>());
    EXPECT_EQ(res3, data.data() + N);
    for (int i = 0; i < M; ++i) EXPECT_EQ(data[i], N - i - 1);

    std::shuffle(data.begin(), data.end(), gen);
    auto res4 = fermat::ranges::partial_sort(data, data.begin() + M, std::greater<int>());
    EXPECT_EQ(res4, data.end());
    for (int i = 0; i < M; ++i) EXPECT_EQ(data[i], N - i - 1);
}

void test_partial_sort_for_N(int N) {
    test_partial_sort_sizes(N, 0);
    test_partial_sort_sizes(N, 1);
    test_partial_sort_sizes(N, 2);
    test_partial_sort_sizes(N, 3);
    test_partial_sort_sizes(N, N/2 - 1);
    test_partial_sort_sizes(N, N/2);
    test_partial_sort_sizes(N, N/2 + 1);
    test_partial_sort_sizes(N, N-2);
    test_partial_sort_sizes(N, N-1);
    test_partial_sort_sizes(N, N);
}

TEST(PartialSortTest, SingleElement) {
    int i = 0;
    auto res = fermat::ranges::partial_sort(&i, &i, &i);
    EXPECT_EQ(i, 0);
    EXPECT_EQ(res, &i);
}

TEST(PartialSortTest, VariousSizes) {
    test_partial_sort_for_N(10);
    test_partial_sort_for_N(256);
    test_partial_sort_for_N(257);
    test_partial_sort_for_N(499);
    test_partial_sort_for_N(500);
    test_partial_sort_for_N(997);
    test_partial_sort_for_N(1000);
    test_partial_sort_for_N(1009);
}

TEST(PartialSortTest, MoveOnly) {
    const int N = 1000;
    std::vector<std::unique_ptr<int>> v(N);
    for (int i = 0; i < N; ++i) v[i] = std::make_unique<int>(N - i - 1);
    auto middle = v.begin() + N/2;
    auto indirect_less = [](const std::unique_ptr<int>& x, const std::unique_ptr<int>& y) {
        return *x < *y;
    };
    auto res = fermat::ranges::partial_sort(v, middle, indirect_less);
    EXPECT_EQ(res, v.end());
    for (int i = 0; i < N/2; ++i) EXPECT_EQ(*v[i], i);
}

TEST(PartialSortTest, Projection) {
    struct S { int i; int j; };
    const int N = 1000;
    std::vector<S> v(N);
    for (int i = 0; i < N; ++i) {
        v[i].i = N - i - 1;
        v[i].j = i;
    }
    auto middle = v.begin() + N/2;
    auto res = fermat::ranges::partial_sort(v, middle, std::less<int>(), &S::i);
    EXPECT_EQ(res, v.end());
    for (int i = 0; i < N/2; ++i) {
        EXPECT_EQ(v[i].i, i);
        EXPECT_EQ(v[i].j, N - i - 1);
    }
}

TEST(PartialSortTest, ConstexprRuntime) {
    const int N = 100;
    std::array<int, N> ia;
    for (int i = 0; i < N; ++i) ia[i] = N - i - 1;
    auto res = fermat::ranges::partial_sort(ia.begin(), ia.begin() + N/2, ia.end(), std::less<int>());
    EXPECT_EQ(res, ia.end());
    for (int i = 0; i < N/2; ++i) EXPECT_EQ(ia[i], i);
}
