#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <vector>
#include <fermat/algorithm/nth_element.h>

std::mt19937 gen;

void test_one(unsigned N, unsigned M) {
    ASSERT_GT(N, 0u);
    ASSERT_LT(M, N);
    std::vector<int> data(N);
    for (unsigned i = 0; i < N; ++i) data[i] = i;
    std::shuffle(data.begin(), data.end(), gen);

    // iterator-pair version: nth_element(first, nth, last)
    auto res = fermat::ranges::nth_element(data.data(), data.data() + M, data.data() + N);
    EXPECT_EQ(res, data.data() + N);
    EXPECT_EQ(static_cast<unsigned>(data[M]), M);

    // shuffle again for range version
    std::shuffle(data.begin(), data.end(), gen);
    // range version: nth_element(rng, nth)
    auto res2 = fermat::ranges::nth_element(data, data.begin() + M);
    EXPECT_EQ(res2, data.end());
    EXPECT_EQ(static_cast<unsigned>(data[M]), M);
}

void test(unsigned N) {
    test_one(N, 0);
    test_one(N, 1);
    test_one(N, 2);
    test_one(N, 3);
    test_one(N, N/2 - 1);
    test_one(N, N/2);
    test_one(N, N/2 + 1);
    test_one(N, N - 3);
    test_one(N, N - 2);
    test_one(N, N - 1);
}

TEST(NthElementTest, SingleElement) {
    int d = 0;
    fermat::ranges::nth_element(&d, &d, &d);
    EXPECT_EQ(d, 0);
}

TEST(NthElementTest, VariousSizes) {
    test(256);
    test(257);
    test(499);
    test(500);
    test(997);
    test(1000);
    test(1009);
}

TEST(NthElementTest, Projection) {
    struct S { int i; int j; };
    const int N = 257;
    const int M = 56;
    std::vector<S> data(N);
    for (int i = 0; i < N; ++i) {
        data[i].i = i;
        data[i].j = i;
    }
    std::shuffle(data.begin(), data.end(), gen);
    auto res = fermat::ranges::nth_element(data, data.begin() + M, std::less<int>(), &S::i);
    EXPECT_EQ(res, data.end());
    EXPECT_EQ(data[M].i, M);
    EXPECT_EQ(data[M].j, M);
}