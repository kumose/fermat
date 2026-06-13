#include <gtest/gtest.h>
#include <utility>
#include <string>
#include <fermat/algorithm/replace_copy_if.h>
#include <fermat/view/subrange.h>

/// test replace_copy_if with iterator pairs (raw pointers)
void test_replace_copy_if_iter() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    auto pred = [](int i) { return i == 2; };
    auto res = ranges::replace_copy_if(ia, ia + sa, ib, pred, 5);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + sa);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 5);
    EXPECT_EQ(ib[3], 3);
    EXPECT_EQ(ib[4], 4);
}

/// test replace_copy_if with range (subrange)
void test_replace_copy_if_range() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    auto pred = [](int i) { return i == 2; };
    auto rng = ranges::make_subrange(ia, ia + sa);
    auto res = ranges::replace_copy_if(rng, ib, pred, 5);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + sa);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 5);
    EXPECT_EQ(ib[3], 3);
    EXPECT_EQ(ib[4], 4);
}

/// projection test
TEST(ReplaceCopyIfTest, IteratorPair) {
    test_replace_copy_if_iter();
}

TEST(ReplaceCopyIfTest, Range) {
    test_replace_copy_if_range();
}
TEST(ReplaceCopyIfTest, Projection) {
    using P = std::pair<int, std::string>;
    P in[] = {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    constexpr std::size_t sa = sizeof(in) / sizeof(in[0]);
    P out[sa] = {};

    auto pred = [](int i) { return i == 2; };
    auto res = ranges::replace_copy_if(in, out, pred, P{5, "5"}, &P::first);
    EXPECT_EQ(res.in, std::end(in));
    EXPECT_EQ(res.out, std::end(out));
    // Use parentheses to avoid macro comma issues
    EXPECT_EQ(out[0], (P{0, "0"}));
    EXPECT_EQ(out[1], (P{1, "1"}));
    EXPECT_EQ(out[2], (P{5, "5"}));
    EXPECT_EQ(out[3], (P{3, "3"}));
    EXPECT_EQ(out[4], (P{4, "4"}));
}

/// constexpr test (compile‑time)
TEST(ReplaceCopyIfTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
        int ib[sa] = {0};
        auto pred = [](int i) { return i == 2; };
        auto r = ranges::replace_copy_if(ia, ib, pred, 42);
        bool ok = (r.in == ia + sa) &&
                  (r.out == ib + sa) &&
                  (ib[0] == 0) && (ib[1] == 1) &&
                  (ib[2] == 42) && (ib[3] == 3) && (ib[4] == 4);
        return ok;
    };
    static_assert(test(), "");
}
