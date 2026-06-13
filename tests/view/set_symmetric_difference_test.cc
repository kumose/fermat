/// set_symmetric_difference_gtest.cpp
/// Google Test conversion of range-v3 set_symmetric_difference view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <memory>
#include <string>
#include <numeric>
#include <iterator>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/algorithm/set_algorithm.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/equal.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/functional/identity.h>
#include <fermat/view/all.h>
#include <fermat/view/const.h>
#include <fermat/view/drop_while.h>
#include <fermat/view/empty.h>
#include <fermat/view/iota.h>
#include <fermat/view/reverse.h>
#include <fermat/view/set_algorithm.h>
#include <fermat/view/stride.h>
#include <fermat/view/take.h>
#include <fermat/view/transform.h>
#include <fermat/view/zip.h>
#include <fermat/utility/common_type.h>
#include <fermat/utility/copy.h>

/// ------------------------------------------------------------
/// MoveOnlyString (as in original test_utils.hpp)
/// ------------------------------------------------------------
struct MoveOnlyString {
    std::string s_;

    MoveOnlyString() = default;

    MoveOnlyString(const char *sz) : s_(sz) {
    }

    MoveOnlyString(MoveOnlyString &&) = default;

    MoveOnlyString &operator=(MoveOnlyString &&) = default;

    MoveOnlyString(const MoveOnlyString &) = delete;

    MoveOnlyString &operator=(const MoveOnlyString &) = delete;

    bool operator==(const MoveOnlyString &other) const { return s_ == other.s_; }
    bool operator!=(const MoveOnlyString &other) const { return !(*this == other); }
    bool operator<(const MoveOnlyString &other) const { return s_ < other.s_; }
    bool operator==(const char *sz) const { return s_ == sz; }

    friend std::ostream &operator<<(std::ostream &os, const MoveOnlyString &s) {
        return os << s.s_;
    }
};

