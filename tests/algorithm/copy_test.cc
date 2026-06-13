#include <gtest/gtest.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <utility>
#include <algorithm>
#include <fermat/core.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/delimit.h>
#include <fermat/iterator/stream_iterators.h>

namespace {
    template<typename T, std::size_t N>
    using test_array = std::array<T, N>;
} // namespace

#if RANGES_CXX_CONSTEXPR >= RANGES_CXX_CONSTEXPR_14 && RANGES_CONSTEXPR_INVOKE
constexpr bool test_constexpr_copy() {
    int a[4] = {0, 0, 0, 0};
    int const b[4] = {1, 2, 3, 4};
    fermat::ranges::copy(b, a);
    return fermat::ranges::equal(b, a);
}

static_assert(test_constexpr_copy(), "");
#endif

constexpr bool test_constexpr() {
    using IL = std::initializer_list<int>;
    constexpr test_array<int, 4> input{{0, 1, 2, 3}};
    test_array<int, 4> tmp{{0, 0, 0, 0}};
    auto res = fermat::ranges::copy(input, fermat::ranges::begin(tmp));
    bool ok = (res.in == fermat::ranges::end(input)) &&
              (res.out == fermat::ranges::end(tmp)) &&
              fermat::ranges::equal(tmp, IL{0, 1, 2, 3});
    return ok;
}

static_assert(test_constexpr(), "");

TEST(CopyTest, PairArray) {
    using fermat::ranges::begin;
    using fermat::ranges::end;
    using fermat::ranges::size;

    std::pair<int, int> const a[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};
    static_assert(size(a) == 6, "");
    std::pair<int, int> out[size(a)] = {};

    auto res = fermat::ranges::copy(begin(a), end(a), out);
    EXPECT_EQ(res.in, end(a));
    EXPECT_EQ(res.out, out + size(out));
    EXPECT_TRUE(std::equal(a, a + size(a), out));

    std::fill_n(out, size(out), std::make_pair(0, 0));
    EXPECT_FALSE(std::equal(a, a + size(a), out));

    res = fermat::ranges::copy(a, out);
    EXPECT_EQ(res.in, a + size(a));
    EXPECT_EQ(res.out, out + size(out));
    EXPECT_TRUE(std::equal(a, a + size(a), out));
}

TEST(CopyTest, DelimitView) {
    using fermat::ranges::views::delimit;

    char const *sz = "hello world";
    char buf[50];
    auto str = delimit(sz, '\0');
    auto res = fermat::ranges::copy(str, buf);
    *res.out = '\0';
    EXPECT_EQ(res.in, std::next(fermat::ranges::begin(str), static_cast<std::ptrdiff_t>(std::strlen(sz))));
    EXPECT_EQ(res.out, buf + std::strlen(sz));
    EXPECT_EQ(std::strcmp(sz, buf), 0);
}

TEST(CopyTest, DelimitViewMove) {
    using fermat::ranges::views::delimit;

    char const *sz = "hello world";
    char buf[50];
    auto str = delimit(sz, '\0');
    auto res = fermat::ranges::copy(std::move(str), buf);
    *res.out = '\0';
    // is_dangling check removed
    EXPECT_EQ(res.out, buf + std::strlen(sz));
    EXPECT_EQ(std::strcmp(sz, buf), 0);
}

TEST(CopyTest, OstreamIterator) {
    using namespace fermat::ranges;
    std::ostringstream sout;
    std::vector<int> copy_vec{1, 1, 1, 1, 1};
    copy(copy_vec, ostream_iterator<int>(sout, " "));
    EXPECT_EQ(sout.str(), "1 1 1 1 1 ");
}
