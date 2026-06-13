#include <gtest/gtest.h>
#include <utility>
#include <string>
#include <fermat/algorithm/replace_copy.h>
#include <fermat/view/subrange.h>

/// test replace_copy with iterator pairs (raw pointers)
void test_replace_copy_iter() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    auto res = ranges::replace_copy(ia, ia + sa, ib, 2, 5);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + sa);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 5);
    EXPECT_EQ(ib[3], 3);
    EXPECT_EQ(ib[4], 4);
}

/// test replace_copy with range (subrange)
void test_replace_copy_range() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    auto rng = ranges::make_subrange(ia, ia + sa);
    auto res = ranges::replace_copy(rng, ib, 2, 5);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + sa);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 5);
    EXPECT_EQ(ib[3], 3);
    EXPECT_EQ(ib[4], 4);
}

/// projection test
TEST(ReplaceCopyTest, IteratorPair) {
    test_replace_copy_iter();
}

TEST(ReplaceCopyTest, Range) {
    test_replace_copy_range();
}

TEST(ReplaceCopyTest, Projection) {
    using P = std::pair<int, std::string>;
    P in[] = {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    constexpr std::size_t sa = sizeof(in) / sizeof(in[0]);
    P out[sa] = {};

    auto res = ranges::replace_copy(in, out, 2, P{5, "5"}, &P::first);
    EXPECT_EQ(res.in, std::end(in));
    EXPECT_EQ(res.out, std::end(out));
    EXPECT_EQ(out[0], (P{0, "0"}));
    EXPECT_EQ(out[1], (P{1, "1"}));
    EXPECT_EQ(out[2], (P{5, "5"}));
    EXPECT_EQ(out[3], (P{3, "3"}));
    EXPECT_EQ(out[4], (P{4, "4"}));
}

/// constexpr test (compile‑time)
TEST(ReplaceCopyTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
        int ib[sa] = {0};
        auto r = ranges::replace_copy(ia, ib, 2, 42);
        bool ok = (r.in == ia + sa) &&
                  (r.out == ib + sa) &&
                  (ib[0] == 0) && (ib[1] == 1) &&
                  (ib[2] == 42) && (ib[3] == 3) && (ib[4] == 4);
        return ok;
    };
    static_assert(test(), "");
}