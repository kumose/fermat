// sort_gtest.cpp
// Google Test conversion of range-v3 sort algorithm test.
// Uses fermat::range and minimal iterator adapters.
// All comments in English.

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <random>
#include <vector>
#include <array>
#include <numeric>
#include <functional>

#include <fermat/core.h>
#include <fermat/algorithm/sort.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/for_each.h>
#include <fermat/view/iota.h>
#include <fermat/view/repeat_n.h>
#include <fermat/view/reverse.h>
#include <fermat/view/zip.h>
#include <fermat/view/transform.h>
#include <fermat/range/conversion.h>

// ------------------------------------------------------------
// Minimal test iterators (replaces ../test_iterators.hpp)
// ------------------------------------------------------------

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
    friend bool operator==(const RandomAccessIterator<It>& i, const Sentinel& s) { return i.base() == s.it_; }
    friend bool operator==(const Sentinel& s, const RandomAccessIterator<It>& i) { return i == s; }
    friend bool operator!=(const RandomAccessIterator<It>& i, const Sentinel& s) { return !(i == s); }
    friend bool operator!=(const Sentinel& s, const RandomAccessIterator<It>& i) { return !(i == s); }
    It base() const { return it_; }
};

// Helper to convert a Raw pointer iterator pair to a range
template <typename Iter, typename Sent>
auto MakeTestRange(Iter begin, Sent end) {
    return ranges::subrange<Iter, Sent>(begin, end);
}

// Placeholder for is_dangling (C++17 compatible template)
namespace ranges {
    template<typename T>
    bool is_dangling(T&&) { return false; }
}

// ------------------------------------------------------------
// Original test helpers (CHECK -> EXPECT)
// ------------------------------------------------------------

namespace {
    std::mt19937 gen;

    struct indirect_less {
        template<class P>
        bool operator()(const P& x, const P& y) { return *x < *y; }
    };

    // Helper to test sort on a given permutation (using raw pointers)
    template<class RI>
    void test_sort_helper(RI f, RI l) {
        using value_type = ranges::iter_value_t<RI>;
        if (f != l) {
            auto len = l - f;
            value_type* save = new value_type[len];
            do {
                std::copy(f, l, save);
                // Default sort
                auto res = ranges::sort(save, save + len);
                EXPECT_EQ(res, save + len);
                EXPECT_TRUE(std::is_sorted(save, save + len));
                // Copy back original permutation
                std::copy(f, l, save);
                // Sort with greater comparator
                res = ranges::sort(save, save + len, std::greater<int>{});
                EXPECT_EQ(res, save + len);
                EXPECT_TRUE(std::is_sorted(save, save + len, std::greater<int>{}));
                std::copy(f, l, save);
            } while (std::next_permutation(f, l));
            delete[] save;
        }
    }

    template<class RI>
    void test_sort_driver_driver(RI f, RI l, int start, RI real_last) {
        for (RI i = l; i > f + start;) {
            *--i = start;
            if (f == i) {
                test_sort_helper(f, real_last);
            }
            if (start > 0)
                test_sort_driver_driver(f, i, start - 1, real_last);
        }
    }

    template<class RI>
    void test_sort_driver(RI f, RI l, int start) {
        test_sort_driver_driver(f, l, start, l);
    }

    template<int sa>
    void test_sort_() {
        int ia[sa];
        for (int i = 0; i < sa; ++i) {
            test_sort_driver(ia, ia + sa, i);
        }
    }

    void test_larger_sorts(int N, int M) {
        // RANGES_ENSURE macro not used; we use assert-like check
        assert(N > 0 && M > 0);
        int* array = new int[N];
        int x = 0;
        for (int i = 0; i < N; ++i) {
            array[i] = x;
            if (++x == M) x = 0;
        }

        // sawtooth pattern
        EXPECT_EQ(ranges::sort(array, array + N), array + N);
        EXPECT_TRUE(std::is_sorted(array, array + N));
        // random pattern
        std::shuffle(array, array + N, gen);
        EXPECT_EQ(ranges::sort(array, array + N), array + N);
        EXPECT_TRUE(std::is_sorted(array, array + N));
        // already sorted
        EXPECT_EQ(ranges::sort(array, array + N), array + N);
        EXPECT_TRUE(std::is_sorted(array, array + N));
        // reverse sorted
        std::reverse(array, array + N);
        EXPECT_EQ(ranges::sort(array, array + N), array + N);
        EXPECT_TRUE(std::is_sorted(array, array + N));
        // swap ranges 2 pattern
        std::swap_ranges(array, array + N / 2, array + N / 2);
        EXPECT_EQ(ranges::sort(array, array + N), array + N);
        EXPECT_TRUE(std::is_sorted(array, array + N));
        // reverse swap ranges 2 pattern
        std::reverse(array, array + N);
        std::swap_ranges(array, array + N / 2, array + N / 2);
        EXPECT_EQ(ranges::sort(array, array + N), array + N);
        EXPECT_TRUE(std::is_sorted(array, array + N));
        delete[] array;
    }

    void test_larger_sorts(unsigned N) {
        test_larger_sorts(N, 1);
        test_larger_sorts(N, 2);
        test_larger_sorts(N, 3);
        test_larger_sorts(N, N/2 - 1);
        test_larger_sorts(N, N/2);
        test_larger_sorts(N, N/2 + 1);
        test_larger_sorts(N, N - 2);
        test_larger_sorts(N, N - 1);
        test_larger_sorts(N, N);
    }

