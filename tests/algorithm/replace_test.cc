#include <gtest/gtest.h>
#include <utility>
#include <string>
#include <vector>
#include <fermat/algorithm/replace.h>
#include <fermat/view/subrange.h>

void test_replace_iter() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    int* r = ranges::replace(ia, ia + sa, 2, 5);
    EXPECT_EQ(r, ia + sa);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 5);
    EXPECT_EQ(ia[3], 3);
    EXPECT_EQ(ia[4], 4);
}

void test_replace_range() {
    int ia[] = {0, 1, 2, 3, 4};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto rng = ranges::make_subrange(ia, ia + sa);
    auto r = ranges::replace(rng, 2, 5);
    EXPECT_EQ(r, ia + sa);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 5);
    EXPECT_EQ(ia[3], 3);
    EXPECT_EQ(ia[4], 4);
}

TEST(ReplaceTest, IteratorPair) {
    test_replace_iter();
}

TEST(ReplaceTest, Range) {
    test_replace_range();
}

TEST(ReplaceTest, Projection) {
    using P = std::pair<int, std::string>;
    P ia[] = {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    P* r = ranges::replace(ia, 2, P{42, "42"}, &P::first);
    EXPECT_EQ(r, ia + sa);
    EXPECT_EQ(ia[0], (P{0, "0"}));
    EXPECT_EQ(ia[1], (P{1, "1"}));
    EXPECT_EQ(ia[2], (P{42, "42"}));
    EXPECT_EQ(ia[3], (P{3, "3"}));
    EXPECT_EQ(ia[4], (P{4, "4"}));
}

TEST(ReplaceTest, RvalueRange) {
    using P = std::pair<int, std::string>;
    P ia[] = {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    auto r = ranges::replace(std::move(ia), 2, P{42, "42"}, &P::first);
    (void)r;
    EXPECT_EQ(ia[0], (P{0, "0"}));
    EXPECT_EQ(ia[1], (P{1, "1"}));
    EXPECT_EQ(ia[2], (P{42, "42"}));
    EXPECT_EQ(ia[3], (P{3, "3"}));
    EXPECT_EQ(ia[4], (P{4, "4"}));

    std::vector<P> vec{{0,"0"}, {1,"1"}, {2,"2"}, {3,"3"}, {4,"4"}};
    auto r2 = ranges::replace(std::move(vec), 2, P{42, "42"}, &P::first);
    (void)r2;
    EXPECT_EQ(vec[0], (P{0, "0"}));
    EXPECT_EQ(vec[1], (P{1, "1"}));
    EXPECT_EQ(vec[2], (P{42, "42"}));
    EXPECT_EQ(vec[3], (P{3, "3"}));
    EXPECT_EQ(vec[4], (P{4, "4"}));
}

TEST(ReplaceTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
        int* r = ranges::replace(ia, ia + sa, 2, 42);
        bool ok = (r == ia + sa) &&
                  (ia[0] == 0) && (ia[1] == 1) &&
                  (ia[2] == 42) && (ia[3] == 3) && (ia[4] == 4);
        return ok;
    };
    static_assert(test(), "");
}
