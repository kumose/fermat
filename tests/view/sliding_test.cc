/// sliding_gtest.cpp
/// Google Test conversion of range-v3 sliding view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <forward_list>
#include <list>
#include <vector>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>          /// ranges::to
#include <fermat/iterator/operations.h>        /// ranges::next, ranges::prev
#include <fermat/utility/copy.h>               /// ranges::copy (if needed)
#include <fermat/view/sliding.h>               /// views::sliding
#include <fermat/view/cycle.h>                 /// views::cycle
#include <fermat/view/iota.h>                  /// views::iota, views::repeat, views::repeat_n
#include <fermat/view/repeat.h>                /// views::repeat
#include <fermat/view/repeat_n.h>              /// views::repeat_n
#include <fermat/view/reverse.h>               /// views::reverse
#include <fermat/view/zip.h>                   /// views::zip
#include <fermat/view/chunk_by.h>              /// views::chunk_by

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list (single elements)
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for checking a sliding window sub‑range (which is itself a view)
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<std::initializer_list<T>> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& expected_window : expected) {
        EXPECT_NE(it, end);
        // Compare element by element
        auto sub_it = ranges::begin(*it);
        auto sub_end = ranges::end(*it);
        for (auto const& val : expected_window) {
            EXPECT_NE(sub_it, sub_end);
            EXPECT_EQ(*sub_it, val);
            ++sub_it;
        }
        EXPECT_EQ(sub_it, sub_end);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Helper: test_size (compile-time sized check – simplified to runtime)
/// ------------------------------------------------------------
template<typename Adapted>
void test_size(Adapted& a, std::true_type) {
    // sized_range concept check omitted – just runtime size
    EXPECT_EQ(ranges::size(a), static_cast<std::size_t>(7 - 3 + 1));
}
template<typename Adapted>
void test_size(Adapted&, std::false_type) {}

/// Helper: test_common (concept check omitted)
template<typename Adapted>
void test_common(Adapted& a, std::true_type) {
    // common_range concept omitted
    (void)a;
}

/// Helper: test_prev (bidirectional)
template<typename Adapted>
void test_prev(Adapted& a, ranges::iterator_t<Adapted> const& it, std::true_type) {
    // bidirectional_range concept omitted
    auto prev_it = ranges::prev(it, 3);
    // Check that sliding window returned by prev(it,3) contains expected values
    // For a range of 0..6 with window size 3, prev(it,3) corresponds to window starting at index 2
    // Expected values: indices 2,3,4 -> values 2,3,4
    std::vector<int> expected = {2,3,4};
    auto window_it = ranges::begin(*prev_it);
    for (auto val : expected) {
        EXPECT_NE(window_it, ranges::end(*prev_it));
        EXPECT_EQ(*window_it, val);
        ++window_it;
    }
    EXPECT_EQ(window_it, ranges::end(*prev_it));
}
template<typename Adapted>
void test_prev(Adapted&, ranges::iterator_t<Adapted> const&, std::false_type) {}

/// ------------------------------------------------------------
/// Helper: test_finite – instantiated for different base ranges
/// ------------------------------------------------------------
template<typename BaseRange>
void test_finite(BaseRange&& v) {
    using namespace ranges;
    using Base = std::decay_t<BaseRange>;
    auto rng = v | views::sliding(3);
    using Adapted = decltype(rng);

    test_size(rng, meta::bool_<sized_range<Base>>{});
    test_common(rng, meta::bool_<common_range<Base>>{});

    auto it = ranges::begin(rng);
    // Expected sliding windows for 0..6 with size 3:
    // [0,1,2], [1,2,3], [2,3,4], [3,4,5], [4,5,6]
    std::initializer_list<std::initializer_list<int>> expected = {
        {0,1,2}, {1,2,3}, {2,3,4}, {3,4,5}, {4,5,6}
    };
    for (auto const& win : expected) {
        EXPECT_NE(it, ranges::end(rng));
        check_equal(*it, win);
        ++it;
    }
    EXPECT_EQ(it, ranges::end(rng));

    test_prev(rng, it, meta::bool_<bidirectional_range<Base>>{});
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(SlidingTest, ForwardList) {
    auto v = ranges::views::iota(0,7) | ranges::to<std::forward_list<int>>();
    test_finite(v);
}

TEST(SlidingTest, List) {
    auto v = ranges::views::iota(0,7) | ranges::to<std::list<int>>();
    test_finite(v);
}

TEST(SlidingTest, Vector) {
    auto v = ranges::views::iota(0,7) | ranges::to<std::vector<int>>();
    test_finite(v);
}

TEST(SlidingTest, IdentityRange) {
    // The original test uses identity{} to pass the iota view directly
    auto v = ranges::views::iota(0,7);   // this is a view, not a container
    test_finite(v);
}

/// Regression test for issue #975
TEST(SlidingTest, Bug975) {
    using namespace ranges;
    std::vector<double> v{2.0, 2.0, 3.0, 1.0};
    std::vector<int> i{1, 2, 1, 2};
    std::vector<int> t{1, 1, 2, 2};
    auto vals = views::zip(v, i, t);
    using T = std::tuple<double, int, int>;
    auto g = vals | views::chunk_by(
        [](T t1, T t2) {
            return std::get<2>(t1) == std::get<2>(t2);
        }
    );
    auto windows = views::sliding(g, 2);
    auto it = std::begin(windows);
    (void)it;   // just ensure it compiles
    SUCCEED();
}

TEST(SlidingTest, InfiniteRepeatCycle) {
    using namespace ranges;
    constexpr int K = 3;
    // views::repeat(5) infinite
    auto rng = views::repeat(5) | views::sliding(K);
    auto it = rng.begin();
    // Check first window
    auto window = *it;
    std::vector<int> window_vals;
    for (auto w : window) window_vals.push_back(w);
    EXPECT_EQ(window_vals, std::vector<int>(K, 5));
    // Check window after 42 steps
    auto it2 = next(it, 42);
    window = *it2;
    window_vals.clear();
    for (auto w : window) window_vals.push_back(w);
    EXPECT_EQ(window_vals, std::vector<int>(K, 5));
    EXPECT_NE(it, it2);
}

TEST(SlidingTest, CycleLengthEqualsK) {
    using namespace ranges;
    constexpr int K = 3;
    auto rng = views::iota(0, K) | views::cycle | views::sliding(K);
    auto it = rng.begin();
    // Expected pattern: [0,1,2], [1,2,0], [2,0,1], repeating
    check_equal(*it++, {0,1,2});
    check_equal(*it++, {1,2,0});
    check_equal(*it++, {2,0,1});
    // Advance further
    auto it2 = next(it, 42 * K);
    check_equal(*it2, {0,1,2});
}

TEST(SlidingTest, CycleLengthGreaterThanK) {
    using namespace ranges;
    constexpr int K = 3;
    auto rng = views::iota(0,7) | views::cycle | views::sliding(K);
    auto it = rng.begin();
    check_equal(*it, {0,1,2});
    check_equal(*next(it, 2), {2,3,4});
    check_equal(*next(it, 16), {2,3,4});   // depends on cycle length, but original uses 16
    check_equal(*next(it, 27), {6,0,1});
}
