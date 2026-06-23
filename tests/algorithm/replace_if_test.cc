#include <gtest/gtest.h>
#include <utility>
#include <string>
#include <vector>
#include <fermat/algorithm/replace_if.h>
#include <fermat/view/subrange.h>

/// test replace_if with iterator pairs (raw pointers)
void test_replace_if_iter() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto pred = [](int i) { return i == 2; };
    int* r = fermat::ranges::replace_if(ia, ia + sa, pred, 5);
    EXPECT_EQ(r, ia + sa);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 5);
    EXPECT_EQ(ia[3], 3);
    EXPECT_EQ(ia[4], 4);
}

/// test replace_if with range (subrange)
void test_replace_if_range() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto pred = [](int i) { return i == 2; };
    auto rng = fermat::ranges::make_subrange(ia, ia + sa);
    auto r = fermat::ranges::replace_if(rng, pred, 5);
    EXPECT_EQ(r, ia + sa);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 5);
    EXPECT_EQ(ia[3], 3);
    EXPECT_EQ(ia[4], 4);
}

/// projection test
TEST(ReplaceIfTest, IteratorPair) {
    test_replace_if_iter();
}

TEST(ReplaceIfTest, Range) {
    test_replace_if_range();
}

TEST(ReplaceIfTest, Projection) {
    using P = std::pair<int, std::string>;
    P ia[] = {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto pred = [](int i) { return i == 2; };
    P* r = fermat::ranges::replace_if(ia, pred, P{42, "42"}, &P::first);
    EXPECT_EQ(r, ia + sa);
    EXPECT_EQ(ia[0], (P{0, "0"}));
    EXPECT_EQ(ia[1], (P{1, "1"}));
    EXPECT_EQ(ia[2], (P{42, "42"}));
    EXPECT_EQ(ia[3], (P{3, "3"}));
    EXPECT_EQ(ia[4], (P{4, "4"}));
}

/// rvalue range test (compile only, we just check that it compiles and content is correct)
TEST(ReplaceIfTest, RvalueRange) {
    using P = std::pair<int, std::string>;
    P ia[] = {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    auto pred = [](int i) { return i == 2; };
    auto r = fermat::ranges::replace_if(std::move(ia), pred, P{42, "42"}, &P::first);
    // r is a dangling iterator; we ignore it.
    (void)r;
    EXPECT_EQ(ia[0], (P{0, "0"}));
    EXPECT_EQ(ia[1], (P{1, "1"}));
    EXPECT_EQ(ia[2], (P{42, "42"}));
    EXPECT_EQ(ia[3], (P{3, "3"}));
    EXPECT_EQ(ia[4], (P{4, "4"}));

    std::vector<P> vec{{0,"0"}, {1,"1"}, {2,"2"}, {3,"3"}, {4,"4"}};
    auto r2 = fermat::ranges::replace_if(std::move(vec), pred, P{42, "42"}, &P::first);
    (void)r2;
    EXPECT_EQ(vec[0], (P{0, "0"}));
    EXPECT_EQ(vec[1], (P{1, "1"}));
    EXPECT_EQ(vec[2], (P{42, "42"}));
    EXPECT_EQ(vec[3], (P{3, "3"}));
    EXPECT_EQ(vec[4], (P{4, "4"}));
}

/// constexpr test (compile‑time)
TEST(ReplaceIfTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
        auto pred = [](int i) { return i == 2; };
        int* r = fermat::ranges::replace_if(ia, ia + sa, pred, 42);
        bool ok = (r == ia + sa) &&
                  (ia[0] == 0) && (ia[1] == 1) &&
                  (ia[2] == 42) && (ia[3] == 3) && (ia[4] == 4);
        return ok;
    };
    static_assert(test(), "");
}
