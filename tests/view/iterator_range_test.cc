/// iterator_range_gtest.cpp
/// Google Test conversion of range-v3 iterator_range test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <list>
#include <utility>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/iterator_range.h>
#include <fermat/iterator/operations.h>
#include <fermat/utility/compressed_pair.h>
#include <fermat/iterator/unreachable_sentinel.h>   // for unreachable_sentinel_t

using namespace fermat::ranges;

// Helper to avoid comparing two unreachable_sentinel_t directly
template<typename T>
bool is_unreachable(const T &) { return false; }

template<>
bool is_unreachable<unreachable_sentinel_t>(const unreachable_sentinel_t &) { return true; }

TEST(IteratorRangeTest, BasicIteratorRange) {
    std::vector<int> vi = {1, 2, 3, 4, 5};

    // make_iterator_range from begin/end
    auto r0 = make_iterator_range(vi.begin(), vi.end());
    static_assert(view_<decltype(r0)>);
    static_assert(common_range<decltype(r0)>);
    // In range-v3, iterator_range of random-access iterators is sized
    static_assert(sized_range<decltype(r0)>);
    EXPECT_EQ(r0.begin(), vi.begin());
    EXPECT_EQ(r0.end(), vi.end());

    // get<I> access
    EXPECT_EQ(get<0>(r0), vi.begin());
    EXPECT_EQ(get<1>(r0), vi.end());

    // Convert to pair
    std::pair<decltype(vi.begin()), decltype(vi.end())> p = r0;
    EXPECT_EQ(p.first, vi.begin());
    EXPECT_EQ(p.second, vi.end());

    // Construction from pair – use two-argument make_iterator_range
    auto r1 = make_iterator_range(vi.begin(), vi.end());
    EXPECT_EQ(r1.begin(), vi.begin());
    EXPECT_EQ(r1.end(), vi.end());

    // Unreachable sentinel
    auto r2 = make_iterator_range(vi.begin(), unreachable);
    EXPECT_EQ(r2.begin(), vi.begin());
    // Check that end() is unreachable_sentinel_t
    EXPECT_TRUE(is_unreachable(r2.end()));
    // Iterate over finite part
    auto it = r2.begin();
    for (int i = 0; i < 5; ++i, ++it) {
        EXPECT_EQ(*it, vi[i]);
    }

    // Conversion to pair with unreachable sentinel
    auto p2 = static_cast<std::pair<decltype(vi.begin()), unreachable_sentinel_t>>(r2);
    EXPECT_EQ(p2.first, vi.begin());
    static_assert(std::is_same_v<decltype(p2.second), unreachable_sentinel_t>);

    // sized_iterator_range with list (bidirectional, not random-access)
    std::list<int> li = {10, 20, 30, 40, 50};
    sized_iterator_range<std::list<int>::iterator> r3(li.begin(), li.end(), li.size());
    EXPECT_EQ(r3.begin(), li.begin());
    EXPECT_EQ(r3.end(), li.end());
    EXPECT_EQ(r3.size(), li.size());

    int sum = 0;
    for (int x: r3) sum += x;
    EXPECT_EQ(sum, 150);
}

TEST(IteratorRangeTest, EmptyRange) {
    std::vector<int> empty;
    auto r = make_iterator_range(empty.begin(), empty.end());
    EXPECT_EQ(r.begin(), r.end());
    EXPECT_EQ(get<0>(r), empty.begin());
    EXPECT_EQ(get<1>(r), empty.end());
}

TEST(IteratorRangeTest, ConstIterators) {
    const std::vector<int> vi = {1, 2, 3};
    auto r = make_iterator_range(vi.begin(), vi.end());
    // The iterators must compare equal to the original ones.
    // We don't require exact type match (the implementation may wrap them).
    EXPECT_EQ(r.begin(), vi.begin());
    EXPECT_EQ(r.end(), vi.end());
    // Also verify that we can read elements.
    EXPECT_EQ(*r.begin(), vi[0]);
}
