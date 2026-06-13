/// cycle_gtest.cpp
/// Google Test conversion of range-v3 cycle view test (simplified).
/// Only runtime behavior is tested; compile-time concept checks are omitted.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <array>
#include <vector>
#include <memory>
#include <forward_list>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/iterator/operations.h>
#include <fermat/view/cycle.h>
#include <fermat/view/take.h>
#include <fermat/view/take_exactly.h>
#include <fermat/view/iota.h>
#include <fermat/view/reverse.h>
#include <fermat/view/slice.h>
#include <fermat/view/c_str.h>

/// ------------------------------------------------------------
/// Helper: check_equal for two ranges (hand‑written comparison)
/// ------------------------------------------------------------
template<typename Rng1, typename Rng2>
void check_equal(Rng1&& rng1, Rng2&& rng2) {
    auto it1 = fermat::ranges::begin(rng1);
    auto end1 = fermat::ranges::end(rng1);
    auto it2 = fermat::ranges::begin(rng2);
    auto end2 = fermat::ranges::end(rng2);
    while (it1 != end1 && it2 != end2) {
        EXPECT_EQ(*it1, *it2);
        ++it1; ++it2;
    }
    EXPECT_EQ(it1, end1);
    EXPECT_EQ(it2, end2);
}

/// Overload for initializer_list (expected)
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

/// Overload for std::array (convert to initializer_list)
template<typename Rng, typename T, std::size_t N>
void check_equal(Rng&& rng, const std::array<T, N>& expected) {
    check_equal(std::forward<Rng>(rng), std::initializer_list<T>(expected.begin(), expected.end()));
}

/// ------------------------------------------------------------
/// Test functions – only runtime, no concept checks
/// ------------------------------------------------------------
template<typename Rng>
void test_const_forward_range(Rng const& rng) {
    using namespace fermat::ranges;
    auto r = rng | views::cycle;

    // Check distances
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(0)), 0);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(1)), 1);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(2)), 2);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(3)), 3);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(4)), 4);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(6)), 6);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(7)), 7);

    // Check content using manual loops
    check_equal(r | views::take_exactly(0), std::array<int,0>{});
    check_equal(r | views::take_exactly(1), {0});
    check_equal(r | views::take_exactly(2), {0,1});
    check_equal(r | views::take_exactly(3), {0,1,2});
    check_equal(r | views::take_exactly(4), {0,1,2,0});
    check_equal(r | views::take_exactly(6), {0,1,2,0,1,2});
    check_equal(r | views::take_exactly(7), {0,1,2,0,1,2,0});

    // With take (non-exact)
    EXPECT_EQ(fermat::ranges::distance(r | views::take(0)), 0);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(1)), 1);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(2)), 2);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(3)), 3);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(4)), 4);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(6)), 6);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(7)), 7);

    check_equal(r | views::take(0), std::array<int,0>{});
    check_equal(r | views::take(1), {0});
    check_equal(r | views::take(2), {0,1});
    check_equal(r | views::take(3), {0,1,2});
    check_equal(r | views::take(4), {0,1,2,0});
    check_equal(r | views::take(6), {0,1,2,0,1,2});
    check_equal(r | views::take(7), {0,1,2,0,1,2,0});
}

template<typename Rng>
void test_const_forward_reversed_range(Rng const& rng) {
    using namespace fermat::ranges;
    test_const_forward_range(rng);

    auto r = rng | views::reverse | views::cycle;

    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(0)), 0);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(1)), 1);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(2)), 2);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(3)), 3);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(4)), 4);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(6)), 6);
    EXPECT_EQ(fermat::ranges::distance(r | views::take_exactly(7)), 7);

    check_equal(r | views::take_exactly(0), std::array<int,0>{});
    check_equal(r | views::take_exactly(1), {2});
    check_equal(r | views::take_exactly(2), {2,1});
    check_equal(r | views::take_exactly(3), {2,1,0});
    check_equal(r | views::take_exactly(4), {2,1,0,2});
    check_equal(r | views::take_exactly(6), {2,1,0,2,1,0});
    check_equal(r | views::take_exactly(7), {2,1,0,2,1,0,2});

    EXPECT_EQ(fermat::ranges::distance(r | views::take(0)), 0);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(1)), 1);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(2)), 2);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(3)), 3);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(4)), 4);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(6)), 6);
    EXPECT_EQ(fermat::ranges::distance(r | views::take(7)), 7);

    check_equal(r | views::take(0), std::array<int,0>{});
    check_equal(r | views::take(1), {2});
    check_equal(r | views::take(2), {2,1});
    check_equal(r | views::take(3), {2,1,0});
    check_equal(r | views::take(4), {2,1,0,2});
    check_equal(r | views::take(6), {2,1,0,2,1,0});
    check_equal(r | views::take(7), {2,1,0,2,1,0,2});
}

