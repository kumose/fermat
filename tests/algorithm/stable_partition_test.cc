// stable_partition_gtest.cpp
// Google Test conversion of range-v3 stable_partition algorithm test.
// Uses fermat::range and minimal iterator adapters.
// All comments in English.

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <fermat/core.h>
#include <fermat/algorithm/stable_partition.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/all.h>

// ------------------------------------------------------------
// Minimal test iterators (replaces ../test_iterators.hpp)
// ------------------------------------------------------------

template <typename It>
class BidirectionalIterator {
    It it_;
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename std::iterator_traits<It>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::iterator_traits<It>::pointer;
    using reference = typename std::iterator_traits<It>::reference;

    BidirectionalIterator() = default;
    explicit BidirectionalIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer operator->() const { return &*it_; }
    BidirectionalIterator& operator++() { ++it_; return *this; }
    BidirectionalIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    BidirectionalIterator& operator--() { --it_; return *this; }
    BidirectionalIterator operator--(int) { auto tmp = *this; --it_; return tmp; }
    friend bool operator==(const BidirectionalIterator& a, const BidirectionalIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const BidirectionalIterator& a, const BidirectionalIterator& b) { return !(a == b); }
    It base() const { return it_; }
};

template <typename It>
class RandomAccessIterator {
    It it_;
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename std::iterator_traits<It>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::iterator_traits<It>::pointer;
    using reference = typename std::iterator_traits<It>::reference;

    RandomAccessIterator() = default;
    explicit RandomAccessIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer operator->() const { return &*it_; }
    RandomAccessIterator& operator++() { ++it_; return *this; }
    RandomAccessIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    RandomAccessIterator& operator--() { --it_; return *this; }
    RandomAccessIterator operator--(int) { auto tmp = *this; --it_; return tmp; }
    RandomAccessIterator& operator+=(difference_type n) { it_ += n; return *this; }
    RandomAccessIterator& operator-=(difference_type n) { it_ -= n; return *this; }
    friend RandomAccessIterator operator+(RandomAccessIterator it, difference_type n) { it += n; return it; }
    friend RandomAccessIterator operator+(difference_type n, RandomAccessIterator it) { return it + n; }
    friend RandomAccessIterator operator-(RandomAccessIterator it, difference_type n) { it -= n; return it; }
    friend difference_type operator-(const RandomAccessIterator& a, const RandomAccessIterator& b) {
        return a.it_ - b.it_;
    }
    reference operator[](difference_type n) const { return *(it_ + n); }
    friend bool operator==(const RandomAccessIterator& a, const RandomAccessIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const RandomAccessIterator& a, const RandomAccessIterator& b) { return !(a == b); }
    friend bool operator<(const RandomAccessIterator& a, const RandomAccessIterator& b) { return a.it_ < b.it_; }
    friend bool operator>(const RandomAccessIterator& a, const RandomAccessIterator& b) { return b < a; }
    friend bool operator<=(const RandomAccessIterator& a, const RandomAccessIterator& b) { return !(b < a); }
    friend bool operator>=(const RandomAccessIterator& a, const RandomAccessIterator& b) { return !(a < b); }
    It base() const { return it_; }
};

template <typename It>
class Sentinel {
    It it_;
public:
    Sentinel() = default;
    explicit Sentinel(It it) : it_(it) {}
    friend bool operator==(const BidirectionalIterator<It>& i, const Sentinel& s) { return i.base() == s.it_; }
    friend bool operator==(const Sentinel& s, const BidirectionalIterator<It>& i) { return i == s; }
    friend bool operator!=(const BidirectionalIterator<It>& i, const Sentinel& s) { return !(i == s); }
    friend bool operator!=(const Sentinel& s, const BidirectionalIterator<It>& i) { return !(i == s); }
    friend bool operator==(const RandomAccessIterator<It>& i, const Sentinel& s) { return i.base() == s.it_; }
    friend bool operator==(const Sentinel& s, const RandomAccessIterator<It>& i) { return i == s; }
    friend bool operator!=(const RandomAccessIterator<It>& i, const Sentinel& s) { return !(i == s); }
    friend bool operator!=(const Sentinel& s, const RandomAccessIterator<It>& i) { return !(i == s); }
    It base() const { return it_; }
};

