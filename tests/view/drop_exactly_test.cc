/// drop_exactly_gtest.cpp
/// Google Test conversion of range-v3 drop_exactly view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/view/drop_exactly.h>
#include <fermat/view/iota.h>
#include <fermat/view/join.h>
#include <fermat/view/reverse.h>
#include <fermat/view/take.h>
#include <fermat/view/transform.h>
#include <fermat/view/chunk.h>
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
/// Helper: has_type (static assertion that a value has given type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
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

TEST(DropExactlyTest, RawArrayDrop6)
{
    using namespace ranges;
    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng0 = rgi | views::drop_exactly(6);
    has_type<int&>(*begin(rng0));
    static_assert(view_<decltype(rng0)>, "");
    static_assert(common_range<decltype(rng0)>, "");
    static_assert(sized_range<decltype(rng0)>, "");
    static_assert(random_access_iterator<decltype(begin(rng0))>, "");
    check_equal(rng0, {6, 7, 8, 9, 10});
    EXPECT_EQ(size(rng0), 5u);
}

TEST(DropExactlyTest, RawArrayReverseAfterDrop)
{
    using namespace ranges;
    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng0 = rgi | views::drop_exactly(6);
    auto rng1 = rng0 | views::reverse;
    has_type<int&>(*begin(rng1));
    static_assert(view_<decltype(rng1)>, "");
    static_assert(common_range<decltype(rng1)>, "");
    static_assert(sized_range<decltype(rng1)>, "");
    static_assert(random_access_iterator<decltype(begin(rng1))>, "");
    check_equal(rng1, {10, 9, 8, 7, 6});
}

TEST(DropExactlyTest, VectorDrop6Reverse)
{
    using namespace ranges;
    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng2 = v | views::drop_exactly(6) | views::reverse;
    has_type<int&>(*begin(rng2));
    static_assert(view_<decltype(rng2)>, "");
    static_assert(common_range<decltype(rng2)>, "");
    static_assert(sized_range<decltype(rng2)>, "");
    static_assert(random_access_iterator<decltype(begin(rng2))>, "");
    check_equal(rng2, {10, 9, 8, 7, 6});
}

TEST(DropExactlyTest, ListDrop6)
{
    using namespace ranges;
    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng3 = l | views::drop_exactly(6);
    has_type<int&>(*begin(rng3));
    static_assert(view_<decltype(rng3)>, "");
    static_assert(common_range<decltype(rng3)>, "");
    static_assert(sized_range<decltype(rng3)>, "");
    static_assert(bidirectional_iterator<decltype(begin(rng3))>, "");
    static_assert(!random_access_iterator<decltype(begin(rng3))>, "");
    check_equal(rng3, {6, 7, 8, 9, 10});
}

TEST(DropExactlyTest, IotaInfiniteDrop10)
{
    using namespace ranges;
    auto rng4 = views::iota(10) | views::drop_exactly(10);
    static_assert(view_<decltype(rng4)>, "");
    static_assert(!common_range<decltype(rng4)>, "");
    static_assert(!sized_range<decltype(rng4)>, "");
    static_assert(ranges::is_infinite<decltype(rng4)>::value, "");
    auto b = ranges::begin(rng4);
    EXPECT_EQ(*b, 20);
    EXPECT_EQ(*(b + 1), 21);
}

TEST(DropExactlyTest, IotaDrop10Take10Reverse)
{
    using namespace ranges;
    auto rng5 = views::iota(10) | views::drop_exactly(10) | views::take(10) | views::reverse;
    static_assert(view_<decltype(rng5)>, "");
    static_assert(common_range<decltype(rng5)>, "");
    static_assert(sized_range<decltype(rng5)>, "");
    static_assert(!ranges::is_infinite<decltype(rng5)>::value, "");
    check_equal(rng5, {29, 28, 27, 26, 25, 24, 23, 22, 21, 20});
    EXPECT_EQ(size(rng5), 10u);
}

TEST(DropExactlyTest, ComposeWithChunkTransformJoin)
{
    using namespace ranges;
    // drop_exactly should work with random-access mutable-only Views.
    auto odds = views::iota(0) |
                views::chunk(2) |
                views::transform(views::drop_exactly(1)) |
                views::join;
    (void)odds;
    SUCCEED();
}

TEST(DropExactlyTest, DebugInputView)
{
    using namespace ranges;
    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng = debug_input_view<int const>{rgi, 11} | views::drop_exactly(5);
    using Rng = decltype(rng);
    static_assert(input_range<Rng> && view_<Rng>, "");
    static_assert(sized_range<Rng>, "");
    check_equal(rng, {5, 6, 7, 8, 9, 10});
}
