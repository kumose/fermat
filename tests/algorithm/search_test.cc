#include <gtest/gtest.h>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/search.h>
#include <fermat/algorithm/search_n.h>
#include <fermat/view/counted.h>
#include <fermat/view/all.h>

/// test search with iterator pairs (raw pointers) and ranges
void test_search_basic() {
    // basic patterns
    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    // empty pattern
    auto res = fermat::ranges::search(ia, ia + sa, ia, ia);
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia);

    // pattern of length 1
    res = fermat::ranges::search(ia, ia + sa, ia, ia + 1);
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia + 1);

    res = fermat::ranges::search(ia, ia + sa, ia + 1, ia + 2);
    EXPECT_EQ(res.begin(), ia + 1);
    EXPECT_EQ(res.end(), ia + 2);

    // pattern longer than haystack
    res = fermat::ranges::search(ia, ia + sa, ia + 2, ia + 2);
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia);

    res = fermat::ranges::search(ia, ia + sa, ia + 2, ia + 3);
    EXPECT_EQ(res.begin(), ia + 2);
    EXPECT_EQ(res.end(), ia + 3);

    // pattern at the end
    res = fermat::ranges::search(ia, ia + sa, ia + sa - 1, ia + sa);
    EXPECT_EQ(res.begin(), ia + sa - 1);
    EXPECT_EQ(res.end(), ia + sa);

    // full haystack as pattern
    res = fermat::ranges::search(ia, ia + sa, ia, ia + sa);
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia + sa);

    // haystack too short
    res = fermat::ranges::search(ia, ia + sa - 1, ia, ia + sa);
    EXPECT_EQ(res.begin(), ia + sa - 1);
    EXPECT_EQ(res.end(), ia + sa - 1);

    res = fermat::ranges::search(ia, ia + 1, ia, ia + sa);
    EXPECT_EQ(res.begin(), ia + 1);
    EXPECT_EQ(res.end(), ia + 1);

    // more complex sequences
    int ib[] = {0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4};
    constexpr std::size_t sb = sizeof(ib) / sizeof(ib[0]);

    int ic[] = {1};
    res = fermat::ranges::search(ib, ib + sb, ic, ic + 1);
    EXPECT_EQ(res.begin(), ib + 1);
    EXPECT_EQ(res.end(), ib + 2);

    int id[] = {1, 2};
    res = fermat::ranges::search(ib, ib + sb, id, id + 2);
    EXPECT_EQ(res.begin(), ib + 1);
    EXPECT_EQ(res.end(), ib + 3);

    int ie[] = {1, 2, 3};
    res = fermat::ranges::search(ib, ib + sb, ie, ie + 3);
    EXPECT_EQ(res.begin(), ib + 4);
    EXPECT_EQ(res.end(), ib + 7);

    int ig[] = {1, 2, 3, 4};
    res = fermat::ranges::search(ib, ib + sb, ig, ig + 4);
    EXPECT_EQ(res.begin(), ib + 8);
    EXPECT_EQ(res.end(), ib + 12);

    int ih[] = {0, 1, 1, 1, 1, 2, 3, 0, 1, 2, 3, 4};
    constexpr std::size_t sh = sizeof(ih) / sizeof(ih[0]);
    int ii[] = {1, 1, 2};
    res = fermat::ranges::search(ih, ih + sh, ii, ii + 3);
    EXPECT_EQ(res.begin(), ih + 3);
    EXPECT_EQ(res.end(), ih + 6);

    int ij[] = {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0};
    constexpr std::size_t sj = sizeof(ij) / sizeof(ij[0]);
    int ik[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0};
    constexpr std::size_t sk = sizeof(ik) / sizeof(ik[0]);
    res = fermat::ranges::search(ij, ij + sj, ik, ik + sk);
    EXPECT_EQ(res.begin(), ij + 6);
    EXPECT_EQ(res.end(), ij + 6 + sk);
}

