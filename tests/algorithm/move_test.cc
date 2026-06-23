#include <gtest/gtest.h>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <fermat/algorithm/move.h>
#include <fermat/view/subrange.h>

/// Test move on plain integers
TEST(MoveTest, Basic) {
    constexpr int N = 1000;
    std::vector<int> src(N);
    for (int i = 0; i < N; ++i) src[i] = i;
    std::vector<int> dst(N, 0);

    auto res = fermat::ranges::move(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.end());
    for (int i = 0; i < N; ++i) EXPECT_EQ(src[i], dst[i]);

    // Range version
    std::fill(dst.begin(), dst.end(), 0);
    res = fermat::ranges::move(src, dst.begin());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.end());
    for (int i = 0; i < N; ++i) EXPECT_EQ(src[i], dst[i]);
}

/// Test move on std::unique_ptr (move‑only)
TEST(MoveTest, MoveOnly) {
    constexpr int N = 100;
    std::vector<std::unique_ptr<int>> src(N);
    for (int i = 0; i < N; ++i) src[i] = std::make_unique<int>(i);
    std::vector<std::unique_ptr<int>> dst(N);

    auto res = fermat::ranges::move(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.end());
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(src[i], nullptr);
        EXPECT_EQ(*dst[i], i);
    }

    // Move back to source
    fermat::ranges::move(dst.begin(), dst.end(), src.begin());
    res = fermat::ranges::move(src.begin(), src.end(), dst.begin());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.end());
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(src[i], nullptr);
        EXPECT_EQ(*dst[i], i);
    }
}

/// Constexpr test using std::array
TEST(MoveTest, Constexpr) {
    constexpr int N = 1000;
    constexpr auto test = []() constexpr {
        std::array<int, N> src{};
        for (int i = 0; i < N; ++i) src[i] = i;
        std::array<int, N> dst{};
        auto res = fermat::ranges::move(src.begin(), src.end(), dst.begin());
        bool ok = (res.in == src.end()) && (res.out == dst.end());
        for (int i = 0; i < N; ++i) ok = ok && (src[i] == dst[i]);
        return ok;
    };
    static_assert(test(), "constexpr move test failed");
}
