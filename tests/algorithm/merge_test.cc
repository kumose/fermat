#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <array>
#include <vector>
#include <fermat/algorithm/merge.h>
#include <fermat/algorithm/is_sorted.h>

constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    constexpr unsigned N = 100;
    std::array<int, N> a{};
    std::array<int, N> b{};
    std::array<int, 2*N> c{};
    for (unsigned i = 0; i < N; ++i) {
        a[i] = 2 * i;          // even
        b[i] = 2 * i + 1;      // odd
    }
    auto result = merge(a.begin(), a.end(), b.begin(), b.end(), c.begin());
    bool ok = (result.in1 == a.end()) &&
              (result.in2 == b.end()) &&
              (result.out == c.end()) &&
              (c[0] == 0) &&
              (c[2*N-1] == static_cast<int>(2*N-1)) &&
              is_sorted(c);
    return ok;
}
static_assert(test_constexpr(), "constexpr merge test failed");

TEST(MergeTest, Basic) {
    const int N = 100000;
    std::vector<int> a(N), b(N), c(2*N);
    for (int i = 0; i < N; ++i) {
        a[i] = 2 * i;
        b[i] = 2 * i + 1;
    }
    auto res = fermat::ranges::merge(a.begin(), a.end(), b.begin(), b.end(), c.begin());
    EXPECT_EQ(res.in1, a.end());
    EXPECT_EQ(res.in2, b.end());
    EXPECT_EQ(res.out, c.end());
    EXPECT_EQ(c[0], 0);
    EXPECT_EQ(c[2*N-1], 2*N-1);
    EXPECT_TRUE(std::is_sorted(c.begin(), c.end()));
}

TEST(MergeTest, RangeVersion) {
    const int N = 100000;
    std::vector<int> a(N), b(N), c(2*N);
    for (int i = 0; i < N; ++i) {
        a[i] = 2 * i;
        b[i] = 2 * i + 1;
    }
    auto res = fermat::ranges::merge(a, b, c.begin());
    EXPECT_EQ(res.in1, a.end());
    EXPECT_EQ(res.in2, b.end());
    EXPECT_EQ(res.out, c.end());
    EXPECT_EQ(c[0], 0);
    EXPECT_EQ(c[2*N-1], 2*N-1);
    EXPECT_TRUE(std::is_sorted(c.begin(), c.end()));
}
