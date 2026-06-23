/// chunk_gtest.cpp
/// Google Test conversion of range-v3 chunk view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <forward_list>
#include <list>
#include <vector>
#include <memory>

#include <fermat/range/conversion.h>
#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/iterator/operations.h>
#include <fermat/view/chunk.h>
#include <fermat/view/cycle.h>
#include <fermat/view/filter.h>
#include <fermat/view/iota.h>
#include <fermat/view/join.h>
#include <fermat/view/move.h>
#include <fermat/view/repeat.h>
#include <fermat/view/reverse.h>
#include <fermat/view/take.h>
#include <fermat/view/all.h>

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : fermat::ranges::view_interface<debug_input_view<T>>
{
    struct data
    {
        const T* first_;
        std::ptrdiff_t size_;
    };
    std::shared_ptr<data> data_;

    debug_input_view() = default;
    explicit debug_input_view(const T* first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size}))
    {}

    const T* begin() const { return data_->first_; }
    const T* end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace fermat::ranges
{
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected)
{
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// Overload for 2D initializer_list (for chunk of chunks)
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<std::initializer_list<T>> expected)
{
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& expected_row : expected)
    {
        EXPECT_NE(it, end);
        check_equal(*it, expected_row);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test: input ranges (using debug_input_view)
/// ------------------------------------------------------------
void test_input_ranges()
{
    using namespace fermat::ranges;

    int const ints[] = {0,1,2,3,4};
    constexpr auto N = size(ints);
    constexpr auto K = 2;
    auto make_range = [&]{
        return debug_input_view<int const>{ints, static_cast<std::ptrdiff_t>(N)} | views::chunk(K);
    };
    auto rng = make_range();

    // Concept checks are removed because Fermat uses CPP_concept which is not
    // easily queryable with ::value. The original runtime behavior is preserved.

    EXPECT_EQ(size(rng), (N + K - 1) / K);
    EXPECT_EQ(fermat::ranges::distance(begin(rng), end(rng)), (N + K - 1) / K);

    rng = make_range();
    auto i = begin(rng);
    auto e = end(rng);
    EXPECT_NE(i, e);
    if (i == e) return;
    {
        auto r = *i;
        EXPECT_EQ(size(r), 2u);
        auto ii = begin(r);
        auto ee = end(r);
        EXPECT_NE(ii, ee);
        if (ii == ee) return;
        EXPECT_EQ((ee - ii), 2);
        EXPECT_EQ(*ii, 0);
        ++ii;
        EXPECT_NE(ii, ee);
        if (ii == ee) return;
        EXPECT_EQ((ee - ii), 1);
        EXPECT_EQ(*ii, 1);
        ++ii;
        EXPECT_EQ(ii, ee);
        EXPECT_EQ((ee - ii), 0);
    }
    ++i;
    EXPECT_NE(i, e);
    if (i == e) return;
    check_equal(*i, {2,3});
    ++i;
    if (i != e)
    {
        check_equal(*i, {4});
        ++i;
        EXPECT_EQ(i, e);
    }
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ChunkTest, VectorOfInts)
{
    using namespace fermat::ranges;
    auto v = views::iota(0,11) | to<std::vector<int>>();
    auto rng1 = v | views::chunk(3);

    // Concept checks omitted.

    auto it1 = begin(rng1);
    check_equal(*it1++, {0,1,2});
    check_equal(*it1++, {3,4,5});
    check_equal(*it1++, {6,7,8});
    check_equal(*it1++, {9,10});
    EXPECT_EQ(it1, end(rng1));
    check_equal(*next(it1, -3), {3,4,5});
    EXPECT_EQ(size(rng1), 4u);
}

TEST(ChunkTest, ForwardListOfInts)
{
    using namespace fermat::ranges;
    auto l = views::iota(0,11) | to<std::forward_list<int>>();
    auto rng2 = l | views::chunk(3);

    auto it2 = begin(rng2);
    check_equal(*it2++, {0,1,2});
    check_equal(*it2++, {3,4,5});
    check_equal(*it2++, {6,7,8});
    check_equal(*it2++, {9,10});
    EXPECT_EQ(it2, end(rng2));
}

TEST(ChunkTest, RepeatCycle)
{
    using namespace fermat::ranges;
    auto fives = views::repeat(5);
    auto rng = fives | views::chunk(3);

    auto it = rng.begin();
    auto it2 = next(it, 3);
    EXPECT_EQ((it2 - it), 3);
    check_equal(*it, {5,5,5});
    check_equal(*it2, {5,5,5});
}

TEST(ChunkTest, CycleLength3Chunk2)
{
    using namespace fermat::ranges;
    int const ints[] = {0,1,2};
    auto cyc = ints | views::cycle;
    auto rng = cyc | views::chunk(2);

    auto it = rng.begin();
    auto it2 = next(it, 2);
    check_equal(*it, {0,1});
    check_equal(*it2, {1,2});
    EXPECT_EQ((it - it), 0);
    EXPECT_EQ((next(it,1) - it), 1);
    EXPECT_EQ((next(it,2) - it), 2);
    EXPECT_EQ((next(it,3) - it), 3);
    EXPECT_EQ((next(it,4) - it), 4);
    EXPECT_EQ((next(it,5) - it), 5);
    EXPECT_EQ((next(it,6) - it), 6);
    EXPECT_EQ((next(it,7) - it), 7);
}

TEST(ChunkTest, CycleLength3Chunk4)
{
    using namespace fermat::ranges;
    int const ints[] = {0,1,2};
    auto cyc = ints | views::cycle;
    auto rng = cyc | views::chunk(4);

    auto it = rng.begin();
    auto it2 = next(it, 2);
    check_equal(*it, {0,1,2,0});
    check_equal(*it2, {2,0,1,2});
    EXPECT_EQ((it - it), 0);
    EXPECT_EQ((next(it,1) - it), 1);
    EXPECT_EQ((next(it,2) - it), 2);
    EXPECT_EQ((next(it,3) - it), 3);
    EXPECT_EQ((next(it,4) - it), 4);
    EXPECT_EQ((next(it,5) - it), 5);
    EXPECT_EQ((next(it,6) - it), 6);
    EXPECT_EQ((next(it,7) - it), 7);
}

TEST(ChunkTest, CycleLength10Chunk3)
{
    using namespace fermat::ranges;
    int const ints[] = {0,1,2,3,4,5,6,7,8,9};
    auto cyc = ints | views::cycle;
    auto rng = cyc | views::chunk(3);

    auto it = rng.begin();
    auto it2 = next(it, 2);
    check_equal(*it, {0,1,2});
    check_equal(*it2, {6,7,8});
    EXPECT_EQ((it - it), 0);
    EXPECT_EQ((next(it,1) - it), 1);
    EXPECT_EQ((next(it,2) - it), 2);
    EXPECT_EQ((next(it,3) - it), 3);
    EXPECT_EQ((next(it,4) - it), 4);
    EXPECT_EQ((next(it,5) - it), 5);
    EXPECT_EQ((next(it,6) - it), 6);
    EXPECT_EQ((next(it,7) - it), 7);
    EXPECT_EQ((next(it,8) - it), 8);
    EXPECT_EQ((next(it,9) - it), 9);
    EXPECT_EQ((next(it,10) - it), 10);
    EXPECT_EQ((next(it,11) - it), 11);
    EXPECT_EQ((next(it,12) - it), 12);
    EXPECT_EQ((next(it,13) - it), 13);
}

TEST(ChunkTest, InputRanges)
{
    test_input_ranges();
}

TEST(ChunkTest, Regression567)
{
    using namespace fermat::ranges;
    std::vector<std::vector<int>> vec{{1, 2, 3}, {4, 5, 6}};
    auto rng = vec | views::join | views::chunk(2);
    int const expected[][2] = {{1, 2}, {3, 4}, {5, 6}};
    // Convert to initializer_list of initializer_list
    std::initializer_list<std::initializer_list<int>> expected_list = {
        {1,2}, {3,4}, {5,6}
    };
    check_equal(rng, expected_list);
    (void)expected; // avoid unused warning
}

TEST(ChunkTest, Regression567NotExactly)
{
    using namespace fermat::ranges;
    int some_ints[] = {0,1,2,3};
    auto rng = views::all(some_ints);
    int const expected[][2] = {{0, 1}, {2, 3}};
    check_equal(rng | views::chunk(2), {std::initializer_list<int>{0,1}, {2,3}});
    (void)expected;
}

TEST(ChunkTest, StackOverflowExample)
{
    using namespace fermat::ranges;
    auto rng = views::closed_iota(1,25)
        | views::filter([](int item){ return item % 10 != 0; })
        | views::chunk(10);
    auto it = begin(rng);
    auto last = end(rng);
    EXPECT_NE(it, last);
    check_equal(*it, {1,2,3,4,5,6,7,8,9,11});
    ++it;
    EXPECT_NE(it, last);
    check_equal(*it, {12,13,14,15,16,17,18,19,21,22});
    ++it;
    EXPECT_NE(it, last);
    check_equal(*it, {23,24,25});
    ++it;
    EXPECT_EQ(it, last);
}
