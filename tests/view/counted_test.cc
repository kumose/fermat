/// counted_gtest.cpp
/// Google Test conversion of range-v3 counted view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/view/counted.h>
#include <fermat/iterator/counted_iterator.h>
#include <fermat/utility/copy.h>
#include <fermat/iterator/operations.h>

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
/// fortytwo_erator: an input iterator where post‑increment returns void
/// ------------------------------------------------------------
struct fortytwo_erator {
    using difference_type = int;
    using value_type = int;

    fortytwo_erator() = default;

    int operator*() const { return 42; }
    fortytwo_erator &operator++() { return *this; }

    void operator++(int) {
    }
};

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
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

/// ------------------------------------------------------------
/// Helper: ignore unused variables (suppress warnings)
/// ------------------------------------------------------------
template<typename... T>
void ignore_unused(T &&...) {
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(CountedTest, BasicCountedView) {
    using namespace fermat::ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng = views::counted(ForwardIterator<int *>{rgi}, 10);
    rng.size(); // ensure it compiles

    static_assert(sized_range<decltype(rng)> && view_<decltype(rng)>, "");
    auto i = rng.begin();
    auto b = i.base();
    auto c = i.count();
    decltype(i) j{b, c};
    (void) j; // avoid unused variable warning
    check_equal(rng, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    static_assert(std::is_same<decltype(i), counted_iterator<ForwardIterator<int *> > >::value, "");
}

TEST(CountedTest, IteratorArithmetic) {
    using namespace fermat::ranges;

    std::list<int> l;
    counted_iterator<std::list<int>::iterator> a(l.begin(), 0);
    counted_iterator<std::list<int>::const_iterator> b(l.begin(), 0);

    ignore_unused(
        a - a,
        b - b,
        a - b,
        b - a);

    counted_iterator<char *> c(nullptr, 0);
    counted_iterator<char const *> d(nullptr, 0);
    ignore_unused(
        c - c,
        d - d,
        c - d,
        d - c);
}

TEST(CountedTest, VoidPostIncrement) {
    using namespace fermat::ranges;

    static_assert(input_iterator<fortytwo_erator>, "");
    counted_iterator<fortytwo_erator> c{{}, 42};
    c++; // post‑increment returns void
    SUCCEED();
}