template<typename Rng>
void test_mutable_forward_range_reversed(Rng& rng) {
    using namespace fermat::ranges;
    test_const_forward_reversed_range(rng);
    int count = 2;
    for (auto&& i : rng | views::cycle | views::take_exactly(6)) {
        i = ++count;
    }
    auto result = rng | views::take_exactly(3);
    check_equal(result, {6,7,8});
}

template<typename Rng>
void test_forward_it(Rng const& rng) {
    using namespace fermat::ranges;
    auto r = rng | views::cycle;
    auto f = begin(r);
    EXPECT_EQ(*f, 0);
    auto n = next(f, 1);
    EXPECT_EQ(*n, 1);
}

template<typename Rng>
void test_bidirectional_it(Rng const& rng) {
    using namespace fermat::ranges;
    test_forward_it(rng);
    auto r = rng | views::cycle;
    auto f = begin(r);
    EXPECT_EQ(*f, 0);
    auto n = next(f, 1);
    EXPECT_EQ(*n, 1);
    auto prev = --n;
    EXPECT_EQ(prev, f);
}

template<typename Rng>
void test_random_access_it(Rng const& rng) {
    using namespace fermat::ranges;
    test_bidirectional_it(rng);
    auto r = rng | views::cycle;
    auto f = begin(r);
    auto m = begin(r) + 1;
    auto l = begin(r) + 2;
    auto f1 = begin(r) + 3;
    auto f2 = begin(r) + 6;

    EXPECT_EQ(r[0], 0);
    EXPECT_EQ(r[1], 1);
    EXPECT_EQ(r[2], 2);
    EXPECT_EQ(r[3], 0);
    EXPECT_EQ(r[4], 1);
    EXPECT_EQ(r[5], 2);

    EXPECT_EQ((f + 3), f1);
    EXPECT_EQ((f + 6), f2);
    EXPECT_EQ((f1 + 3), f2);
    EXPECT_EQ((f2 - 3), f1);
    EXPECT_EQ((f2 - 6), f);

    auto e = end(r);

    EXPECT_EQ(*f, 0);
    EXPECT_EQ(f[0], 0);
    EXPECT_EQ(f[1], 1);
    EXPECT_EQ(f[2], 2);
    EXPECT_EQ(f[3], 0);
    EXPECT_EQ(f[4], 1);
    EXPECT_EQ(f[5], 2);

    EXPECT_EQ(*m, 1);
    EXPECT_EQ(m[0], 1);
    EXPECT_EQ(m[1], 2);
    EXPECT_EQ(m[2], 0);
    EXPECT_EQ(m[3], 1);
    EXPECT_EQ(m[4], 2);
    EXPECT_EQ(m[5], 0);
    EXPECT_EQ(m[-1], 0);

    EXPECT_EQ(*l, 2);
    EXPECT_EQ(l[0], 2);
    EXPECT_EQ(l[1], 0);
    EXPECT_EQ(l[2], 1);
    EXPECT_EQ(l[3], 2);
    EXPECT_EQ(l[4], 0);
    EXPECT_EQ(l[5], 1);
    EXPECT_EQ(l[-1], 1);
    EXPECT_EQ(l[-2], 0);

    EXPECT_NE(f, e);

    auto cur = f;
    for (int i = 0; i < 100; ++i, ++cur) {
        EXPECT_EQ((next(begin(r), i) - f), i);
        EXPECT_EQ((cur - f), i);
        if (i > 0) {
            EXPECT_EQ((cur - m), i - 1);
        }
        if (i > 1) {
            EXPECT_EQ((cur - l), i - 2);
        }
    }
}

