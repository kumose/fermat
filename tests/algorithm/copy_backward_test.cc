#include <gtest/gtest.h>
#include <cstring>
#include <algorithm>
#include <utility>
#include <vector>
#include <array>
#include <fermat/core.h>
#include <fermat/algorithm/copy_backward.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/all.h>

namespace {
    template<typename T, std::size_t N>
    using test_array = std::array<T, N>;
} // namespace

constexpr bool test_constexpr() {
    using IL = std::initializer_list<int>;
    constexpr test_array<int, 4> input{{0, 1, 2, 3}};
    test_array<int, 4> a1{{0, 0, 0, 0}};
    auto res = ranges::copy_backward(input, ranges::end(a1));
    bool ok = (res.in == ranges::end(input)) &&
              (res.out == ranges::begin(a1)) &&
              ranges::equal(a1, IL{0, 1, 2, 3});
    return ok;
}
static_assert(test_constexpr(), "");

TEST(CopyBackwardTest, PairArray) {
    using ranges::begin;
    using ranges::end;
    using ranges::size;

    std::pair<int, int> const a[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};
    static_assert(size(a) == 6, "");
    std::pair<int, int> out[size(a)] = {};

    {
        auto res = ranges::copy_backward(begin(a), end(a), end(out));
        EXPECT_EQ(res.in, end(a));
        EXPECT_EQ(res.out, begin(out));
        EXPECT_TRUE(std::equal(a, a + size(a), out));
    }

    {
        std::fill_n(out, size(out), std::make_pair(0, 0));
        auto res = ranges::copy_backward(a, end(out));
        EXPECT_EQ(res.in, end(a));
        EXPECT_EQ(res.out, begin(out));
        EXPECT_TRUE(std::equal(a, a + size(a), out));
    }

#ifndef RANGES_WORKAROUND_MSVC_573728
    {
        std::fill_n(out, size(out), std::make_pair(0, 0));
        auto res = ranges::copy_backward(std::move(a), end(out));
        // is_dangling check omitted for simplicity
        EXPECT_EQ(res.out, begin(out));
        EXPECT_TRUE(std::equal(a, a + size(a), out));
    }
#endif

    {
        std::fill_n(out, size(out), std::make_pair(0, 0));
        std::vector<std::pair<int, int>> vec(begin(a), end(a));
        auto res = ranges::copy_backward(std::move(vec), end(out));
        EXPECT_EQ(res.out, begin(out));
        EXPECT_TRUE(std::equal(a, a + size(a), out));
    }

    {
        std::fill_n(out, size(out), std::make_pair(0, 0));
        auto res = ranges::copy_backward(ranges::views::all(a), end(out));
        EXPECT_EQ(res.in, end(a));
        EXPECT_EQ(res.out, begin(out));
        EXPECT_TRUE(std::equal(a, a + size(a), out));
    }
}