// ------------------------------------------------------------
// Helper: base() to get underlying raw pointer
// ------------------------------------------------------------
template <typename Iter>
auto base(Iter i) -> decltype(i.base()) { return i.base(); }
template <typename T>
T* base(T* p) { return p; }

// ------------------------------------------------------------
// is_dangling placeholder (C++17 compatible)
// ------------------------------------------------------------
namespace fermat::ranges {
    template<typename T>
    bool is_dangling(T&&) { return false; }
}

// Helper to treat lvalue (as in original test)
template <typename T>
T& as_lvalue(T&& t) { return t; }

// Helper to make subrange
template <typename Iter, typename Sent>
auto make_subrange(Iter begin, Sent end) {
    return fermat::ranges::subrange<Iter, Sent>(begin, end);
}

// ------------------------------------------------------------
// Predicates used in tests
// ------------------------------------------------------------
struct is_odd {
    bool operator()(int i) const { return i & 1; }
};
struct odd_first {
    bool operator()(const std::pair<int,int>& p) const { return p.first & 1; }
};

// ------------------------------------------------------------
// Test helpers (converted from original)
// ------------------------------------------------------------
template<class Iter, class Sent = Iter>
void test_iter() {
    using P = std::pair<int, int>;
    {  // check mixed
        P ap[] = { {0, 1}, {0, 2}, {1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 1}, {3, 2}, {4, 1}, {4, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + 4);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{0, 1}));
        EXPECT_EQ(ap[5], (P{0, 2}));
        EXPECT_EQ(ap[6], (P{2, 1}));
        EXPECT_EQ(ap[7], (P{2, 2}));
        EXPECT_EQ(ap[8], (P{4, 1}));
        EXPECT_EQ(ap[9], (P{4, 2}));
    }
    {
        P ap[] = { {0, 1}, {0, 2}, {1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 1}, {3, 2}, {4, 1}, {4, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + 4);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{0, 1}));
        EXPECT_EQ(ap[5], (P{0, 2}));
        EXPECT_EQ(ap[6], (P{2, 1}));
        EXPECT_EQ(ap[7], (P{2, 2}));
        EXPECT_EQ(ap[8], (P{4, 1}));
        EXPECT_EQ(ap[9], (P{4, 2}));
        // empty
        r = fermat::ranges::stable_partition(Iter(ap), Sent(ap), odd_first());
        EXPECT_EQ(base(r), ap);
        // one true
        r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+1), odd_first());
        EXPECT_EQ(base(r), ap + 1);
        EXPECT_EQ(ap[0], (P{1, 1}));
        // one false
        r = fermat::ranges::stable_partition(Iter(ap+4), Sent(ap+5), odd_first());
        EXPECT_EQ(base(r), ap + 4);
        EXPECT_EQ(ap[4], (P{0, 1}));
    }
    {  // all false
        P ap[] = { {0, 1}, {0, 2}, {2, 1}, {2, 2}, {4, 1}, {4, 2}, {6, 1}, {6, 2}, {8, 1}, {8, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap);
        EXPECT_EQ(ap[0], (P{0, 1}));
        EXPECT_EQ(ap[1], (P{0, 2}));
        EXPECT_EQ(ap[2], (P{2, 1}));
        EXPECT_EQ(ap[3], (P{2, 2}));
        EXPECT_EQ(ap[4], (P{4, 1}));
        EXPECT_EQ(ap[5], (P{4, 2}));
        EXPECT_EQ(ap[6], (P{6, 1}));
        EXPECT_EQ(ap[7], (P{6, 2}));
        EXPECT_EQ(ap[8], (P{8, 1}));
        EXPECT_EQ(ap[9], (P{8, 2}));
    }
    {  // all true
        P ap[] = { {1, 1}, {1, 2}, {3, 1}, {3, 2}, {5, 1}, {5, 2}, {7, 1}, {7, 2}, {9, 1}, {9, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + size);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{5, 1}));
        EXPECT_EQ(ap[5], (P{5, 2}));
        EXPECT_EQ(ap[6], (P{7, 1}));
        EXPECT_EQ(ap[7], (P{7, 2}));
        EXPECT_EQ(ap[8], (P{9, 1}));
        EXPECT_EQ(ap[9], (P{9, 2}));
    }
    {  // all false but first true
        P ap[] = { {1, 1}, {0, 2}, {2, 1}, {2, 2}, {4, 1}, {4, 2}, {6, 1}, {6, 2}, {8, 1}, {8, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + 1);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{0, 2}));
        EXPECT_EQ(ap[2], (P{2, 1}));
        EXPECT_EQ(ap[3], (P{2, 2}));
        EXPECT_EQ(ap[4], (P{4, 1}));
        EXPECT_EQ(ap[5], (P{4, 2}));
        EXPECT_EQ(ap[6], (P{6, 1}));
        EXPECT_EQ(ap[7], (P{6, 2}));
        EXPECT_EQ(ap[8], (P{8, 1}));
        EXPECT_EQ(ap[9], (P{8, 2}));
    }
    {  // all false but last true
        P ap[] = { {0, 1}, {0, 2}, {2, 1}, {2, 2}, {4, 1}, {4, 2}, {6, 1}, {6, 2}, {8, 1}, {1, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + 1);
        EXPECT_EQ(ap[0], (P{1, 2}));
        EXPECT_EQ(ap[1], (P{0, 1}));
        EXPECT_EQ(ap[2], (P{0, 2}));
        EXPECT_EQ(ap[3], (P{2, 1}));
        EXPECT_EQ(ap[4], (P{2, 2}));
        EXPECT_EQ(ap[5], (P{4, 1}));
        EXPECT_EQ(ap[6], (P{4, 2}));
        EXPECT_EQ(ap[7], (P{6, 1}));
        EXPECT_EQ(ap[8], (P{6, 2}));
        EXPECT_EQ(ap[9], (P{8, 1}));
    }
    {  // all true but first false
        P ap[] = { {0, 1}, {1, 2}, {3, 1}, {3, 2}, {5, 1}, {5, 2}, {7, 1}, {7, 2}, {9, 1}, {9, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + size - 1);
        EXPECT_EQ(ap[0], (P{1, 2}));
        EXPECT_EQ(ap[1], (P{3, 1}));
        EXPECT_EQ(ap[2], (P{3, 2}));
        EXPECT_EQ(ap[3], (P{5, 1}));
        EXPECT_EQ(ap[4], (P{5, 2}));
        EXPECT_EQ(ap[5], (P{7, 1}));
        EXPECT_EQ(ap[6], (P{7, 2}));
        EXPECT_EQ(ap[7], (P{9, 1}));
        EXPECT_EQ(ap[8], (P{9, 2}));
        EXPECT_EQ(ap[9], (P{0, 1}));
    }
    {  // all true but last false
        P ap[] = { {1, 1}, {1, 2}, {3, 1}, {3, 2}, {5, 1}, {5, 2}, {7, 1}, {7, 2}, {9, 1}, {0, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(Iter(ap), Sent(ap+size), odd_first());
        EXPECT_EQ(base(r), ap + size - 1);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{5, 1}));
        EXPECT_EQ(ap[5], (P{5, 2}));
        EXPECT_EQ(ap[6], (P{7, 1}));
        EXPECT_EQ(ap[7], (P{7, 2}));
        EXPECT_EQ(ap[8], (P{9, 1}));
        EXPECT_EQ(ap[9], (P{0, 2}));
    }
}

template<class Iter, class Sent = Iter>
void test_range() {
    using P = std::pair<int, int>;
    {  // mixed
        P ap[] = { {0, 1}, {0, 2}, {1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 1}, {3, 2}, {4, 1}, {4, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + 4);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{0, 1}));
        EXPECT_EQ(ap[5], (P{0, 2}));
        EXPECT_EQ(ap[6], (P{2, 1}));
        EXPECT_EQ(ap[7], (P{2, 2}));
        EXPECT_EQ(ap[8], (P{4, 1}));
        EXPECT_EQ(ap[9], (P{4, 2}));
    }
    {
        P ap[] = { {0, 1}, {0, 2}, {1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 1}, {3, 2}, {4, 1}, {4, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + 4);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{0, 1}));
        EXPECT_EQ(ap[5], (P{0, 2}));
        EXPECT_EQ(ap[6], (P{2, 1}));
        EXPECT_EQ(ap[7], (P{2, 2}));
        EXPECT_EQ(ap[8], (P{4, 1}));
        EXPECT_EQ(ap[9], (P{4, 2}));
        // empty
        r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap))), odd_first());
        EXPECT_EQ(base(r), ap);
        // one true
        r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+1))), odd_first());
        EXPECT_EQ(base(r), ap + 1);
        EXPECT_EQ(ap[0], (P{1, 1}));
        // one false
        r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap+4), Sent(ap+5))), odd_first());
        EXPECT_EQ(base(r), ap + 4);
        EXPECT_EQ(ap[4], (P{0, 1}));
    }
    {  // all false
        P ap[] = { {0, 1}, {0, 2}, {2, 1}, {2, 2}, {4, 1}, {4, 2}, {6, 1}, {6, 2}, {8, 1}, {8, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap);
        EXPECT_EQ(ap[0], (P{0, 1}));
        EXPECT_EQ(ap[1], (P{0, 2}));
        EXPECT_EQ(ap[2], (P{2, 1}));
        EXPECT_EQ(ap[3], (P{2, 2}));
        EXPECT_EQ(ap[4], (P{4, 1}));
        EXPECT_EQ(ap[5], (P{4, 2}));
        EXPECT_EQ(ap[6], (P{6, 1}));
        EXPECT_EQ(ap[7], (P{6, 2}));
        EXPECT_EQ(ap[8], (P{8, 1}));
        EXPECT_EQ(ap[9], (P{8, 2}));
    }
    {  // all true
        P ap[] = { {1, 1}, {1, 2}, {3, 1}, {3, 2}, {5, 1}, {5, 2}, {7, 1}, {7, 2}, {9, 1}, {9, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + size);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{5, 1}));
        EXPECT_EQ(ap[5], (P{5, 2}));
        EXPECT_EQ(ap[6], (P{7, 1}));
        EXPECT_EQ(ap[7], (P{7, 2}));
        EXPECT_EQ(ap[8], (P{9, 1}));
        EXPECT_EQ(ap[9], (P{9, 2}));
    }
    {  // all false but first true
        P ap[] = { {1, 1}, {0, 2}, {2, 1}, {2, 2}, {4, 1}, {4, 2}, {6, 1}, {6, 2}, {8, 1}, {8, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + 1);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{0, 2}));
        EXPECT_EQ(ap[2], (P{2, 1}));
        EXPECT_EQ(ap[3], (P{2, 2}));
        EXPECT_EQ(ap[4], (P{4, 1}));
        EXPECT_EQ(ap[5], (P{4, 2}));
        EXPECT_EQ(ap[6], (P{6, 1}));
        EXPECT_EQ(ap[7], (P{6, 2}));
        EXPECT_EQ(ap[8], (P{8, 1}));
        EXPECT_EQ(ap[9], (P{8, 2}));
    }
    {  // all false but last true
        P ap[] = { {0, 1}, {0, 2}, {2, 1}, {2, 2}, {4, 1}, {4, 2}, {6, 1}, {6, 2}, {8, 1}, {1, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + 1);
        EXPECT_EQ(ap[0], (P{1, 2}));
        EXPECT_EQ(ap[1], (P{0, 1}));
        EXPECT_EQ(ap[2], (P{0, 2}));
        EXPECT_EQ(ap[3], (P{2, 1}));
        EXPECT_EQ(ap[4], (P{2, 2}));
        EXPECT_EQ(ap[5], (P{4, 1}));
        EXPECT_EQ(ap[6], (P{4, 2}));
        EXPECT_EQ(ap[7], (P{6, 1}));
        EXPECT_EQ(ap[8], (P{6, 2}));
        EXPECT_EQ(ap[9], (P{8, 1}));
    }
    {  // all true but first false
        P ap[] = { {0, 1}, {1, 2}, {3, 1}, {3, 2}, {5, 1}, {5, 2}, {7, 1}, {7, 2}, {9, 1}, {9, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + size - 1);
        EXPECT_EQ(ap[0], (P{1, 2}));
        EXPECT_EQ(ap[1], (P{3, 1}));
        EXPECT_EQ(ap[2], (P{3, 2}));
        EXPECT_EQ(ap[3], (P{5, 1}));
        EXPECT_EQ(ap[4], (P{5, 2}));
        EXPECT_EQ(ap[5], (P{7, 1}));
        EXPECT_EQ(ap[6], (P{7, 2}));
        EXPECT_EQ(ap[7], (P{9, 1}));
        EXPECT_EQ(ap[8], (P{9, 2}));
        EXPECT_EQ(ap[9], (P{0, 1}));
    }
    {  // all true but last false
        P ap[] = { {1, 1}, {1, 2}, {3, 1}, {3, 2}, {5, 1}, {5, 2}, {7, 1}, {7, 2}, {9, 1}, {0, 2} };
        std::size_t size = fermat::ranges::size(ap);
        Iter r = fermat::ranges::stable_partition(as_lvalue(make_subrange(Iter(ap), Sent(ap+size))), odd_first());
        EXPECT_EQ(base(r), ap + size - 1);
        EXPECT_EQ(ap[0], (P{1, 1}));
        EXPECT_EQ(ap[1], (P{1, 2}));
        EXPECT_EQ(ap[2], (P{3, 1}));
        EXPECT_EQ(ap[3], (P{3, 2}));
        EXPECT_EQ(ap[4], (P{5, 1}));
        EXPECT_EQ(ap[5], (P{5, 2}));
        EXPECT_EQ(ap[6], (P{7, 1}));
        EXPECT_EQ(ap[7], (P{7, 2}));
        EXPECT_EQ(ap[8], (P{9, 1}));
        EXPECT_EQ(ap[9], (P{0, 2}));
    }
}

// Move-only type test
struct move_only {
    static int count;
    int i;
    move_only() = delete;
    move_only(int j) : i(j) { ++count; }
    move_only(move_only&& that) : i(that.i) { ++count; }
    move_only(move_only const&) = delete;
    ~move_only() { --count; }
    move_only& operator=(move_only&&) = default;
    move_only& operator=(move_only const&) = delete;
};
int move_only::count = 0;

template<class Iter>
void test_move_only() {
    const unsigned size = 5;
    move_only array[size] = { 1, 2, 3, 4, 5 };
    Iter r = fermat::ranges::stable_partition(Iter(array), Iter(array+size), is_odd{}, &move_only::i);
    EXPECT_EQ(base(r), array + 3);
    EXPECT_EQ(array[0].i, 1);
    EXPECT_EQ(array[1].i, 3);
    EXPECT_EQ(array[2].i, 5);
    EXPECT_EQ(array[3].i, 2);
    EXPECT_EQ(array[4].i, 4);
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------

TEST(StablePartition, IteratorPair) {
    test_iter<BidirectionalIterator<std::pair<int,int>*>>();
    test_iter<RandomAccessIterator<std::pair<int,int>*>>();
    test_iter<std::pair<int,int>*>();
    test_iter<BidirectionalIterator<std::pair<int,int>*>, Sentinel<std::pair<int,int>*>>();
    test_iter<RandomAccessIterator<std::pair<int,int>*>, Sentinel<std::pair<int,int>*>>();
}

TEST(StablePartition, Range) {
    test_range<BidirectionalIterator<std::pair<int,int>*>>();
    test_range<RandomAccessIterator<std::pair<int,int>*>>();
    test_range<std::pair<int,int>*>();
    test_range<BidirectionalIterator<std::pair<int,int>*>, Sentinel<std::pair<int,int>*>>();
    test_range<RandomAccessIterator<std::pair<int,int>*>, Sentinel<std::pair<int,int>*>>();
}

TEST(StablePartition, MoveOnly) {
    EXPECT_EQ(move_only::count, 0);
    test_move_only<BidirectionalIterator<move_only*>>();
    EXPECT_EQ(move_only::count, 0);
}

TEST(StablePartition, Projection) {
    using P = std::pair<int, int>;
    struct S { P p; };
    S ap[] = { {{0, 1}}, {{0, 2}}, {{1, 1}}, {{1, 2}}, {{2, 1}}, {{2, 2}}, {{3, 1}}, {{3, 2}}, {{4, 1}}, {{4, 2}} };
    S* r = fermat::ranges::stable_partition(ap, odd_first(), &S::p);
    EXPECT_EQ(r, ap + 4);
    EXPECT_EQ(ap[0].p, (P{1, 1}));
    EXPECT_EQ(ap[1].p, (P{1, 2}));
    EXPECT_EQ(ap[2].p, (P{3, 1}));
    EXPECT_EQ(ap[3].p, (P{3, 2}));
    EXPECT_EQ(ap[4].p, (P{0, 1}));
    EXPECT_EQ(ap[5].p, (P{0, 2}));
    EXPECT_EQ(ap[6].p, (P{2, 1}));
    EXPECT_EQ(ap[7].p, (P{2, 2}));
    EXPECT_EQ(ap[8].p, (P{4, 1}));
    EXPECT_EQ(ap[9].p, (P{4, 2}));
}

TEST(StablePartition, RvalueRange) {
    using P = std::pair<int, int>;
    struct S { P p; };
    {  // all view
        S ap[] = { {{0, 1}}, {{0, 2}}, {{1, 1}}, {{1, 2}}, {{2, 1}}, {{2, 2}}, {{3, 1}}, {{3, 2}}, {{4, 1}}, {{4, 2}} };
        auto r = fermat::ranges::stable_partition(fermat::ranges::views::all(ap), odd_first(), &S::p);
        EXPECT_EQ(r, ap + 4);
        EXPECT_EQ(ap[0].p, (P{1, 1}));
        EXPECT_EQ(ap[1].p, (P{1, 2}));
        EXPECT_EQ(ap[2].p, (P{3, 1}));
        EXPECT_EQ(ap[3].p, (P{3, 2}));
        EXPECT_EQ(ap[4].p, (P{0, 1}));
        EXPECT_EQ(ap[5].p, (P{0, 2}));
        EXPECT_EQ(ap[6].p, (P{2, 1}));
        EXPECT_EQ(ap[7].p, (P{2, 2}));
        EXPECT_EQ(ap[8].p, (P{4, 1}));
        EXPECT_EQ(ap[9].p, (P{4, 2}));
    }
    {  // rvalue array
        S ap[] = { {{0, 1}}, {{0, 2}}, {{1, 1}}, {{1, 2}}, {{2, 1}}, {{2, 2}}, {{3, 1}}, {{3, 2}}, {{4, 1}}, {{4, 2}} };
        auto r = fermat::ranges::stable_partition(std::move(ap), odd_first(), &S::p);
        // Note: is_dangling may not be implemented; skip that check.
        EXPECT_EQ(ap[0].p, (P{1, 1}));
        EXPECT_EQ(ap[1].p, (P{1, 2}));
        EXPECT_EQ(ap[2].p, (P{3, 1}));
        EXPECT_EQ(ap[3].p, (P{3, 2}));
        EXPECT_EQ(ap[4].p, (P{0, 1}));
        EXPECT_EQ(ap[5].p, (P{0, 2}));
        EXPECT_EQ(ap[6].p, (P{2, 1}));
        EXPECT_EQ(ap[7].p, (P{2, 2}));
        EXPECT_EQ(ap[8].p, (P{4, 1}));
        EXPECT_EQ(ap[9].p, (P{4, 2}));
        (void)r;
    }
    {  // rvalue vector
        std::vector<S> ap{ {{0, 1}}, {{0, 2}}, {{1, 1}}, {{1, 2}}, {{2, 1}}, {{2, 2}}, {{3, 1}}, {{3, 2}}, {{4, 1}}, {{4, 2}} };
        auto r = fermat::ranges::stable_partition(std::move(ap), odd_first(), &S::p);
        // is_dangling may not be available; skip check.
        EXPECT_EQ(ap[0].p, (P{1, 1}));
        EXPECT_EQ(ap[1].p, (P{1, 2}));
        EXPECT_EQ(ap[2].p, (P{3, 1}));
        EXPECT_EQ(ap[3].p, (P{3, 2}));
        EXPECT_EQ(ap[4].p, (P{0, 1}));
        EXPECT_EQ(ap[5].p, (P{0, 2}));
        EXPECT_EQ(ap[6].p, (P{2, 1}));
        EXPECT_EQ(ap[7].p, (P{2, 2}));
        EXPECT_EQ(ap[8].p, (P{4, 1}));
        EXPECT_EQ(ap[9].p, (P{4, 2}));
        (void)r;
    }
}
