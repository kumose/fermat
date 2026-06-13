/// replace_if_gtest.cpp
/// Google Test conversion of range-v3 replace_if view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/replace_if.h>         /// views::replace_if
#include <fermat/view/istream.h>            /// istream<int>
#include <fermat/view/iota.h>               /// views::ints
#include <fermat/view/take.h>               /// views::take
#include <fermat/view/common.h>             /// views::common
#include <fermat/functional/reference_wrapper.h>  /// fermat::ranges::ref
#include <fermat/utility/copy.h>            /// fermat::ranges::copy

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

/// Overload for checking a vector directly (used when range is not a view)
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

TEST(ReplaceIfTest, IstreamReplace) {
    using namespace fermat::ranges;

    std::string str{"1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 "};
    std::stringstream sin{str};

    auto rng = istream<int>(sin) | views::replace_if([](int i){ return i == 1; }, 42);
    has_type<int const&>(*begin(rng));
    static_assert(view_<decltype(rng)>, "");
    static_assert(!sized_range<decltype(rng)>, "");
    static_assert(!common_range<decltype(rng)>, "");
    static_assert(input_iterator<decltype(begin(rng))>, "");
    static_assert(!forward_iterator<decltype(begin(rng))>, "");

    auto tmp = rng | views::common;
    has_type<int const&>(*begin(tmp));
    static_assert(view_<decltype(tmp)>, "");
    static_assert(common_range<decltype(tmp)>, "");
    static_assert(!sized_range<decltype(tmp)>, "");
    static_assert(input_iterator<decltype(begin(tmp))>, "");
    static_assert(!forward_iterator<decltype(begin(tmp))>, "");

    std::vector<int> actual(begin(tmp), end(tmp));
    check_equal(actual, {42,2,3,4,5,6,7,8,9,
                         42,2,3,4,5,6,7,8,9,
                         42,2,3,4,5,6,7,8,9});
}

TEST(ReplaceIfTest, VectorReplaceWithValue) {
    using namespace fermat::ranges;

    std::vector<int> vi{1,2,3,4,5,6,7,8,9};
    auto rng2 = vi | views::replace_if([](int i){ return i == 5; }, 42);

    static_assert(std::is_same<range_value_t<decltype(rng2)>, int>::value, "");
    has_type<int const&>(*begin(rng2));
    has_type<int const&>(iter_move(begin(rng2)));
    static_assert(view_<decltype(rng2)>, "");
    static_assert(sized_range<decltype(rng2)>, "");
    static_assert(common_range<decltype(rng2)>, "");
    static_assert(random_access_iterator<decltype(begin(rng2))>, "");

    check_equal(rng2, {1,2,3,4,42,6,7,8,9});
}

TEST(ReplaceIfTest, VectorReplaceWithReferenceWrapper) {
    using namespace fermat::ranges;

    std::vector<int> vi{1,2,3,4,5,6,7,8,9};
    int forty_two = 42;
    auto rng3 = vi | views::replace_if([](int i){ return i == 5; }, ref(forty_two));

    static_assert(std::is_same<range_value_t<decltype(rng3)>, int>::value, "");
    has_type<int&>(*begin(rng3));
    has_type<int const&>(iter_move(begin(rng3)));
    static_assert(view_<decltype(rng3)>, "");
    static_assert(sized_range<decltype(rng3)>, "");
    static_assert(common_range<decltype(rng3)>, "");
    static_assert(random_access_iterator<decltype(begin(rng3))>, "");

    check_equal(rng3, {1,2,3,4,42,6,7,8,9});
}

TEST(ReplaceIfTest, InfiniteRangeTake) {
    using namespace fermat::ranges;

    auto rng4 = views::ints | views::replace_if([](int i){ return i == 5; }, 42) | views::take(10);

    static_assert(std::is_same<range_value_t<decltype(rng4)>, int>::value, "");
    has_type<int>(*begin(rng4));
    has_type<int>(iter_move(begin(rng4)));
    static_assert(view_<decltype(rng4)>, "");
    static_assert(sized_range<decltype(rng4)>, "");
    static_assert(!common_range<decltype(rng4)>, "");
    static_assert(random_access_iterator<decltype(begin(rng4))>, "");

    check_equal(rng4, {0,1,2,3,4,42,6,7,8,9});
}

TEST(ReplaceIfTest, MutablePredicate) {
    using namespace fermat::ranges;

    int rgi[] = {0,1,2,3,4,5,6,7,8,9};
    bool flag = false;
    auto mutable_only = views::replace_if(rgi, [flag](int) mutable { return flag = !flag; }, 42);
    check_equal(mutable_only, {42,1,42,3,42,5,42,7,42,9});
    static_assert(view_<decltype(mutable_only)>, "");
    static_assert(!view_<decltype(mutable_only) const>, "");
}

TEST(ReplaceIfTest, DebugInputView) {
    using namespace fermat::ranges;

    int const some_ints[] = {1,2,3,4,5,6,7,8,9,
                             1,2,3,4,5,6,7,8,9,
                             1,2,3,4,5,6,7,8,9};
    auto rng = debug_input_view<int const>{some_ints, 27}
             | views::replace_if([](int i){ return i == 1; }, 42);
    check_equal(rng, {42,2,3,4,5,6,7,8,9,
                      42,2,3,4,5,6,7,8,9,
                      42,2,3,4,5,6,7,8,9});
}
