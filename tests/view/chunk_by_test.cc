/// chunk_by_gtest.cpp
/// Google Test conversion of range-v3 chunk_by view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <forward_list>
#include <string>

#include <fermat/range/access.h>
#include <fermat/view/chunk_by.h>
#include <fermat/view/counted.h>
#include <fermat/view/cycle.h>
#include <fermat/view/remove_if.h>
#include <fermat/view/take.h>

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

/// Overload for ranges of pairs (uses (*it).first to avoid -> operator)
template<typename Rng, typename U, typename V>
void check_equal(Rng&& rng, std::initializer_list<std::pair<U, V>> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ((*it).first, val.first);
        EXPECT_EQ((*it).second, val.second);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for checking a range against a std::vector (for chunk sub‑ranges)
template<typename Rng, typename T>
void check_equal(Rng&& rng, const std::vector<T>& expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Minimal forward iterator wrapper (replaces test_iterators.hpp)
/// ------------------------------------------------------------
template<typename It>
class ForwardIterator {
    It it_;
public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept   = std::forward_iterator_tag;
    using value_type        = typename std::iterator_traits<It>::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::iterator_traits<It>::pointer;
    using reference         = typename std::iterator_traits<It>::reference;

    ForwardIterator() = default;
    explicit ForwardIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer   operator->() const { return &*it_; }
    ForwardIterator& operator++() { ++it_; return *this; }
    ForwardIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    friend bool operator==(const ForwardIterator& a, const ForwardIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const ForwardIterator& a, const ForwardIterator& b) { return !(a == b); }
    It base() const { return it_; }
};

/// ------------------------------------------------------------
/// Test cases (mapping original tests to Google Test)
/// ------------------------------------------------------------

using P = std::pair<int, int>;

TEST(ChunkByTest, VectorWithPair) {
    using namespace ranges;

    std::vector<P> v = {
        {1,1}, {1,1}, {1,2}, {1,2}, {1,2}, {1,2},
        {2,2}, {2,2}, {2,3}, {2,3}, {2,3}, {2,3}
    };

    {
        auto rng = v | views::chunk_by([](P p0, P p1) { return p0.second == p1.second; });
        static_assert(forward_range<decltype(rng)>, "");
        static_assert(!bidirectional_range<decltype(rng)>, "");
        EXPECT_EQ(ranges::distance(rng), 3);
        check_equal(*rng.begin(), {P{1,1}, P{1,1}});
        auto it = rng.begin();
        ++it;
        check_equal(*it, {P{1,2}, P{1,2}, P{1,2}, P{1,2}, P{2,2}, P{2,2}});
        ++it;
        check_equal(*it, {P{2,3}, P{2,3}, P{2,3}, P{2,3}});
    }
    {
        auto rng = v | views::chunk_by([](P p0, P p1) { return p0.first == p1.first; });
        static_assert(forward_range<decltype(rng)>, "");
        static_assert(!bidirectional_range<decltype(rng)>, "");
        EXPECT_EQ(ranges::distance(rng), 2);
        check_equal(*rng.begin(), {P{1,1}, P{1,1}, P{1,2}, P{1,2}, P{1,2}, P{1,2}});
        auto it = rng.begin();
        ++it;
        check_equal(*it, {P{2,2}, P{2,2}, P{2,3}, P{2,3}, P{2,3}, P{2,3}});
    }
}

TEST(ChunkByTest, ForwardIteratorCounted) {
    using namespace ranges;

    std::vector<P> v = {
        {1,1}, {1,1}, {1,2}, {1,2}, {1,2}, {1,2},
        {2,2}, {2,2}, {2,3}, {2,3}, {2,3}, {2,3}
    };
    using Iter = ForwardIterator<decltype(v.begin())>;
    Iter b{v.begin()};
    auto rng0 = views::counted(b, v.size()) |
                views::chunk_by([](P p0, P p1) { return p0.second == p1.second; });
    static_assert(forward_range<decltype(rng0)>, "");
    static_assert(!bidirectional_range<decltype(rng0)>, "");
    EXPECT_EQ(ranges::distance(rng0), 3);
    check_equal(*rng0.begin(), {P{1,1}, P{1,1}});
    auto it0 = rng0.begin();
    ++it0;
    check_equal(*it0, {P{1,2}, P{1,2}, P{1,2}, P{1,2}, P{2,2}, P{2,2}});
    ++it0;
    check_equal(*it0, {P{2,3}, P{2,3}, P{2,3}, P{2,3}});

    auto rng1 = views::counted(b, v.size()) |
                views::chunk_by([](P p0, P p1) { return p0.first == p1.first; });
    static_assert(forward_range<decltype(rng1)>, "");
    static_assert(!bidirectional_range<decltype(rng1)>, "");
    EXPECT_EQ(ranges::distance(rng1), 2);
    check_equal(*rng1.begin(), {P{1,1}, P{1,1}, P{1,2}, P{1,2}, P{1,2}, P{1,2}});
    auto it1 = rng1.begin();
    ++it1;
    check_equal(*it1, {P{2,2}, P{2,2}, P{2,3}, P{2,3}, P{2,3}, P{2,3}});
}

TEST(ChunkByTest, RemoveIfThenChunk) {
    using namespace ranges;

    int a[] = {0,1,2,3,4,5};
    auto rng = a | views::remove_if([](int n) { return n % 2 == 0; }) |
               views::chunk_by([](int, int) { return true; });
    check_equal(*rng.begin(), {1,3,5});
}

TEST(ChunkByTest, VectorWithPredicate) {
    using namespace ranges;

    std::vector<int> v2{0,1,2,6,8,10,15,17,18,29};
    auto rng0 = views::chunk_by(v2, [](int i, int j) { return j - i < 3; });
    check_equal(*rng0.begin(), {0,1,2});
    auto it = rng0.begin();
    ++it;
    check_equal(*it, {6,8,10});
    ++it;
    check_equal(*it, {15,17,18});
    ++it;
    check_equal(*it, {29});
    EXPECT_EQ(ranges::distance(rng0), 4);
}

TEST(ChunkByTest, AlwaysFalsePredicate) {
    using namespace ranges;

    std::vector<int> v3{1,2,3,4,5};
    int count_invoc = 0;
    auto rng = views::chunk_by(v3, [&](int, int) {
        ++count_invoc;
        return false;
    });

    EXPECT_EQ(ranges::distance(rng), 5);
    EXPECT_EQ(count_invoc, 4);

    auto it = rng.begin();
    check_equal(*it, {1});
    ++it;
    check_equal(*it, {2});
    ++it;
    check_equal(*it, {3});
    ++it;
    check_equal(*it, {4});
    ++it;
    check_equal(*it, {5});
    // 7, not 8, because caching in begin()
    EXPECT_EQ(count_invoc, 7);
}

TEST(ChunkByTest, StrictlyIncreasing) {
    using namespace ranges;

    std::vector<int> v4 = {2,3,4,5,0,1,2,3,4,5,6,0,1,2,3,0};
    auto rng = v4 | views::chunk_by(std::less<>{});
    EXPECT_EQ(ranges::distance(rng), 4);
    check_equal(*rng.begin(), {2,3,4,5});
    auto it = rng.begin();
    ++it;
    check_equal(*it, {0,1,2,3,4,5,6});
    ++it;
    check_equal(*it, {0,1,2,3});
    ++it;
    check_equal(*it, {0});
}

TEST(ChunkByTest, CycleThenTake) {
    using namespace ranges;

    std::vector<int> v5 = {0,1,2};
    auto rng = views::cycle(v5) | views::take(6) | views::chunk_by(std::less<>{});
    EXPECT_EQ(ranges::distance(rng), 2);
    check_equal(*rng.begin(), v5);
    auto it = rng.begin();
    ++it;
    check_equal(*it, v5);
}

TEST(ChunkByTest, EmptyRange) {
    using namespace ranges;

    std::vector<int> e;
    auto rng = e | views::chunk_by(std::less<>{});
    EXPECT_EQ(ranges::distance(rng), 0);
}

TEST(ChunkByTest, SingleElement) {
    using namespace ranges;

    std::vector<int> single{2};
    auto rng = single | views::chunk_by([](int, int) -> bool { throw 0; });
    EXPECT_EQ(ranges::distance(rng), 1);
    check_equal(*rng.begin(), {2});
}

TEST(ChunkByTest, VariousChunks) {
    using namespace ranges;

    std::vector<int> v6 = {3,6,9,4,5,0,3,2};
    auto rng = v6 | views::chunk_by(std::less<>{});
    check_equal(*rng.begin(), {3,6,9});
    auto it = rng.begin();
    ++it;
    check_equal(*it, {4,5});
    ++it;
    check_equal(*it, {0,3});
    ++it;
    check_equal(*it, {2});
    EXPECT_EQ(ranges::distance(rng), 4);
}
