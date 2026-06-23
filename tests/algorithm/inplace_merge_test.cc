#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <vector>
#include <fermat/algorithm/inplace_merge.h>

namespace {
    std::mt19937 gen;

    void test_one(unsigned N, unsigned M) {
        ASSERT_LE(M, N);
        std::vector<int> v(N);
        for (unsigned i = 0; i < N; ++i)
            v[i] = static_cast<int>(i);
        std::shuffle(v.begin(), v.end(), gen);
        std::sort(v.begin(), v.begin() + M);
        std::sort(v.begin() + M, v.end());

        auto res = fermat::ranges::inplace_merge(v.begin(), v.begin() + M, v.end());
        EXPECT_EQ(res, v.end());
        if (N > 0) {
            EXPECT_EQ(v[0], 0);
            EXPECT_EQ(v[N-1], static_cast<int>(N-1));
            EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
        }
    }

    void test_one_rng(unsigned N, unsigned M) {
        ASSERT_LE(M, N);
        std::vector<int> v(N);
        for (unsigned i = 0; i < N; ++i)
            v[i] = static_cast<int>(i);
        std::shuffle(v.begin(), v.end(), gen);
        std::sort(v.begin(), v.begin() + M);
        std::sort(v.begin() + M, v.end());

        auto res = fermat::ranges::inplace_merge(fermat::ranges::make_subrange(v.begin(), v.end()), v.begin() + M);
        EXPECT_EQ(res, v.end());
        if (N > 0) {
            EXPECT_EQ(v[0], 0);
            EXPECT_EQ(v[N-1], static_cast<int>(N-1));
            EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
        }
    }

    void test(unsigned N) {
        test_one(N, 0);
        test_one(N, N/4);
        test_one(N, N/2);
        test_one(N, 3*N/4);
        test_one(N, N);

        test_one_rng(N, 0);
        test_one_rng(N, N/4);
        test_one_rng(N, N/2);
        test_one_rng(N, 3*N/4);
        test_one_rng(N, N);
    }
} // namespace

TEST(InplaceMergeTest, Basic) {
    test(0);
    test(1);
    test(2);
    test(3);
    test(4);
    test(100);
    test(1000);
}