// ------------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------------

TEST(CycleTest, InitializerList) {
    auto il = {0, 1, 2};
    test_random_access_it(il);
    test_const_forward_reversed_range(il);

    const auto cil = {0, 1, 2};
    test_random_access_it(cil);
    test_const_forward_reversed_range(cil);
}

TEST(CycleTest, StdArray) {
    std::array<int, 3> a = {{0, 1, 2}};
    test_random_access_it(a);
    test_mutable_forward_range_reversed(a);

    const std::array<int, 3> ca = {{0, 1, 2}};
    test_random_access_it(ca);
    test_const_forward_reversed_range(ca);
}

TEST(CycleTest, StdList) {
    std::list<int> l = {0, 1, 2};
    test_bidirectional_it(l);
    test_mutable_forward_range_reversed(l);

    const std::list<int> cl = {0, 1, 2};
    test_bidirectional_it(cl);
    test_const_forward_reversed_range(cl);
}

TEST(CycleTest, StdForwardList) {
    std::forward_list<int> l = {0, 1, 2};
    test_forward_it(l);
    test_const_forward_range(l);

    const std::forward_list<int> cl = {0, 1, 2};
    test_forward_it(cl);
    test_const_forward_range(cl);
}

TEST(CycleTest, MoveOnlyTypes) {
    using namespace fermat::ranges;
    std::array<std::unique_ptr<int>, 3> a = {
        std::unique_ptr<int>(new int(0)),
        std::unique_ptr<int>(new int(1)),
        std::unique_ptr<int>(new int(2))
    };
    auto r = a | views::cycle;
    auto b = iter_move(r.begin() + 4);
    EXPECT_EQ(*b, 1);
}

TEST(CycleTest, Infinite) {
    using namespace fermat::ranges;
    int count = 0;
    auto il = {0, 1, 2};
    int v = 10;
    for (auto&& i : il | views::cycle) {
        if (count == 42) { break; }
        v = i;
        ++count;
    }
    EXPECT_EQ(count, 42);
    EXPECT_EQ(v, 2);
}

TEST(CycleTest, NonBounded) {
    using namespace fermat::ranges;
    auto sz = views::c_str((char const*)"hi! ");
    auto cycled = sz | views::cycle;
    auto taken = cycled | views::take(10);
    std::vector<char> result;
    for (char c : taken) result.push_back(c);
    EXPECT_EQ(result, std::vector<char>({'h','i','!',' ','h','i','!',' ','h','i'}));

    auto rng = sz | views::cycle;
    auto it = fermat::ranges::begin(rng);
    EXPECT_EQ(*it, 'h');
    EXPECT_EQ(*++it, 'i');
    EXPECT_EQ(*++it, '!');
    EXPECT_EQ(*++it, ' ');
    EXPECT_EQ(*++it, 'h');
    EXPECT_EQ(*--it, ' ');
    EXPECT_EQ(*--it, '!');
    EXPECT_EQ(*--it, 'i');
    EXPECT_EQ(*--it, 'h');

    rng = sz | views::cycle;
    it = fermat::ranges::begin(rng);
    it += 4;
    EXPECT_EQ(*it, 'h');
}

TEST(CycleTest, CycleOfInfiniteRange) {
    using namespace fermat::ranges;
    auto view = views::iota(0) | views::cycle;
    EXPECT_EQ(view[5], 5);
}

TEST(CycleTest, CycleWithSlice) {
    using namespace fermat::ranges;
    const auto length = 512;
    const auto k = 16;

    std::vector<int> input(length);
    auto output = views::cycle(input)
                | views::slice(length + k, 2 * length + k);

    EXPECT_NE(begin(output), end(output));
    EXPECT_EQ(size(output), 512u);
    EXPECT_EQ(fermat::ranges::distance(output), 512);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
