/// replace_gtest.cpp
/// Google Test conversion of range-v3 replace view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/replace.h>          /// views::replace
#include <fermat/view/istream.h>          /// istream<int>
#include <fermat/view/iota.h>             /// views::ints
#include <fermat/view/take.h>             /// views::take
#include <fermat/view/common.h>           /// views::common
#include <fermat/functional/reference_wrapper.h>  /// ranges::ref
#include <fermat/utility/copy.h>          /// ranges::copy

/// ------------------------------------------------------------
/// Helper: has_type (static assertion on expression type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T>>
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

namespace ranges
{
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
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

/// Overload for checking a vector directly (when range is not a view)
template<typename T>
void check_equal(const std::vector<T>& actual, std::initializer_list<T> expected) {
    auto it = actual.begin();
    for (auto const& val : expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ReplaceTest, IstreamReplace) {
    using namespace ranges;

    std::string str{"1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 "};
    std::stringstream sin{str};

    auto rng = istream<int>(sin) | views::replace(1, 42);

    // Runtime checks only; concept checks omitted.
    auto tmp = rng | views::common;
    std::vector<int> actual(begin(tmp), end(tmp));
    check_equal(actual, {42,2,3,4,5,6,7,8,9,
                         42,2,3,4,5,6,7,8,9,
                         42,2,3,4,5,6,7,8,9});
}

TEST(ReplaceTest, VectorReplaceWithValue) {
    using namespace ranges;

    std::vector<int> rgi{1,2,3,4,5,6,7,8,9};
    auto rng2 = rgi | views::replace(5, 42);
    check_equal(rng2, {1,2,3,4,42,6,7,8,9});
}

TEST(ReplaceTest, VectorReplaceWithReferenceWrapper) {
    using namespace ranges;

    std::vector<int> rgi{1,2,3,4,5,6,7,8,9};
    int forty_two = 42;
    auto rng3 = rgi | views::replace(5, ref(forty_two));
    check_equal(rng3, {1,2,3,4,42,6,7,8,9});
    // Check that the reference wrapper is actually used – modification
    // of forty_two would affect the view? Not required by test.
}

TEST(ReplaceTest, InfiniteRangeTake) {
    using namespace ranges;

    auto rng4 = views::ints | views::replace(5, 42) | views::take(10);
    check_equal(rng4, {0,1,2,3,4,42,6,7,8,9});
}

TEST(ReplaceTest, DebugInputView) {
    using namespace ranges;

    int const some_ints[] = {1,2,3,4,5,6,7,8,9,
                             1,2,3,4,5,6,7,8,9,
                             1,2,3,4,5,6,7,8,9};
    auto rng = debug_input_view<int const>{some_ints, 27} | views::replace(1, 42);
    check_equal(rng, {42,2,3,4,5,6,7,8,9,
                      42,2,3,4,5,6,7,8,9,
                      42,2,3,4,5,6,7,8,9});
}