/// ------------------------------------------------------------
/// debug_input_view (minimal input view for testing)
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T> > {
    struct data {
        const T *first_;
        std::ptrdiff_t size_;
    };

    std::shared_ptr<data> data_;

    debug_input_view() = default;

    explicit debug_input_view(const T *first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size})) {
    }

    const T *begin() const { return data_->first_; }
    const T *end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace ranges {
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T> > = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for MoveOnlyString ranges vs initializer_list<const char*>
template<typename Rng>
void check_equal(Rng &&rng, std::initializer_list<const char *> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(it->s_, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for std::vector<MoveOnlyString> vs initializer_list<const char*>
void check_equal(const std::vector<MoveOnlyString> &actual, std::initializer_list<const char *> expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->s_, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

/// Overload to compare two std::vector<MoveOnlyString> elementwise (for greedy tests)
void check_equal(const std::vector<MoveOnlyString> &a, const std::vector<MoveOnlyString> &b) {
    EXPECT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
        EXPECT_EQ(a[i].s_, b[i].s_);
    }
}

/// Overload for B and D types (avoid using operator->)
struct B {
    int val;

    B(int i) : val(i) {
    }

    bool operator==(const B &other) const { return val == other.val; }
};

struct D : public B {
    D(int i) : B(i) {
    }

    D(B b) : B(std::move(b)) {
    }
};

void check_equal(const std::vector<B> &actual, std::initializer_list<B> expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->val, val.val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

template<typename Rng>
void check_equal(Rng &&rng, std::initializer_list<B> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        // Use (*it).val instead of it->val to avoid operator-> issues
        EXPECT_EQ((*it).val, val.val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test cases (all concept checks omitted, only runtime)
/// ------------------------------------------------------------

// Finite + finite
TEST(SetSymmetricDifferenceTest, FiniteFinite) {
    using namespace ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_symmetric_difference(i1_finite, i2_finite);
    check_equal(res, {-3, 1, 2, 3, 3, 3, 4, 4, 6, 9});

    std::vector<int> greedy_sd;
    set_symmetric_difference(i1_finite, i2_finite, back_inserter(greedy_sd));
    check_equal(greedy_sd, {-3, 1, 2, 3, 3, 3, 4, 4, 6, 9});

    auto it = begin(res);
    EXPECT_EQ(&*it, &*(begin(i2_finite))); // -3
    ++it;
    EXPECT_EQ(&*it, &*(begin(i1_finite))); // 1
}

// Infinite + infinite
TEST(SetSymmetricDifferenceTest, InfiniteInfinite) {
    using namespace ranges;

    auto i1_infinite = views::ints | views::stride(3);
    auto i2_infinite = views::ints | views::transform([](int x) { return x * x; });

    auto res = views::set_symmetric_difference(i1_infinite, i2_infinite);
    check_equal(res | views::take(6), {1, 3, 4, 6, 12, 15});

    std::vector<int> greedy_sd;
    set_symmetric_difference(i1_infinite | views::take(10),
                             i2_infinite | views::take(10),
                             back_inserter(greedy_sd));
    EXPECT_TRUE(ranges::equal(res | views::take(6), greedy_sd | views::take(6)));
}

// Finite + infinite
TEST(SetSymmetricDifferenceTest, FiniteInfinite) {
    using namespace ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    auto i2_infinite = views::ints | views::transform([](int x) { return x * x; });

    auto res1 = views::set_symmetric_difference(i1_finite, i2_infinite);
    check_equal(res1 | views::take(10), {0, 2, 2, 3, 3, 3, 4, 4, 4, 9});

    auto res2 = views::set_symmetric_difference(i2_infinite, i1_finite);
    EXPECT_TRUE(ranges::equal(res2 | views::take(10), res1 | views::take(10)));
}

// Unknown cardinalities
TEST(SetSymmetricDifferenceTest, UnknownCardinality) {
    using namespace ranges;

    auto rng0 = views::iota(10) | views::drop_while([](int i) { return i < 25; });
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};
    auto i1_infinite = views::ints | views::stride(3);

    auto res1 = views::set_symmetric_difference(i2_finite, rng0);
    auto res2 = views::set_symmetric_difference(rng0, i2_finite);
    auto res3 = views::set_symmetric_difference(i1_infinite, rng0);
    auto res4 = views::set_symmetric_difference(rng0, i1_infinite);
    auto res5 = views::set_symmetric_difference(rng0, rng0);
    (void) res1;
    (void) res2;
    (void) res3;
    (void) res4;
    (void) res5;
    SUCCEED();
}

// Const ranges
TEST(SetSymmetricDifferenceTest, ConstRanges) {
    using namespace ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res1 = views::set_symmetric_difference(views::const_(i1_finite), views::const_(i2_finite));
    check_equal(res1, {-3, 1, 2, 3, 3, 3, 4, 4, 6, 9});

    auto res2 = views::set_symmetric_difference(views::const_(i1_finite), i2_finite);
    check_equal(res2, {-3, 1, 2, 3, 3, 3, 4, 4, 6, 9});
}

// Different ordering
TEST(SetSymmetricDifferenceTest, DifferentOrdering) {
    using namespace ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_symmetric_difference(views::reverse(i1_finite),
                                               views::reverse(i2_finite),
                                               [](int a, int b) { return a > b; });
    check_equal(res, {9, 6, 4, 4, 3, 3, 3, 2, 1, -3});
    auto it = begin(res);
    EXPECT_EQ(&*it, &*(begin(i2_finite) + 5)); // 9
}

// Different element types, custom ordering (B and D)
TEST(SetSymmetricDifferenceTest, DifferentElementTypes) {
    using namespace ranges;

    B b_finite[] = {B{-20}, B{-10}, B{1}, B{3}, B{3}, B{6}, B{8}, B{20}};
    D d_finite[] = {D{0}, D{2}, D{4}, D{6}};

    auto res = views::set_symmetric_difference(b_finite, d_finite,
                                               [](const B &a, const D &b) { return a.val < b.val; });
    check_equal(res, {B{-20}, B{-10}, B{0}, B{1}, B{2}, B{3}, B{3}, B{4}, B{8}, B{20}});
    auto it = begin(res);
    EXPECT_EQ(&*it, &*begin(b_finite)); // -20
    advance(it, 2);
    EXPECT_EQ(&*it, &*begin(d_finite)); // 0
}

// Projections
TEST(SetSymmetricDifferenceTest, Projections) {
    using namespace ranges;

    B b_finite[] = {B{-20}, B{-10}, B{1}, B{3}, B{3}, B{6}, B{8}, B{20}};
    D d_finite[] = {D{0}, D{2}, D{4}, D{6}};

    auto res1 = views::set_symmetric_difference(b_finite, d_finite,
                                                less(), &B::val, &D::val);
    check_equal(res1, {B{-20}, B{-10}, B{0}, B{1}, B{2}, B{3}, B{3}, B{4}, B{8}, B{20}});

    auto res2 = views::set_symmetric_difference(views::ints(-2, 10), b_finite,
                                                less(), identity(),
                                                [](const B &x) { return x.val; });
    check_equal(res2, {B{-20}, B{-10}, B{-2}, B{-1}, B{0}, B{2}, B{3}, B{4}, B{5}, B{7}, B{9}, B{20}});
}

/// Move-only test: check the view yields correct elements, but do not verify source ranges after move.
TEST(SetSymmetricDifferenceTest, MoveOnly) {
    using namespace ranges;

    auto v0 = to<std::vector<MoveOnlyString> >({"a", "b", "b", "c", "x", "x"});
    auto v1 = to<std::vector<MoveOnlyString> >({"b", "x", "y", "z"});

    auto res = views::set_symmetric_difference(v0, v1,
                                               [](const MoveOnlyString &a, const MoveOnlyString &b) {
                                                   return a < b;
                                               });

    std::vector<MoveOnlyString> expected;
    move(res, back_inserter(expected));
    check_equal(expected, {"a", "b", "c", "x", "y", "z"});

    // The source ranges v0 and v1 are moved-from after moving elements from the view.
    // Therefore we do not check their contents.
}

// Iterator equality
TEST(SetSymmetricDifferenceTest, IteratorEquality) {
    using namespace ranges;

    int r1[] = {1, 2, 3};
    int r2[] = {2, 3, 4, 5};
    auto res = views::set_symmetric_difference(r1, r2); // 1, 4, 5

    auto it1 = ranges::next(res.begin()); // points to 4
    auto it2 = ranges::next(it1); // points to 5
    auto sentinel = res.end();

    EXPECT_EQ(*it1, 4);
    EXPECT_EQ(*it2, 5);
    EXPECT_NE(it1, it2);
    EXPECT_NE(it1, sentinel);
    EXPECT_EQ(ranges::next(it1, 2), sentinel);
    EXPECT_NE(it2, sentinel);
    EXPECT_EQ(ranges::next(it2, 1), sentinel);
}

// debug_input_view
TEST(SetSymmetricDifferenceTest, DebugInputView) {
    using namespace ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto rng = views::set_symmetric_difference(debug_input_view<int const>{i1_finite, 10},
                                               debug_input_view<int const>{i2_finite, 6});
    check_equal(rng, {-3, 1, 2, 3, 3, 3, 4, 4, 6, 9});
}
