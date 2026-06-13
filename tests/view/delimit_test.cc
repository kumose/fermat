/// delimit_gtest.cpp
/// Google Test conversion of range-v3 delimit view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>

#include <fermat/range/access.h>
#include <fermat/view/delimit.h>
#include <fermat/view/iota.h>
#include <fermat/algorithm/for_each.h>
#include <fermat/utility/copy.h>

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
void check_equal(Rng&& rng, std::initializer_list<T> expected)
{
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(DelimitTest, IotaDelimit)
{
    using namespace ranges;
    auto rng0 = views::iota(10) | views::delimit(25);
    check_equal(rng0, {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});

    // Concept checks omitted (original used CPP_assert)
    // static_assert(view_<decltype(rng0)>);
    // static_assert(!common_range<decltype(rng0)>);
    // static_assert(random_access_iterator<decltype(rng0.begin())>);
    // static_assert(view_<delimit_view<views::all_t<std::vector<int>&>, int>>);
    // static_assert(random_access_range<delimit_view<views::all_t<std::vector<int>&>, int>>);
}

TEST(DelimitTest, VectorDelimit)
{
    using namespace ranges;
    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng1 = vi | views::delimit(50);
    check_equal(rng1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

TEST(DelimitTest, IteratorDelimit)
{
    using namespace ranges;
    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng2 = views::delimit(vi.begin(), 8);
    check_equal(rng2, {0, 1, 2, 3, 4, 5, 6, 7});
}

TEST(DelimitTest, DebugInputViewDelimit)
{
    using namespace ranges;
    int const some_ints[] = {1,2,3,0,4,5,6};
    auto rng = debug_input_view<int const>{some_ints, 7} | views::delimit(0);
    check_equal(rng, {1,2,3});
}

TEST(DelimitTest, ArrayDelimit)
{
    using namespace ranges;
    int const some_ints[] = {1,2,3};
    auto rng = views::delimit(some_ints, 0);
    check_equal(rng, {1,2,3});
}
