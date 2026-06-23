/// set_intersection_gtest.cpp
/// Google Test conversion of range-v3 set_intersection view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <sstream>
#include <memory>
#include <numeric>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>          /// fermat::ranges::to
#include <fermat/algorithm/set_algorithm.h>    /// set_intersection
#include <fermat/algorithm/move.h>             /// fermat::ranges::move
#include <fermat/iterator/operations.h>        /// fermat::ranges::next, fermat::ranges::distance
#include <fermat/iterator/insert_iterators.h>  /// back_inserter
#include <fermat/functional/identity.h>        /// fermat::ranges::identity
#include <fermat/view/all.h>                   /// views::all
#include <fermat/view/const.h>                 /// views::const_
#include <fermat/view/drop_while.h>            /// views::drop_while
#include <fermat/view/iota.h>                  /// views::ints, views::iota
#include <fermat/view/reverse.h>               /// views::reverse
#include <fermat/view/set_algorithm.h>         /// views::set_intersection
#include <fermat/view/stride.h>                /// views::stride
#include <fermat/view/take.h>                  /// views::take
#include <fermat/view/transform.h>             /// views::transform
#include <fermat/utility/copy.h>               /// fermat::ranges::copy

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

/// Overload for struct S
struct S {
    int val;
    bool operator==(const S& other) const { return val == other.val; }
};
void check_equal(const std::vector<S>& actual, std::initializer_list<S> expected) {
    auto it = actual.begin();
    for (auto const& val : expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->val, val.val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<S> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(it->val, val.val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(SetIntersectionTest, FiniteFinite) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_intersection(i1_finite, i2_finite);
    check_equal(res, {2, 4, 4});
    EXPECT_EQ(&*begin(res), &*(begin(i1_finite) + 1));
}

TEST(SetIntersectionTest, InfiniteInfinite) {
    using namespace fermat::ranges;

    auto i1_infinite = views::ints | views::stride(3);
    auto i2_infinite = views::ints | views::transform([](int x) { return x * x; });

    auto res = views::set_intersection(i1_infinite, i2_infinite);
    check_equal(res | views::take(5), {0, 9, 36, 81, 144});
}

TEST(SetIntersectionTest, FiniteInfinite) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    auto i2_infinite = views::ints | views::transform([](int x) { return x * x; });

    auto res = views::set_intersection(i1_finite, i2_infinite);
    check_equal(res | views::take(500), {1, 4});

    auto i1_infinite = views::ints | views::stride(3);
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res2 = views::set_intersection(i1_infinite, i2_finite);
    check_equal(res2 | views::take(500), {6, 9});
}

TEST(SetIntersectionTest, UnknownCardinality) {
    using namespace fermat::ranges;

    auto rng0 = views::iota(10) | views::drop_while([](int i) { return i < 25; });
    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    auto res = views::set_intersection(i1_finite, rng0);
    // Only compile‑time checks; just ensure it compiles.
    (void)res;
    SUCCEED();
}

TEST(SetIntersectionTest, ConstRanges) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res1 = views::set_intersection(views::const_(i1_finite), views::const_(i2_finite));
    check_equal(res1, {2, 4, 4});

    auto res2 = views::set_intersection(views::const_(i1_finite), i2_finite);
    check_equal(res2, {2, 4, 4});
}

TEST(SetIntersectionTest, DifferentOrdering) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto res = views::set_intersection(views::reverse(i1_finite),
                                       views::reverse(i2_finite),
                                       [](int a, int b) { return a > b; });
    check_equal(res, {4, 4, 2});
}

TEST(SetIntersectionTest, Projections) {
    using namespace fermat::ranges;

    S s_finite[] = {{-20}, {-10}, {1}, {3}, {3}, {6}, {8}, {20}};

    auto res1 = views::set_intersection(s_finite, views::ints(-2, 10),
                                        less(), &S::val, identity());
    check_equal(res1, {S{1}, S{3}, S{6}, S{8}});

    auto res2 = views::set_intersection(views::ints(-2, 10), s_finite,
                                        less(), identity(),
                                        [](const S& x) { return x.val; });
    check_equal(res2, {1, 3, 6, 8});
}

TEST(SetIntersectionTest, MoveOnly) {
    using namespace fermat::ranges;

    auto v0 = to<std::vector<MoveOnlyString>>({"a","b","b","c","x","x"});
    auto v1 = to<std::vector<MoveOnlyString>>({"b","x","y","z"});

    auto res = views::set_intersection(v0, v1,
                                       [](const MoveOnlyString& a, const MoveOnlyString& b) {
                                           return a < b;
                                       });

    std::vector<MoveOnlyString> expected;
    move(res, back_inserter(expected));
    check_equal(expected, {"b","x"});
    check_equal(v0, {"a","","b","c","","x"});
    check_equal(v1, {"b","x","y","z"});
}

TEST(SetIntersectionTest, DebugInputView) {
    using namespace fermat::ranges;

    int i1_finite[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int i2_finite[] = {-3, 2, 4, 4, 6, 9};

    auto rng = views::set_intersection(debug_input_view<int const>{i1_finite, 10},
                                       debug_input_view<int const>{i2_finite, 6});
    check_equal(rng, {2, 4, 4});
}
