#include <gtest/gtest.h>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/search_n.h>
#include <fermat/view/counted.h>
#include <fermat/view/all.h>

/// test search_n with iterator pairs (raw pointers)
void test_search_n_iter() {
    int a[] = {0, 1, 2, 3, 4, 5};
    const std::size_t N = sizeof(a)/sizeof(a[0]);

    // count = 0 always matches at beginning
    auto res = ranges::search_n(a, a + N, 0, 0);
    EXPECT_EQ(res.begin(), a);
    EXPECT_EQ(res.end(), a);

    // count = 1, value = 0
    res = ranges::search_n(a, a + N, 1, 0);
    EXPECT_EQ(res.begin(), a);
    EXPECT_EQ(res.end(), a + 1);

    // count = 2, value = 0 (not present)
    res = ranges::search_n(a, a + N, 2, 0);
    EXPECT_EQ(res.begin(), a + N);
    EXPECT_EQ(res.end(), a + N);

    // count = N, value = 0 (not enough)
    res = ranges::search_n(a, a + N, N, 0);
    EXPECT_EQ(res.begin(), a + N);
    EXPECT_EQ(res.end(), a + N);

    // count = 0, value = 3
    res = ranges::search_n(a, a + N, 0, 3);
    EXPECT_EQ(res.begin(), a);
    EXPECT_EQ(res.end(), a);

    // count = 1, value = 3
    res = ranges::search_n(a, a + N, 1, 3);
    EXPECT_EQ(res.begin(), a + 3);
    EXPECT_EQ(res.end(), a + 4);

    // count = 2, value = 3 (only one)
    res = ranges::search_n(a, a + N, 2, 3);
    EXPECT_EQ(res.begin(), a + N);
    EXPECT_EQ(res.end(), a + N);

    // count = 1, value = 5 (last element)
    res = ranges::search_n(a, a + N, 1, 5);
    EXPECT_EQ(res.begin(), a + 5);
    EXPECT_EQ(res.end(), a + 6);

    // count = 2, value = 5 (only one)
    res = ranges::search_n(a, a + N, 2, 5);
    EXPECT_EQ(res.begin(), a + N);
    EXPECT_EQ(res.end(), a + N);

    // more complex array with repetitions
    int b[] = {0, 0, 1, 1, 2, 2};
    const std::size_t M = sizeof(b)/sizeof(b[0]);

    // count = 2, value = 0
    res = ranges::search_n(b, b + M, 2, 0);
    EXPECT_EQ(res.begin(), b);
    EXPECT_EQ(res.end(), b + 2);

    // count = 3, value = 0 (not enough)
    res = ranges::search_n(b, b + M, 3, 0);
    EXPECT_EQ(res.begin(), b + M);
    EXPECT_EQ(res.end(), b + M);

    // count = 2, value = 1
    res = ranges::search_n(b, b + M, 2, 1);
    EXPECT_EQ(res.begin(), b + 2);
    EXPECT_EQ(res.end(), b + 4);

    // count = 2, value = 2
    res = ranges::search_n(b, b + M, 2, 2);
    EXPECT_EQ(res.begin(), b + 4);
    EXPECT_EQ(res.end(), b + 6);

    // all zeros
    int c[] = {0, 0, 0};
    const std::size_t L = sizeof(c)/sizeof(c[0]);

    res = ranges::search_n(c, c + L, 1, 0);
    EXPECT_EQ(res.begin(), c);
    EXPECT_EQ(res.end(), c + 1);

    res = ranges::search_n(c, c + L, 2, 0);
    EXPECT_EQ(res.begin(), c);
    EXPECT_EQ(res.end(), c + 2);

    res = ranges::search_n(c, c + L, 3, 0);
    EXPECT_EQ(res.begin(), c);
    EXPECT_EQ(res.end(), c + 3);

    res = ranges::search_n(c, c + L, 4, 0);
    EXPECT_EQ(res.begin(), c + L);
    EXPECT_EQ(res.end(), c + L);
}

TEST(SearchNTest, IteratorPair) {
    test_search_n_iter();
}

/// test search_n with range (subrange)
void test_search_n_range() {
    int a[] = {0, 1, 2, 3, 4, 5};
    const std::size_t N = sizeof(a)/sizeof(a[0]);
    auto rng = ranges::make_subrange(a, a + N);

    auto res = ranges::search_n(rng, 1, 0);
    EXPECT_EQ(res.begin(), a);
    EXPECT_EQ(res.end(), a + 1);

    res = ranges::search_n(rng, 2, 0);
    EXPECT_EQ(res.begin(), a + N);
    EXPECT_EQ(res.end(), a + N);

    res = ranges::search_n(rng, 1, 3);
    EXPECT_EQ(res.begin(), a + 3);
    EXPECT_EQ(res.end(), a + 4);

    int b[] = {0, 0, 1, 1, 2, 2};
    auto rng2 = ranges::make_subrange(b, b + 6);
    res = ranges::search_n(rng2, 2, 1);
    EXPECT_EQ(res.begin(), b + 2);
    EXPECT_EQ(res.end(), b + 4);
}

TEST(SearchNTest, Range) {
    test_search_n_range();
}

/// projection test
struct S { int i; };

TEST(SearchNTest, Projection) {
    S arr[] = {{0}, {1}, {2}, {2}, {4}, {5}};
    auto res = ranges::search_n(arr, 2, 2, std::equal_to<int>{}, &S::i);
    EXPECT_EQ(res.begin(), arr + 2);
    EXPECT_EQ(res.end(), arr + 4);
}

/// counted range test
// Fixed CountedRange test for search_n
TEST(SearchNTest, CountedRange) {
    int in[] = {0, 1, 2, 2, 4, 5};
    auto rng = ranges::views::counted(in, 6);
    auto sub = ranges::search_n(rng, 2, 2);

    // Compare iterators directly (they are raw pointers), no .base()
    EXPECT_EQ(sub.begin(), in + 2);
    EXPECT_EQ(sub.end(), in + 4);
    // Use ranges::distance to get the length of the subrange
    EXPECT_EQ(ranges::distance(sub), 2);

    auto sub2 = ranges::search_n(rng, 3, 2);
    EXPECT_EQ(sub2.begin(), in + 6);
    EXPECT_EQ(sub2.end(), in + 6);
    EXPECT_EQ(ranges::distance(sub2), 0);
}
/// rvalue range test (compile only; dangling is not checked)
TEST(SearchNTest, RvalueRange) {
    int ib[] = {0, 0, 1, 1, 2, 2};
    // lvalue view
    auto res = ranges::search_n(ranges::views::all(ib), 2, 1);
    EXPECT_EQ(res.begin(), ib + 2);
    EXPECT_EQ(res.end(), ib + 4);

    // rvalue array (moved)
    int ib2[] = {0, 0, 1, 1, 2, 2};
    auto res2 = ranges::search_n(std::move(ib2), 2, 1);
    // res2 is dangling; we only check that the algorithm runs (no runtime check)
    (void)res2;

    // rvalue vector
    std::vector<int> vec{0, 0, 1, 1, 2, 2};
    auto res3 = ranges::search_n(std::move(vec), 2, 1);
    (void)res3;
}

/// constexpr test
TEST(SearchNTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int arr[] = {0, 1, 2, 2, 4, 5};
        auto res = ranges::search_n(arr, 2, 2, std::equal_to<int>{});
        return res.begin() == arr + 2;
    };
    static_assert(test(), "");
}
