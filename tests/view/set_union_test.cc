/// set_union_gtest.cpp
/// Google Test conversion of range-v3 set_union view test.
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
#include <fermat/algorithm/equal.h>          // for fermat::ranges::equal
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/functional/identity.h>
#include <fermat/view/all.h>
#include <fermat/view/const.h>
#include <fermat/view/drop_while.h>
#include <fermat/view/iota.h>
#include <fermat/view/move.h>
#include <fermat/view/reverse.h>
#include <fermat/view/set_algorithm.h>
#include <fermat/view/stride.h>
#include <fermat/view/take.h>
#include <fermat/view/transform.h>
#include <fermat/utility/common_type.h>
#include <fermat/utility/copy.h>

/// ------------------------------------------------------------
/// MoveOnlyString (as in original test_utils.hpp)
/// ------------------------------------------------------------
struct MoveOnlyString {
    std::string s_;
    MoveOnlyString() = default;
    MoveOnlyString(const char* sz) : s_(sz) {}
    MoveOnlyString(MoveOnlyString&&) = default;
    MoveOnlyString& operator=(MoveOnlyString&&) = default;
    MoveOnlyString(const MoveOnlyString&) = delete;
    MoveOnlyString& operator=(const MoveOnlyString&) = delete;

    bool operator==(const MoveOnlyString& other) const { return s_ == other.s_; }
    bool operator!=(const MoveOnlyString& other) const { return !(*this == other); }
    bool operator<(const MoveOnlyString& other) const { return s_ < other.s_; }
    bool operator==(const char* sz) const { return s_ == sz; }
    friend std::ostream& operator<<(std::ostream& os, const MoveOnlyString& s) {
        return os << s.s_;
    }
};

/// ------------------------------------------------------------
/// debug_input_view (minimal input view for testing)
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

/// Overload for std::vector<MoveOnlyString>
void check_equal(const std::vector<MoveOnlyString>& actual, std::initializer_list<const char*> expected) {
    auto it = actual.begin();
    for (auto const& val : expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->s_, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

/// Overload for ranges of MoveOnlyString
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<const char*> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(it->s_, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for B and D types (avoid operator->)
struct B {
    int val;
    B(int i) : val(i) {}
    bool operator==(const B& other) const { return val == other.val; }
};
struct D : public B {
    D(int i) : B(i) {}
    D(B b) : B(std::move(b)) {}
};
void check_equal(const std::vector<B>& actual, std::initializer_list<B> expected) {
    auto it = actual.begin();
    for (auto const& val : expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->val, val.val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<B> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ((*it).val, val.val);   // use (*it).val to avoid operator->
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

// Identity check: compares two views using fermat::ranges::equal
TEST(SetUnionTest, Identity) {
    using namespace fermat::ranges;
    auto i1_infinite = views::ints | views::stride(3);
    auto res = views::set_union(i1_infinite, i1_infinite);
    // Compare two ranges directly
    EXPECT_TRUE(fermat::ranges::equal(res | views::take(100), i1_infinite | views::take(100)));
}

// Finite + finite
TEST(SetUnionTest, FiniteFinite) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_union(i1_finite, i2_finite);
    check_equal(res, {-3, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6, 9});

    // Greedy algorithm
    std::vector<int> greedy_union;
    set_union(i1_finite, i2_finite, back_inserter(greedy_union));
    check_equal(greedy_union, {-3, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6, 9});

    auto it = begin(res);
    EXPECT_EQ(&*it, &*(begin(i2_finite))); // -3
    ++it;
    EXPECT_EQ(&*it, &*(begin(i1_finite))); // 1
}

// Infinite + infinite
TEST(SetUnionTest, InfiniteInfinite) {
    using namespace fermat::ranges;

    auto i1_infinite = views::ints | views::stride(3);
    auto i2_infinite = views::ints | views::transform([](int x) { return x * x; });

    auto res = views::set_union(i1_infinite, i2_infinite);
    check_equal(res | views::take(6), {0, 1, 3, 4, 6, 9});

    // Greedy on finite prefixes
    std::vector<int> greedy_union;
    set_union(i1_infinite | views::take(10),
              i2_infinite | views::take(10),
              back_inserter(greedy_union));
    // Compare two ranges elementwise using fermat::ranges::equal
    EXPECT_TRUE(fermat::ranges::equal(res | views::take(6), greedy_union | views::take(6)));
}

// Finite + infinite
TEST(SetUnionTest, FiniteInfinite) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    auto i2_infinite = views::ints | views::transform([](int x) { return x * x; });

    auto res = views::set_union(i1_finite, i2_infinite);
    check_equal(res | views::take(5), {0, 1, 2, 2, 3});
}

// Infinite + finite
TEST(SetUnionTest, InfiniteFinite) {
    using namespace fermat::ranges;

    auto i1_infinite = views::ints | views::stride(3);
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_union(i1_infinite, i2_finite);
    check_equal(res | views::take(7), {-3, 0, 2, 3, 4, 4, 6});
}

// Unknown cardinalities
TEST(SetUnionTest, UnknownCardinality) {
    using namespace fermat::ranges;

    auto rng0 = views::iota(10) | views::drop_while([](int i) { return i < 25; });
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};
    auto i1_infinite = views::ints | views::stride(3);

    auto res1 = views::set_union(i2_finite, rng0);
    auto res2 = views::set_union(rng0, i2_finite);
    auto res3 = views::set_union(i1_infinite, rng0);
    auto res4 = views::set_union(rng0, i1_infinite);
    auto res5 = views::set_union(rng0, rng0);
    (void)res1; (void)res2; (void)res3; (void)res4;
    // Compare two ranges with fermat::ranges::equal
    EXPECT_TRUE(fermat::ranges::equal(res5 | views::take(100), rng0 | views::take(100)));
}

// Const ranges
TEST(SetUnionTest, ConstRanges) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res1 = views::set_union(views::const_(i1_finite), views::const_(i2_finite));
    check_equal(res1, {-3, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6, 9});

    auto res2 = views::set_union(views::const_(i1_finite), i2_finite);
    check_equal(res2, {-3, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6, 9});
}

// Different ordering
TEST(SetUnionTest, DifferentOrdering) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_union(views::reverse(i1_finite),
                                views::reverse(i2_finite),
                                [](int a, int b) { return a > b; });
    check_equal(res, {9, 6, 4, 4, 4, 4, 3, 3, 3, 2, 2, 1, -3});
}

