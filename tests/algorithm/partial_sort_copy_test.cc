#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/partial_sort_copy.h>
#include <turbo/log/logging.h>

std::mt19937 gen;

/// Test partial_sort_copy for given input size N and output size M
void test_partial_sort_copy_sizes(int N, int M) {
    if (N < 0 || M < 0) return;
    KLOG(INFO)<<"N="<<N<<", M="<<M;
    std::vector<int> input(N);
    for (int i = 0; i < N; ++i) input[i] = i;
    std::shuffle(input.begin(), input.end(), gen);

    std::vector<int> output(M, 0);
    /// iterator-pair version
    auto res = fermat::ranges::partial_sort_copy(input.begin(), input.end(),
                                         output.begin(), output.end());
    auto expected = output.begin() + std::min(N, M);
    EXPECT_EQ(res, expected);
    for (int i = 0; i < std::min(N, M); ++i) {
        EXPECT_EQ(output[i], i);
    }

    /// with greater comparator
    std::shuffle(input.begin(), input.end(), gen);
    std::fill(output.begin(), output.end(), 0);
    auto res2 = fermat::ranges::partial_sort_copy(input.begin(), input.end(),
                                          output.begin(), output.end(),
                                          std::greater<int>());
    EXPECT_EQ(res2, expected);
    for (int i = 0; i < std::min(N, M); ++i) {
        EXPECT_EQ(output[i], N - 1 - i);
    }
}

void test_partial_sort_copy_for_N(int N) {
    test_partial_sort_copy_sizes(N, 0);
    test_partial_sort_copy_sizes(N, 1);
    test_partial_sort_copy_sizes(N, 2);
    test_partial_sort_copy_sizes(N, 3);
    test_partial_sort_copy_sizes(N, N/2 - 1);
    test_partial_sort_copy_sizes(N, N/2);
    test_partial_sort_copy_sizes(N, N/2 + 1);
    test_partial_sort_copy_sizes(N, N - 2);
    test_partial_sort_copy_sizes(N, N - 1);
    test_partial_sort_copy_sizes(N, N);
    test_partial_sort_copy_sizes(N, N + 1000);
}

TEST(PartialSortCopyTest, Basic) {
    int i = 0;
    auto r = fermat::ranges::partial_sort_copy(&i, &i, &i, &i + 5);
    EXPECT_EQ(r, &i);
    EXPECT_EQ(i, 0);
}

TEST(PartialSortCopyTest, VariousSizes) {
    test_partial_sort_copy_for_N(0);
    test_partial_sort_copy_for_N(10);
    test_partial_sort_copy_for_N(256);
    test_partial_sort_copy_for_N(257);
    test_partial_sort_copy_for_N(499);
    test_partial_sort_copy_for_N(500);
    test_partial_sort_copy_for_N(997);
    test_partial_sort_copy_for_N(1000);
    test_partial_sort_copy_for_N(1009);
}

TEST(PartialSortCopyTest, InitializerList) {
    std::initializer_list<int> input = {5, 3, 4, 1, 8, 2, 6, 7, 0, 9};
    std::vector<int> output(10, 0);
    auto res = fermat::ranges::partial_sort_copy(input.begin(), input.end(),
                                         output.begin(), output.end());
    EXPECT_EQ(res, output.end());
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(output[i], i);
    }
}
