/// take_exactly_gtest.cpp
/// Google Test conversion of range-v3 take_exactly view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
/// No main() function – entry point expected to be provided externally.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <memory>

#include <fermat/range/access.h>                 /// fermat::ranges::begin, fermat::ranges::end
#include <fermat/range/primitives.h>             /// fermat::ranges::size, fermat::ranges::empty
#include <fermat/range/conversion.h>             /// fermat::ranges::to (if needed)
#include <fermat/view/take_exactly.h>            /// views::take_exactly
#include <fermat/view/iota.h>                    /// views::iota
#include <fermat/view/reverse.h>                 /// views::reverse
#include <fermat/utility/copy.h>                 /// fermat::ranges::copy (if needed)

/// ------------------------------------------------------------
/// has_type: static assertion that a value has given type
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

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

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(TakeExactlyTest, RawArrayTake6) {
    using namespace fermat::ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng0 = rgi | views::take_exactly(6);
    has_type<int&>(*begin(rng0));
    // concept checks omitted
    check_equal(rng0, {0,1,2,3,4,5});
    EXPECT_EQ(size(rng0), 6u);
}

TEST(TakeExactlyTest, RawArrayTake6Reverse) {
    using namespace fermat::ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng0 = rgi | views::take_exactly(6);
    auto rng1 = rng0 | views::reverse;
    has_type<int&>(*begin(rng1));
    check_equal(rng1, {5,4,3,2,1,0});
}

TEST(TakeExactlyTest, VectorTake6Reverse) {
    using namespace fermat::ranges;

    std::vector<int> v{0,1,2,3,4,5,6,7,8,9,10};
    auto rng2 = v | views::take_exactly(6) | views::reverse;
    has_type<int&>(*begin(rng2));
    check_equal(rng2, {5,4,3,2,1,0});
}

TEST(TakeExactlyTest, ListTake6) {
    using namespace fermat::ranges;

    std::list<int> l{0,1,2,3,4,5,6,7,8,9,10};
    auto rng3 = l | views::take_exactly(6);
    has_type<int&>(*begin(rng3));
    // not common, sized, bidirectional, not random‑access
    check_equal(rng3, {0,1,2,3,4,5});
}

TEST(TakeExactlyTest, IotaTake10) {
    using namespace fermat::ranges;

    auto rng4 = views::iota(10) | views::take_exactly(10);
    // finite, common, sized
    static_assert(!fermat::ranges::is_infinite<decltype(rng4)>::value, "");
    check_equal(rng4, {10,11,12,13,14,15,16,17,18,19});
    EXPECT_EQ(size(rng4), 10u);
}

TEST(TakeExactlyTest, IotaTake10Reverse) {
    using namespace fermat::ranges;

    auto rng5 = views::iota(10) | views::take_exactly(10) | views::reverse;
    static_assert(!fermat::ranges::is_infinite<decltype(rng5)>::value, "");
    check_equal(rng5, {19,18,17,16,15,14,13,12,11,10});
    EXPECT_EQ(size(rng5), 10u);
}

TEST(TakeExactlyTest, DebugInputView) {
    using namespace fermat::ranges;

    int rgi[] = {0,1,2,3,4,5,6,7,8,9,10};
    auto rng = debug_input_view<int const>{rgi, 11} | views::take_exactly(6);
    check_equal(rng, {0,1,2,3,4,5});
}