// Different element types, custom ordering (B and D)
TEST(SetUnionTest, DifferentElementTypes) {
    using namespace fermat::ranges;

    B b_finite[] = {B{-20}, B{-10}, B{1}, B{3}, B{3}, B{6}, B{8}, B{20}};
    D d_finite[] = {D{0}, D{2}, D{4}, D{6}};

    auto res = views::set_union(b_finite, d_finite,
                                [](const B& a, const D& b) { return a.val < b.val; });
    check_equal(res, {B{-20}, B{-10}, B{0}, B{1}, B{2}, B{3}, B{3}, B{4}, B{6}, B{8}, B{20}});
    auto it = begin(res);
    EXPECT_EQ(&*it, &*begin(b_finite)); // -20
    advance(it, 2);
    EXPECT_EQ(&*it, &*begin(d_finite)); // 0
}

// Projections
TEST(SetUnionTest, Projections) {
    using namespace fermat::ranges;

    B b_finite[] = {B{-20}, B{-10}, B{1}, B{3}, B{3}, B{6}, B{8}, B{20}};
    D d_finite[] = {D{0}, D{2}, D{4}, D{6}};

    auto res1 = views::set_union(b_finite, d_finite,
                                 less(), &B::val, &D::val);
    check_equal(res1, {B{-20}, B{-10}, B{0}, B{1}, B{2}, B{3}, B{3}, B{4}, B{6}, B{8}, B{20}});

    auto res2 = views::set_union(views::ints(-2, 10), b_finite,
                                 less(), identity(),
                                 [](const B& x) { return x.val; });
    check_equal(res2, {B{-20}, B{-10}, B{-2}, B{-1}, B{0}, B{1}, B{2}, B{3}, B{3}, B{4}, B{5}, B{6}, B{7}, B{8}, B{9}, B{20}});
}

// Move-only
TEST(SetUnionTest, MoveOnly) {
    using namespace fermat::ranges;

    auto v0 = to<std::vector<MoveOnlyString>>({"a","b","c","x"});
    auto v1 = to<std::vector<MoveOnlyString>>({"b","x","y","z"});

    auto res = views::set_union(v0, v1,
                                [](const MoveOnlyString& a, const MoveOnlyString& b) {
                                    return a < b;
                                });

    std::vector<MoveOnlyString> expected;
    move(res, back_inserter(expected));
    check_equal(expected, {"a","b","c","x","y","z"});
    check_equal(v0, {"","","",""});
    check_equal(v1, {"b","x","",""});
}

// Iterator equality
TEST(SetUnionTest, IteratorEquality) {
    using namespace fermat::ranges;

    int r1[] = {1, 2, 3};
    int r2[] = {2, 3, 4, 5};
    auto res = views::set_union(r1, r2); // 1, 2, 3, 4, 5

    auto it1 = fermat::ranges::next(res.begin(), 3); // *it1 == 4
    auto it2 = fermat::ranges::next(it1);            // *it2 == 5
    auto sentinel = res.end();

    EXPECT_EQ(*it1, 4);
    EXPECT_EQ(*it2, 5);
    EXPECT_NE(it1, it2);
    EXPECT_NE(it1, sentinel);
    EXPECT_EQ(fermat::ranges::next(it1, 2), sentinel);
    EXPECT_NE(it2, sentinel);
    EXPECT_EQ(fermat::ranges::next(it2, 1), sentinel);
}

// debug_input_view
TEST(SetUnionTest, DebugInputView) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto rng = views::set_union(debug_input_view<int const>{i1_finite, 10},
                                debug_input_view<int const>{i2_finite, 6});
    check_equal(rng, {-3, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6, 9});
}
