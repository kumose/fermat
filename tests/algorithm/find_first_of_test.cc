#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/find_first_of.h>
#include <fermat/view/subrange.h>

TEST(FindFirstOfTest, IteratorPairs) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 0, 1, 2, 3};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int ib[] = {1, 3, 5, 7};
    int ic[] = {7};

    auto it = find_first_of(ia, ia + sa, ib, ib + 4);
    EXPECT_EQ(it, ia + 1);

    it = find_first_of(ia, ia + sa, ic, ic + 1);
    EXPECT_EQ(it, ia + sa);

    it = find_first_of(ia, ia + sa, ic, ic);
    EXPECT_EQ(it, ia + sa);

    it = find_first_of(ia, ia, ic, ic + 1);
    EXPECT_EQ(it, ia);
}

TEST(FindFirstOfTest, WithPredicate) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 0, 1, 2, 3};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int ib[] = {1, 3, 5, 7};
    int ic[] = {7};

    auto it = find_first_of(ia, ia + sa, ib, ib + 4, std::equal_to<int>());
    EXPECT_EQ(it, ia + 1);

    it = find_first_of(ia, ia + sa, ic, ic + 1, std::equal_to<int>());
    EXPECT_EQ(it, ia + sa);

    it = find_first_of(ia, ia + sa, ic, ic, std::equal_to<int>());
    EXPECT_EQ(it, ia + sa);

    it = find_first_of(ia, ia, ic, ic + 1, std::equal_to<int>());
    EXPECT_EQ(it, ia);
}

TEST(FindFirstOfTest, RangeVersions) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 0, 1, 2, 3};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int ib[] = {1, 3, 5, 7};
    int ic[] = {7};

    auto haystack = make_subrange(ia, ia + sa);
    auto needle1 = make_subrange(ib, ib + 4);
    auto needle2 = make_subrange(ic, ic + 1);
    auto empty = make_subrange(ic, ic);

    auto res = find_first_of(haystack, needle1);
    EXPECT_EQ(res, ia + 1);          // res is an iterator
    EXPECT_EQ(res + 1, ia + 2);      // "end" after found element

    res = find_first_of(haystack, needle2);
    EXPECT_EQ(res, ia + sa);         // not found -> haystack.end()
    EXPECT_EQ(res, haystack.end());

    res = find_first_of(haystack, empty);
    EXPECT_EQ(res, ia + sa);
    EXPECT_EQ(res, haystack.end());

    res = find_first_of(make_subrange(ia, ia), needle2);
    EXPECT_EQ(res, ia);
    EXPECT_EQ(res, haystack.begin());
}

TEST(FindFirstOfTest, RangeWithPredicate) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 0, 1, 2, 3};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int ib[] = {1, 3, 5, 7};
    int ic[] = {7};

    auto haystack = make_subrange(ia, ia + sa);
    auto needle1 = make_subrange(ib, ib + 4);
    auto needle2 = make_subrange(ic, ic + 1);
    auto empty = make_subrange(ic, ic);

    auto res = find_first_of(haystack, needle1, std::equal_to<int>());
    EXPECT_EQ(res, ia + 1);

    res = find_first_of(haystack, needle2, std::equal_to<int>());
    EXPECT_EQ(res, ia + sa);

    res = find_first_of(haystack, empty, std::equal_to<int>());
    EXPECT_EQ(res, ia + sa);

    res = find_first_of(make_subrange(ia, ia), needle2, std::equal_to<int>());
    EXPECT_EQ(res, ia);
}

TEST(FindFirstOfTest, WithProjection) {
    using namespace ranges;

    struct S { int i; };
    S ia[] = {{0}, {1}, {2}, {3}, {0}, {1}, {2}, {3}};
    int ib[] = {1, 3, 5, 7};
    int ic[] = {7};

    // Fixed: use identity{} (or ranges::identity{}) instead of identity
    auto it = find_first_of(ia, ia + 8, ib, ib + 4,
                            std::equal_to<int>(), &S::i, identity{});
    EXPECT_EQ(it, ia + 1);

    it = find_first_of(ia, ia + 8, ic, ic + 1,
                       std::equal_to<int>(), &S::i, identity{});
    EXPECT_EQ(it, ia + 8);

    it = find_first_of(ia, ia + 8, ic, ic,
                       std::equal_to<int>(), &S::i, identity{});
    EXPECT_EQ(it, ia + 8);

    it = find_first_of(ia, ia, ic, ic + 1,
                       std::equal_to<int>(), &S::i, identity{});
    EXPECT_EQ(it, ia);

    // Range version with projection
    auto haystack = make_subrange(ia, ia + 8);
    auto needle = make_subrange(ib, ib + 4);
    auto res = find_first_of(haystack, needle,
                             std::equal_to<int>(), &S::i, identity{});
    EXPECT_EQ(res, ia + 1);
}

TEST(FindFirstOfTest, Constexpr) {
    using namespace ranges;
    constexpr int ia[] = {0, 1, 2, 3, 0, 1, 2, 3};
    constexpr int ib[] = {1, 3, 5, 7};
    constexpr int ic[] = {7};

    static_assert(find_first_of(ia, ia + 8, ib, ib + 4) == ia + 1, "");
    static_assert(find_first_of(ia, ia + 8, ic, ic + 1) == ia + 8, "");
    static_assert(find_first_of(ia, ia + 8, ic, ic) == ia + 8, "");
    static_assert(find_first_of(ia, ia, ic, ic + 1) == ia, "");

    // For constexpr subrange, prefer using std::array or iterator pairs.
    // The make_subrange of a raw array may not be constexpr in C++17.
}