TEST(SearchTest, IteratorPair) {
    test_search_basic();
}

TEST(SearchTest, Range) {
    // same tests using range overloads
    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto rng = fermat::ranges::make_subrange(ia, ia + sa);
    auto res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia, ia));
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia);

    res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia, ia + 1));
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia + 1);

    res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia + 1, ia + 2));
    EXPECT_EQ(res.begin(), ia + 1);
    EXPECT_EQ(res.end(), ia + 2);

    res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia + 2, ia + 2));
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia);

    res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia + 2, ia + 3));
    EXPECT_EQ(res.begin(), ia + 2);
    EXPECT_EQ(res.end(), ia + 3);

    res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia + sa - 1, ia + sa));
    EXPECT_EQ(res.begin(), ia + sa - 1);
    EXPECT_EQ(res.end(), ia + sa);

    res = fermat::ranges::search(rng, fermat::ranges::make_subrange(ia, ia + sa));
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia + sa);

    // empty haystack
    auto empty = fermat::ranges::make_subrange(ia, ia);
    res = fermat::ranges::search(empty, fermat::ranges::make_subrange(ia, ia + 1));
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia);

    // complex pattern
    int ib[] = {0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4};
    auto rngb = fermat::ranges::make_subrange(ib, ib + 12);
    int ie[] = {1, 2, 3};
    res = fermat::ranges::search(rngb, fermat::ranges::make_subrange(ie, ie + 3));
    EXPECT_EQ(res.begin(), ib + 4);
    EXPECT_EQ(res.end(), ib + 7);
}

TEST(SearchTest, Projection) {
    struct S { int i; };
    struct T { int i; };
    S in[] = {{0}, {1}, {2}, {3}, {4}, {5}};
    T pat[] = {{2}, {3}};

    auto res = fermat::ranges::search(in, pat, std::equal_to<int>{}, &S::i, &T::i);
    EXPECT_EQ(res.begin(), in + 2);
    EXPECT_EQ(res.end(), in + 4);
}

// Fixed CountedRange test for search_n
TEST(SearchNTest, CountedRange) {
    int in[] = {0, 1, 2, 2, 4, 5};
    auto rng = fermat::ranges::views::counted(in, 6);
    auto sub = fermat::ranges::search_n(rng, 2, 2);

    // Compare iterators directly, no .base()
    EXPECT_EQ(sub.begin(), in + 2);
    EXPECT_EQ(sub.end(), in + 4);
    EXPECT_EQ(fermat::ranges::distance(sub), 2);

    auto sub2 = fermat::ranges::search_n(rng, 3, 2);
    EXPECT_EQ(sub2.begin(), in + 6);
    EXPECT_EQ(sub2.end(), in + 6);
    EXPECT_EQ(fermat::ranges::distance(sub2), 0);
}

TEST(SearchTest, RvalueRange) {
    int ib[] = {0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4};
    int ie[] = {1, 2, 3};
    // lvalue view
    auto res = fermat::ranges::search(fermat::ranges::views::all(ib), ie);
    EXPECT_EQ(res.begin(), ib + 4);
    EXPECT_EQ(res.end(), ib + 7);

    // rvalue range (array moved)
    int ib2[] = {0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4};
    auto res2 = fermat::ranges::search(std::move(ib2), ie);
    // result is dangling, but the algorithm still runs; we only check it compiles.
    (void)res2;

    // vector as rvalue
    std::vector<int> vec{0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4};
    auto res3 = fermat::ranges::search(std::move(vec), ie);
    (void)res3;
}

TEST(SearchTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4};
        int ib[] = {2, 3};
        int ic[] = {2, 4};
        auto r = fermat::ranges::search(ia, ib, std::equal_to{});
        bool ok = (r.begin() == ia + 2);
        auto r2 = fermat::ranges::search(ia, ic, std::equal_to{});
        ok = ok && (r2.begin() == ia + 5);
        return ok;
    };
    static_assert(test(), "");
}