    struct S {
        int i, j;
    };

    struct Int {
        using difference_type = int;
        int i_;
        Int(int i = 0) : i_(i) {}
        Int(Int&& that) : i_(that.i_) { that.i_ = 0; }
        Int(Int const&) = delete;
        Int& operator=(Int&& that) {
            i_ = that.i_;
            that.i_ = 0;
            return *this;
        }
        friend bool operator==(Int const& a, Int const& b) { return a.i_ == b.i_; }
        friend bool operator!=(Int const& a, Int const& b) { return !(a == b); }
        friend bool operator<(Int const& a, Int const& b) { return a.i_ < b.i_; }
        friend bool operator>(Int const& a, Int const& b) { return a.i_ > b.i_; }
        friend bool operator<=(Int const& a, Int const& b) { return a.i_ <= b.i_; }
        friend bool operator>=(Int const& a, Int const& b) { return a.i_ >= b.i_; }
    };
    static_assert(ranges::default_constructible<Int>);
    static_assert(ranges::movable<Int>);
    static_assert(ranges::totally_ordered<Int>);
} // unnamed namespace

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------

TEST(SortTest, NullRange) {
    int d = 0;
    ranges::sort(&d, &d); // should compile and do nothing
}

TEST(SortTest, SmallPermutations) {
    test_sort_<1>();
    test_sort_<2>();
    test_sort_<3>();
    test_sort_<4>();
    test_sort_<5>();
    test_sort_<6>();
    test_sort_<7>();
    test_sort_<8>();
}

TEST(SortTest, LargerSorts) {
    test_larger_sorts(15);
    test_larger_sorts(16);
    test_larger_sorts(17);
    test_larger_sorts(256);
    test_larger_sorts(257);
    test_larger_sorts(499);
    test_larger_sorts(500);
    test_larger_sorts(997);
    test_larger_sorts(1000);
    test_larger_sorts(1009);
}

TEST(SortTest, MoveOnlyTypes) {
    std::vector<std::unique_ptr<int>> v(1000);
    for (size_t i = 0; i < v.size(); ++i)
        v[i].reset(new int(static_cast<int>(v.size() - i - 1)));
    ranges::sort(v, indirect_less());
    for (size_t i = 0; i < v.size(); ++i)
        EXPECT_EQ(*v[i], i);
}

TEST(SortTest, Projections) {
    std::vector<S> v(1000, S{});
    for (size_t i = 0; i < v.size(); ++i) {
        v[i].i = static_cast<int>(v.size() - i - 1);
        v[i].j = static_cast<int>(i);
    }
    ranges::sort(v, std::less<int>{}, &S::i);
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i].i, i);
        EXPECT_EQ(static_cast<size_t>(v[i].j), v.size() - i - 1);
    }
}

TEST(SortTest, RvalueRange) {
    std::vector<S> v(1000, S{});
    for (size_t i = 0; i < v.size(); ++i) {
        v[i].i = static_cast<int>(v.size() - i - 1);
        v[i].j = static_cast<int>(i);
    }
    EXPECT_EQ(ranges::sort(ranges::views::all(v), std::less<int>{}, &S::i), v.end());
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i].i, i);
        EXPECT_EQ(static_cast<size_t>(v[i].j), v.size() - i - 1);
    }
}

TEST(SortTest, RvalueMovedRange) {
    std::vector<S> v(1000, S{});
    for (size_t i = 0; i < v.size(); ++i) {
        v[i].i = static_cast<int>(v.size() - i - 1);
        v[i].j = static_cast<int>(i);
    }
    auto res = ranges::sort(std::move(v), std::less<int>{}, &S::i);
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i].i, i);
        EXPECT_EQ(static_cast<size_t>(v[i].j), v.size() - i - 1);
    }
    (void)res;
}

TEST(SortTest, ZipViewSort) {
    using namespace ranges;
    // Build a vector v0: 5 repeated 5 times, then 4 repeated 4 times, ... 1 repeated 1 time
    auto v0 = views::for_each(views::ints(1, 6) | views::reverse, [](int i) {
                 return yield_from(views::repeat_n(i, i));
             }) | to<std::vector>();
    std::vector<int> expected_v0 = {5,5,5,5,5,4,4,4,4,3,3,3,2,2,1};
    EXPECT_EQ(v0, expected_v0);

    // v1: sequence of Ints: 1,2,2,3,3,3,4,4,4,4,5,5,5,5,5
    auto v1 = to<std::vector<Int>>({1,2,2,3,3,3,4,4,4,4,5,5,5,5,5});
    auto rng = views::zip(v0, v1);
    // Sort by the pair (which compares first, then second)
    sort(rng);
    std::vector<int> expected_v0_sorted = {1,2,2,3,3,3,4,4,4,4,5,5,5,5,5};
    std::vector<int> expected_v1 = {5,5,5,4,5,5,3,4,4,4,1,2,2,3,3};
    std::vector<int> v1_ints;
    for (const auto& i : v1) v1_ints.push_back(i.i_);
    EXPECT_EQ(v0, expected_v0_sorted);
    EXPECT_EQ(v1_ints, expected_v1);

    // Also test sort with custom predicate and projection
    auto pred = [](const auto& r1, const auto& r2) { return r1 < r2; };
    // proj must return a reference to avoid copying move-only Int
    auto proj = [](const auto& r) -> const auto& { return r; };
    sort(rng, pred, proj);
    // Result should be the same; just check that it compiles and runs
    EXPECT_EQ(v0, expected_v0_sorted);
}
