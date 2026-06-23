/// group_by_gtest.cpp
/// Google Test conversion of range-v3 group_by view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <list>
#include <utility>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/iterator/operations.h>
#include <fermat/view/counted.h>
#include <fermat/view/cycle.h>
#include <fermat/view/group_by.h>
#include <fermat/view/remove_if.h>
#include <fermat/view/take.h>

// Disable deprecation warnings for views::group_by (we are testing the old API)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/// ------------------------------------------------------------
/// Minimal forward iterator (as in test_iterators.hpp)
/// ------------------------------------------------------------
template<typename It>
class ForwardIterator {
    It it_;

public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<It>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::iterator_traits<It>::pointer;
    using reference = typename std::iterator_traits<It>::reference;

    ForwardIterator() = default;

    explicit ForwardIterator(It it) : it_(it) {
    }

    reference operator*() const { return *it_; }
    pointer operator->() const { return &*it_; }

    ForwardIterator &operator++() {
        ++it_;
        return *this;
    }

    ForwardIterator operator++(int) {
        auto tmp = *this;
        ++it_;
        return tmp;
    }

    friend bool operator==(const ForwardIterator &a, const ForwardIterator &b) { return a.it_ == b.it_; }
    friend bool operator!=(const ForwardIterator &a, const ForwardIterator &b) { return !(a == b); }
    It base() const { return it_; }
};

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list (single elements)
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for ranges of pairs (use (*it).first to avoid operator-> issues)
template<typename Rng, typename U, typename V>
void check_equal(Rng &&rng, std::initializer_list<std::pair<U, V> > expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ((*it).first, val.first);
        EXPECT_EQ((*it).second, val.second);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for checking a range against a std::vector (for chunk sub‑ranges)
template<typename Rng, typename T>
void check_equal(Rng &&rng, const std::vector<T> &expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------
using P = std::pair<int, int>;

TEST(GroupByTest, VectorWithPair) {
    using namespace fermat::ranges;

    std::vector<P> v = {
        {1, 1}, {1, 1}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
        {2, 2}, {2, 2}, {2, 3}, {2, 3}, {2, 3}, {2, 3}
    };

    {
        auto rng = v | views::group_by([](P p0, P p1) { return p0.second == p1.second; });
        EXPECT_EQ(fermat::ranges::distance(rng), 3);
        check_equal(*rng.begin(), {P{1, 1}, P{1, 1}});
        auto it = rng.begin();
        ++it;
        check_equal(*it, {P{1, 2}, P{1, 2}, P{1, 2}, P{1, 2}, P{2, 2}, P{2, 2}});
        ++it;
        check_equal(*it, {P{2, 3}, P{2, 3}, P{2, 3}, P{2, 3}});
    }
    {
        auto rng = v | views::group_by([](P p0, P p1) { return p0.first == p1.first; });
        EXPECT_EQ(fermat::ranges::distance(rng), 2);
        check_equal(*rng.begin(), {P{1, 1}, P{1, 1}, P{1, 2}, P{1, 2}, P{1, 2}, P{1, 2}});
        auto it = rng.begin();
        ++it;
        check_equal(*it, {P{2, 2}, P{2, 2}, P{2, 3}, P{2, 3}, P{2, 3}, P{2, 3}});
    }
}

TEST(GroupByTest, ForwardIteratorCounted) {
    using namespace fermat::ranges;

    std::vector<P> v = {
        {1, 1}, {1, 1}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
        {2, 2}, {2, 2}, {2, 3}, {2, 3}, {2, 3}, {2, 3}
    };
    using Iter = ForwardIterator<decltype(v.begin())>;
    Iter b{v.begin()};
    auto rng0 = views::counted(b, v.size())
                | views::group_by([](P p0, P p1) { return p0.second == p1.second; });
    EXPECT_EQ(fermat::ranges::distance(rng0), 3);
    check_equal(*rng0.begin(), {P{1, 1}, P{1, 1}});
    auto it0 = rng0.begin();
    ++it0;
    check_equal(*it0, {P{1, 2}, P{1, 2}, P{1, 2}, P{1, 2}, P{2, 2}, P{2, 2}});
    ++it0;
    check_equal(*it0, {P{2, 3}, P{2, 3}, P{2, 3}, P{2, 3}});

    auto rng1 = views::counted(b, v.size())
                | views::group_by([](P p0, P p1) { return p0.first == p1.first; });
    EXPECT_EQ(fermat::ranges::distance(rng1), 2);
    check_equal(*rng1.begin(), {P{1, 1}, P{1, 1}, P{1, 2}, P{1, 2}, P{1, 2}, P{1, 2}});
    auto it1 = rng1.begin();
    ++it1;
    check_equal(*it1, {P{2, 2}, P{2, 2}, P{2, 3}, P{2, 3}, P{2, 3}, P{2, 3}});
}

TEST(GroupByTest, RemoveIfThenGroup) {
    using namespace fermat::ranges;

    int a[] = {0, 1, 2, 3, 4, 5};
    auto rng = a | views::remove_if([](int n) { return n % 2 == 0; })
               | views::group_by([](int, int) { return true; });
    check_equal(*rng.begin(), {1, 3, 5});
}

TEST(GroupByTest, VectorWithPredicate) {
    using namespace fermat::ranges;

    std::vector<int> v2{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng0 = views::group_by(v2, [](int i, int j) { return j - i < 3; });
    check_equal(*rng0.begin(), {0, 1, 2});
    auto it = rng0.begin();
    ++it;
    check_equal(*it, {3, 4, 5});
    ++it;
    check_equal(*it, {6, 7, 8});
    ++it;
    check_equal(*it, {9});
    EXPECT_EQ(fermat::ranges::distance(rng0), 4);
}

TEST(GroupByTest, AlwaysFalsePredicate) {
    using namespace fermat::ranges;

    std::vector<int> v3{1, 2, 3, 4, 5};
    int count_invoc = 0;
    auto rng = views::group_by(v3, [&](int, int) {
        ++count_invoc;
        return false;
    });

    EXPECT_EQ(fermat::ranges::distance(rng), 5);
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

TEST(GroupByTest, StrictlyIncreasing) {
    using namespace fermat::ranges;

    std::vector<int> v4 = {2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 0};
    auto rng = v4 | views::group_by(std::less<>{});
    EXPECT_EQ(fermat::ranges::distance(rng), 4);
    check_equal(*rng.begin(), {2, 3, 4, 5});
    auto it = rng.begin();
    ++it;
    check_equal(*it, {0, 1, 2, 3, 4, 5, 6});
    ++it;
    check_equal(*it, {0, 1, 2, 3});
    ++it;
    check_equal(*it, {0});
}

TEST(GroupByTest, CycleThenTake) {
    using namespace fermat::ranges;

    std::vector<int> v5 = {0, 1, 2};
    auto rng = views::cycle(v5) | views::take(6) | views::group_by(std::less<>{});
    EXPECT_EQ(fermat::ranges::distance(rng), 2);
    check_equal(*rng.begin(), v5);
    auto it = rng.begin();
    ++it;
    check_equal(*it, v5);
}

TEST(GroupByTest, EmptyRange) {
    using namespace fermat::ranges;

    std::vector<int> e;
    auto rng = e | views::group_by(std::less<>{});
    EXPECT_EQ(fermat::ranges::distance(rng), 0);
}

TEST(GroupByTest, SingleElement) {
    using namespace fermat::ranges;

    std::vector<int> single{2};
    auto rng = single | views::group_by([](int, int) -> bool { throw 0; });
    EXPECT_EQ(fermat::ranges::distance(rng), 1);
    check_equal(*rng.begin(), {2});
}

#pragma GCC diagnostic pop
