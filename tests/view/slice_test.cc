/// slice_gtest.cpp
/// Google Test conversion of range-v3 slice view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/view/slice.h>            /// views::slice
#include <fermat/view/iota.h>             /// views::iota, views::closed_iota
#include <fermat/view/istream.h>          /// istream<int>
#include <fermat/view/reverse.h>          /// views::reverse
#include <fermat/view/all.h>              /// views::all
#include <fermat/utility/copy.h>

/// ------------------------------------------------------------
/// Helper: has_type (static assertion on expression type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// debug_input_view (minimal input view for testing)
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

/// Overload for char (for letters test)
void check_equal(const std::vector<char>& actual, std::initializer_list<char> expected) {
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

TEST(SliceTest, RawArraySlice) {
    using namespace ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng0 = rgi | views::slice(3, 9);

    has_type<int&>(*begin(rng0));
    // concept checks omitted
    check_equal(rng0, {3,4,5,6,7,8});
}

TEST(SliceTest, ReverseOfSlice) {
    using namespace ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng0 = rgi | views::slice(3, 9);
    auto rng1 = rng0 | views::reverse;
    check_equal(rng1, {8,7,6,5,4,3});
}

TEST(SliceTest, VectorSliceThenReverse) {
    using namespace ranges;

    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng2 = v | views::slice(3, 9) | views::reverse;
    check_equal(rng2, {8,7,6,5,4,3});
}

TEST(SliceTest, ListSlice) {
    using namespace ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng3 = l | views::slice(3, 9);
    // bidirectional, not random-access
    check_equal(rng3, {3,4,5,6,7,8});
}

TEST(SliceTest, IotaSlice) {
    using namespace ranges;

    auto rng4 = views::iota(10) | views::slice(10, 20);
    // finite
    check_equal(rng4, {20,21,22,23,24,25,26,27,28,29});
}

TEST(SliceTest, IotaSliceWithBraces) {
    using namespace ranges;

    auto rng5 = views::iota(10)[{10, 20}];
    check_equal(rng5, {20,21,22,23,24,25,26,27,28,29});
}

TEST(SliceTest, ListSliceWithBraces) {
    using namespace ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng6 = views::all(l)[{3, 9}];
    check_equal(rng6, {3,4,5,6,7,8});
}

TEST(SliceTest, ListSliceToEnd) {
    using namespace ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng7 = views::all(l)[{3, end}];
    check_equal(rng7, {3,4,5,6,7,8,9,10});
}

TEST(SliceTest, ListSliceWithNegativeOffset) {
    using namespace ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng8 = views::all(l)[{end-5, end-2}];
    check_equal(rng8, {6,7,8});
}

TEST(SliceTest, InfiniteIotaSliceToEnd) {
    using namespace ranges;

    auto rng9 = views::iota(0)[{0, end}];
    static_assert(ranges::is_infinite<decltype(rng9)>::value, "should be infinite");
    (void)rng9; // just compile
    SUCCEED();
}

TEST(SliceTest, IstreamSlice) {
    using namespace ranges;

    std::string str{"0 1 2 3 4 5 6 7 8 9"};
    std::stringstream sin{str};
    auto rng10 = istream<int>(sin)[{3, 9}];
    check_equal(rng10, {3,4,5,6,7,8});
}

TEST(SliceTest, IstreamSliceToEnd) {
    using namespace ranges;

    std::string str{"0 1 2 3 4 5 6 7 8 9"};
    std::stringstream sin{str};
    auto rng11 = istream<int>(sin)[{3, end}];
    check_equal(rng11, {3,4,5,6,7,8,9});
}

TEST(SliceTest, ClosedIotaSlice) {
    using namespace ranges;

    auto letters = views::closed_iota('a','g');
    static_assert(random_access_range<decltype(letters)> && view_<decltype(letters)>, "");
    static_assert(common_range<decltype(letters)> && view_<decltype(letters)>, "");

    auto res = letters[{2, end-2}];
    std::vector<char> result;
    for (char c : res) result.push_back(c);
    check_equal(result, {'c','d','e'});
}

TEST(SliceTest, DebugInputView) {
    using namespace ranges;

    int const some_ints[] = {0,1,2,3,4,5,6,7,8,9};
    auto rng = debug_input_view<int const>{some_ints, 10} | views::slice(3, 10);
    check_equal(rng, {3,4,5,6,7,8,9});
}
