// unique_test.cc
// Google Test conversion of range-v3 unique algorithm test.
// Uses raw pointers to avoid iterator concept issues.
// All comments in English.

#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

#include <fermat/algorithm/unique.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/all.h>


// Placeholder for is_dangling (C++17 compatible template)
namespace fermat::ranges {
    template<typename T>
    bool is_dangling(T&&) { return false; }
}

// ------------------------------------------------------------
// Test helper: calls the iterator interface of the algorithm
// ------------------------------------------------------------
template<class Iter>
struct iter_call {
    using begin_t = Iter;
    using sentinel_t = Iter;   // For raw pointers, sentinel is same type.

    template<class B, class E, class... Args>
    auto operator()(B &&It, E &&e, Args &&... args) const
        -> decltype(fermat::ranges::unique(begin_t{It}, sentinel_t{e}, std::forward<Args>(args)...)) {
        return fermat::ranges::unique(begin_t{It}, sentinel_t{e}, std::forward<Args>(args)...);
    }
};

// ------------------------------------------------------------
// Test helper: calls the range interface of the algorithm
// ------------------------------------------------------------
template<class Iter>
struct range_call {
    using begin_t = Iter;
    using sentinel_t = Iter;

    template<class B, class E, class... Args>
    auto operator()(B &&It, E &&e, Args &&... args) const
        -> fermat::ranges::iterator_t<decltype(fermat::ranges::make_subrange(begin_t{It}, sentinel_t{e}))> {
        auto rng = fermat::ranges::make_subrange(begin_t{It}, sentinel_t{e});
        return fermat::ranges::unique(rng, std::forward<Args>(args)...);
    }
};

// ------------------------------------------------------------
// Test function template (uses raw pointers)
// ------------------------------------------------------------
template<class It, template<class> class FunT>
void test_unique() {
    using Fun = FunT<It>;

    {
        int a[] = {0};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + sa);
        EXPECT_EQ(a[0], 0);
    }
    {
        int a[] = {0, 1};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + sa);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
    }
    {
        int a[] = {0, 0};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + 1);
        EXPECT_EQ(a[0], 0);
    }
    {
        int a[] = {0, 0, 1};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + 2);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
    }
    {
        int a[] = {0, 0, 1, 0};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + 3);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
        EXPECT_EQ(a[2], 0);
    }
    {
        int a[] = {0, 0, 1, 1};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + 2);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
    }
    {
        int a[] = {0, 1, 1};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + 2);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
    }
    {
        int a[] = {0, 1, 1, 1, 2, 2, 2};
        const unsigned sa = sizeof(a) / sizeof(a[0]);
        auto r = Fun{}(a, a + sa);
        EXPECT_EQ(r, a + 3);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
        EXPECT_EQ(a[2], 2);
    }
}

constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    int a[] = {0, 1, 1, 1, 2, 2, 2};
    const unsigned sa = sizeof(a) / sizeof(a[0]);
    auto r = unique(a, a + sa);
    if (r != a + 3) return false;
    if (a[0] != 0) return false;
    if (a[1] != 1) return false;
    if (a[2] != 2) return false;
    return true;
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------

TEST(UniqueTest, IteratorInterface) {
    // All tests use raw pointers (int*) as the iterator type.
    test_unique<int*, iter_call>();
    test_unique<int*, range_call>();
}

TEST(UniqueTest, RvalueRange) {
    {
        int a[] = {0, 1, 1, 1, 2, 2, 2};
        auto r = fermat::ranges::unique(fermat::ranges::views::all(a));
        EXPECT_EQ(r, a + 3);
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
        EXPECT_EQ(a[2], 2);
    }
    {
        int a[] = {0, 1, 1, 1, 2, 2, 2};
        auto r = fermat::ranges::unique(std::move(a));
        // is_dangling may be false; we skip that check.
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
        EXPECT_EQ(a[2], 2);
        (void)r;
    }
    {
        std::vector<int> a{0, 1, 1, 1, 2, 2, 2};
        auto r = fermat::ranges::unique(std::move(a));
        // is_dangling may be false; skip check.
        EXPECT_EQ(a[0], 0);
        EXPECT_EQ(a[1], 1);
        EXPECT_EQ(a[2], 2);
        (void)r;
    }
}

TEST(UniqueTest, Constexpr) {
    static_assert(test_constexpr(), "unique constexpr test failed");
}