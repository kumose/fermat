#include <gtest/gtest.h>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <fermat/algorithm/move_backward.h>
#include <fermat/view/subrange.h>

// Test move_backward with plain integers
TEST(MoveBackwardTest, Basic) {
    constexpr int N = 1000;
    std::vector<int> src(N);
    for (int i = 0; i < N; ++i) src[i] = i;
    std::vector<int> dst(N, 0);

    auto res = fermat::ranges::move_backward(src.begin(), src.end(), dst.end());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.begin());
    for (int i = 0; i < N; ++i) EXPECT_EQ(src[i], dst[i]);

    // Range version
    std::fill(dst.begin(), dst.end(), 0);
    res = fermat::ranges::move_backward(src, dst.end());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.begin());
    for (int i = 0; i < N; ++i) EXPECT_EQ(src[i], dst[i]);
}

// Test move_backward on move‑only types (std::unique_ptr)
TEST(MoveBackwardTest, MoveOnly) {
    constexpr int N = 100;
    std::vector<std::unique_ptr<int>> src(N);
    for (int i = 0; i < N; ++i) src[i] = std::make_unique<int>(i);
    std::vector<std::unique_ptr<int>> dst(N);

    auto res = fermat::ranges::move_backward(src.begin(), src.end(), dst.end());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.begin());
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(src[i], nullptr);
        EXPECT_EQ(*dst[i], i);
    }

    // Move back to source
    fermat::ranges::move_backward(dst.begin(), dst.end(), src.end());
    res = fermat::ranges::move_backward(src.begin(), src.end(), dst.end());
    EXPECT_EQ(res.in, src.end());
    EXPECT_EQ(res.out, dst.begin());
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(src[i], nullptr);
        EXPECT_EQ(*dst[i], i);
    }
}

// Constexpr test using std::array
TEST(MoveBackwardTest, Constexpr) {
    constexpr int N = 1000;
    constexpr auto test = []() constexpr {
        std::array<int, N> src{};
        for (int i = 0; i < N; ++i) src[i] = i;
        std::array<int, N> dst{};
        auto res = fermat::ranges::move_backward(src.begin(), src.end(), dst.end());
        bool ok = (res.in == src.end()) && (res.out == dst.begin());
        for (int i = 0; i < N; ++i) ok = ok && (src[i] == dst[i]);
        return ok;
    };
    static_assert(test(), "constexpr move_backward test failed");
}
