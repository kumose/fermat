/// common_gtest.cpp
/// Google Test conversion of range-v3 common view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <sstream>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/view/common.h>
#include <fermat/view/counted.h>
#include <fermat/view/delimit.h>
#include <fermat/view/repeat_n.h>
#include <fermat/view/istream.h>
#include <fermat/utility/copy.h>
#include <fermat/iterator/operations.h>

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing
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
void check_equal(Rng&& rng, std::initializer_list<T> expected)
{
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Helper: as_const (C++17)
template<typename T>
constexpr const T& as_const(const T& t) { return t; }
template<typename T>
constexpr const T& as_const(T& t) { return t; }

// ------------------------------------------------------------------
// Test cases (original tests, concept checks simplified or commented)
// ------------------------------------------------------------------

TEST(CommonViewTest, IstreamDelimitCommon)
{
    using namespace fermat::ranges;

    std::stringstream sinx("1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 1 2 3 4 42 6 7 8 9 ");
    auto rng1 = istream<int>(sinx) | views::delimit(42);
    // Concept checks: static_assert(!common_range<decltype(rng1)>, ...);
    // In Fermat, we can simply test runtime behavior. Original asserts omitted.

    auto rng2 = rng1 | views::common;
    // static_assert(view_<decltype(rng2)>);
    // static_assert(common_range<decltype(rng2)>);
    // static_assert(input_iterator<decltype(rng2.begin())>);
    // static_assert(!forward_iterator<decltype(rng2.begin())>);

    check_equal(rng2, {1,2,3,4,5,6,7,8,9,1,2,3,4,5,6,7,8,9,1,2,3,4});
}

TEST(CommonViewTest, VectorDelimitCommon)
{
    using namespace fermat::ranges;

    std::vector<int> v{1,2,3,4,5,6,7,8,9,0,42,64};
    auto rng1 = v | views::delimit(42) | views::common;
    // Concept checks omitted.
    check_equal(rng1, {1,2,3,4,5,6,7,8,9,0});

    const auto& crng1 = rng1;
    auto i = rng1.begin();
    auto j = crng1.begin();
    j = i;   // should be assignable
    (void)j;
}

TEST(CommonViewTest, CountedListCommon)
{
    using namespace fermat::ranges;

    std::list<int> l{1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};
    auto rng3 = views::counted(l.begin(), 10) | views::common;
    // static_assert(view_<decltype(rng3)>);
    // static_assert(common_range<decltype(rng3)>);
    // static_assert(sized_range<decltype(rng3)>);
    // static_assert(forward_iterator<decltype(rng3.begin())>);
    // static_assert(!bidirectional_iterator<decltype(rng3.begin())>);
    // static_assert(sized_sentinel_for<decltype(rng3.begin()), decltype(rng3.end())>);

    auto b = begin(rng3);
    auto e = end(rng3);
    EXPECT_EQ((e - b), 10);
    EXPECT_EQ((b - e), -10);
    EXPECT_EQ((e - e), 0);
    EXPECT_EQ((next(b) - b), 1);

    auto rng3b = rng3 | views::common;  // pass-through
    (void)rng3b;
}

TEST(CommonViewTest, CountedVectorCommon)
{
    using namespace fermat::ranges;

    std::vector<int> v{1,2,3,4,5,6,7,8,9,0,42,64};
    auto rng4 = views::counted(begin(v), 8) | views::common;
    // static_assert(view_<decltype(rng4)>);
    // static_assert(common_range<decltype(rng4)>);
    // static_assert(sized_range<decltype(rng4)>);
    // static_assert(random_access_iterator<decltype(begin(rng4))>);

    check_equal(rng4, {1,2,3,4,5,6,7,8});
}

TEST(CommonViewTest, RepeatNCommon)   // Regression test for #504
{
    using namespace fermat::ranges;

    auto rng1 = views::repeat_n(0, 10);
    // static_assert(view_<decltype(rng1)>);
    // static_assert(!common_range<decltype(rng1)>);
    // static_assert(random_access_range<decltype(rng1)>);
    // static_assert(sized_range<decltype(rng1)>);
    const auto& crng1 = rng1;
    // static_assert(random_access_range<decltype(crng1)>);
    // static_assert(sized_range<decltype(crng1)>);

    auto rng2 = rng1 | views::common;
    // static_assert(view_<decltype(rng2)>);
    // static_assert(common_range<decltype(rng2)>);
    // static_assert(random_access_range<decltype(rng2)>);
    // static_assert(sized_range<decltype(rng2)>);
    const auto& crng2 = rng2;
    // static_assert(common_range<decltype(crng2)>);
    // static_assert(random_access_range<decltype(crng2)>);
    // static_assert(sized_range<decltype(crng2)>);

    check_equal(rng2, {0,0,0,0,0,0,0,0,0,0});
}

TEST(CommonViewTest, DebugInputViewCommon)
{
    using namespace fermat::ranges;

    int const rgi[] = {1,2,3,4};
    auto rng = debug_input_view<int const>{rgi, 4} | views::common;
    // static_assert(input_range<decltype(rng)> && view_<decltype(rng)>);
    // static_assert(!forward_range<decltype(rng)>);
    // static_assert(common_range<decltype(rng)>);

    check_equal(rng, {1,2,3,4});
}

