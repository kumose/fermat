/// tail_gtest.cpp
/// Google Test conversion of range-v3 tail view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include <fermat/range/access.h>                 /// fermat::ranges::begin, fermat::ranges::end, fermat::ranges::size, fermat::ranges::empty
#include <fermat/range/primitives.h>             /// fermat::ranges::empty (additional)
#include <fermat/view/tail.h>                    /// views::tail
#include <fermat/view/empty.h>                   /// views::empty
#include <fermat/view/single.h>                  /// views::single
#include <fermat/view/istream.h>                 /// istream_view

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(TailTest, VectorNonEmpty) {
    using namespace fermat::ranges;

    std::vector<int> v{0,1,2,3};
    auto rng = views::tail(v);
    check_equal(rng, {1,2,3});
    EXPECT_EQ(size(rng), 3u);
}

TEST(TailTest, VectorEmpty) {
    using namespace fermat::ranges;

    std::vector<int> v{};
    auto rng = views::tail(v);
    EXPECT_TRUE(empty(rng));
    EXPECT_EQ(size(rng), 0u);
}

TEST(TailTest, IstreamNonEmpty) {
    using namespace fermat::ranges;

    std::stringstream sin{"1 2 3 4"};
    istream_view<int> is(sin);
    auto rng = views::tail(is);
    check_equal(rng, {2,3,4});
}

TEST(TailTest, IstreamEmpty) {
    using namespace fermat::ranges;

    std::stringstream sin{""};
    istream_view<int> is(sin);
    auto rng = views::tail(is);
    EXPECT_EQ(rng.begin(), rng.end());
}

TEST(TailTest, EmptyView) {
    using namespace fermat::ranges;

    auto rng = views::empty<int> | views::tail;
    static_assert(size(rng) == 0, "size must be 0");
    static_assert(std::is_same<empty_view<int>, decltype(rng)>::value,
                  "tail of empty view must be empty_view");
}

TEST(TailTest, ConstEmptyView) {
    using namespace fermat::ranges;

    tail_view<empty_view<int>> const rng(views::empty<int>);
    static_assert(size(rng) == 0, "size must be 0");
}

TEST(TailTest, SingleView) {
    using namespace fermat::ranges;

    auto const rng = views::single(1) | views::tail;
    static_assert(size(rng) == 0, "tail of single element must be empty");
}
