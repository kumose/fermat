/// cache1_gtest.cpp
/// Google Test conversion of range-v3 cache1 view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <string>
#include <memory>

#include <fermat/range/access.h>                /// fermat::ranges::begin, fermat::ranges::end
#include <fermat/range/traits.h>                /// range_value_t, range_reference_t, etc.
#include <fermat/iterator/traits.h>             /// iterator_traits (if needed)
#include <fermat/view/cache1.h>                 /// views::cache1
#include <fermat/view/transform.h>              /// views::transform
#include <fermat/view/c_str.h>                  /// views::c_str
#include <fermat/view/move.h>                   /// views::move
#include <fermat/algorithm/copy.h>              /// fermat::ranges::copy (if needed)
#include <fermat/utility/swap.h>                /// fermat::ranges::swap

/// ------------------------------------------------------------
/// MoveOnlyString (as in original test)
/// ------------------------------------------------------------
struct MoveOnlyString
{
    std::string s_;
    MoveOnlyString() = default;
    MoveOnlyString(const char* sz) : s_(sz) {}
    MoveOnlyString(MoveOnlyString&&) = default;
    MoveOnlyString& operator=(MoveOnlyString&&) = default;
    MoveOnlyString(const MoveOnlyString&) = delete;
    MoveOnlyString& operator=(const MoveOnlyString&) = delete;

    bool operator==(const MoveOnlyString& other) const { return s_ == other.s_; }
    bool operator!=(const MoveOnlyString& other) const { return !(*this == other); }
    bool operator==(const char* sz) const { return s_ == sz; }
    bool operator!=(const char* sz) const { return !(*this == sz); }
    friend std::ostream& operator<<(std::ostream& os, const MoveOnlyString& s) {
        return os << s.s_;
    }
};

/// ------------------------------------------------------------
/// Helper: check_equal for ranges (simplified)
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

// Overload for MoveOnlyString
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<MoveOnlyString> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(Cache1Test, VectorTransform) {
    using namespace fermat::ranges;

    int count = 0;
    std::vector<int> v{1, 2, 3};
    auto rng = v | views::transform([&count](int i) { ++count; return i; })
                 | views::cache1;

    using Rng = decltype(rng);
    static_assert(!fermat::ranges::range<const Rng>, "");
    static_assert(fermat::ranges::input_range<Rng>, "");
    static_assert(!fermat::ranges::forward_range<Rng>, "");
    static_assert(fermat::ranges::common_range<Rng>, "");
    static_assert(fermat::ranges::view_<Rng>, "");
    static_assert(fermat::ranges::sized_range<Rng>, "");
    static_assert(fermat::ranges::sized_sentinel_for<sentinel_t<Rng>, iterator_t<Rng>>, "");
    static_assert(std::is_same<range_value_t<Rng>, int>::value, "");
    static_assert(std::is_same<range_reference_t<Rng>, int&&>::value, "");
    static_assert(std::is_same<range_rvalue_reference_t<Rng>, int&&>::value, "");

    EXPECT_EQ(count, 0);
    auto it = fermat::ranges::begin(rng);
    EXPECT_EQ(count, 0);
    auto last = fermat::ranges::end(rng);
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(count, 1);
    ++it;
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(count, 2);
    ++it;
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(count, 3);
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(count, 3);
    ++it;
    EXPECT_EQ(count, 3);
    EXPECT_EQ(it, last);
}

TEST(Cache1Test, CStringTransform) {
    using namespace fermat::ranges;

    int count = 0;
    const char* hi = "hi";
    auto rng = views::c_str(hi)
               | views::transform([&count](char ch) { ++count; return ch; })
               | views::cache1;

    using Rng = decltype(rng);
    static_assert(!fermat::ranges::range<const Rng>, "");
    static_assert(fermat::ranges::input_range<Rng>, "");
    static_assert(!fermat::ranges::forward_range<Rng>, "");
    static_assert(!fermat::ranges::common_range<Rng>, "");
    static_assert(fermat::ranges::view_<Rng>, "");
    static_assert(!fermat::ranges::sized_range<Rng>, "");
    static_assert(!fermat::ranges::sized_sentinel_for<sentinel_t<Rng>, iterator_t<Rng>>, "");
    static_assert(std::is_same<range_value_t<Rng>, char>::value, "");
    static_assert(std::is_same<range_reference_t<Rng>, char&&>::value, "");
    static_assert(std::is_same<range_rvalue_reference_t<Rng>, char&&>::value, "");

    EXPECT_EQ(count, 0);
    auto it = fermat::ranges::begin(rng);
    EXPECT_EQ(count, 0);
    auto last = fermat::ranges::end(rng);
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(*it, 'h');
    EXPECT_EQ(count, 1);
    EXPECT_EQ(*it, 'h');
    EXPECT_EQ(count, 1);
    ++it;
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(*it, 'i');
    EXPECT_EQ(count, 2);
    EXPECT_EQ(*it, 'i');
    EXPECT_EQ(count, 2);
    ++it;
    EXPECT_EQ(count, 2);
    EXPECT_EQ(it, last);
}

TEST(Cache1Test, MoveOnlyStringArray) {
    using namespace fermat::ranges;

    int count = 0;
    MoveOnlyString rg[] = {"hello", "world"};
    auto rng = rg
               | views::move
               | views::transform([&count](auto s) {
                    ++count;
                    EXPECT_NE(s, MoveOnlyString(""));
                    return s;
                 })
               | views::cache1;

    using Rng = decltype(rng);
    static_assert(!fermat::ranges::range<const Rng>, "");
    static_assert(fermat::ranges::input_range<Rng>, "");
    static_assert(!fermat::ranges::forward_range<Rng>, "");
    static_assert(fermat::ranges::common_range<Rng>, "");
    static_assert(fermat::ranges::view_<Rng>, "");
    static_assert(fermat::ranges::sized_range<Rng>, "");
    static_assert(fermat::ranges::sized_sentinel_for<sentinel_t<Rng>, iterator_t<Rng>>, "");
    static_assert(std::is_same<range_value_t<Rng>, MoveOnlyString>::value, "");
    static_assert(std::is_same<range_reference_t<Rng>, MoveOnlyString&&>::value, "");
    static_assert(std::is_same<range_rvalue_reference_t<Rng>, MoveOnlyString&&>::value, "");

    EXPECT_EQ(count, 0);
    auto it = fermat::ranges::begin(rng);
    EXPECT_EQ(count, 0);
    auto last = fermat::ranges::end(rng);
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(*it, MoveOnlyString("hello"));
    EXPECT_EQ(count, 1);
    EXPECT_EQ(*it, MoveOnlyString("hello"));
    EXPECT_EQ(count, 1);
    ++it;
    EXPECT_NE(it, last);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(*it, MoveOnlyString("world"));
    EXPECT_EQ(count, 2);
    EXPECT_EQ(*it, MoveOnlyString("world"));
    EXPECT_EQ(count, 2);
    ++it;
    EXPECT_EQ(count, 2);
    EXPECT_EQ(it, last);
}
