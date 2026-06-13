/// remove_if_filter_gtest.cpp
/// Google Test conversion of range-v3 remove_if / filter view tests.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <array>
#include <vector>
#include <functional>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/functional/not_fn.h>          /// fermat::ranges::not_fn
#include <fermat/view/remove_if.h>             /// views::remove_if
#include <fermat/view/filter.h>                /// views::filter
#include <fermat/view/counted.h>               /// views::counted
#include <fermat/view/concat.h>                /// views::concat
#include <fermat/view/reverse.h>               /// views::reverse
#include <fermat/utility/copy.h>               /// fermat::ranges::copy (if needed)

/// ------------------------------------------------------------
/// Helper types (is_odd, is_even, my_data)
/// ------------------------------------------------------------
struct is_odd {
    constexpr bool operator()(int i) const { return (i % 2) == 1; }
};

struct is_even {
    constexpr bool operator()(int i) const { return (i % 2) == 0; }
};

struct my_data {
    int i;
    bool operator==(my_data other) const { return i == other.i; }
};

/// ------------------------------------------------------------
/// Minimal forward/bidirectional iterator wrappers (as in test_iterators.hpp)
/// ------------------------------------------------------------
template<typename It>
class BidirectionalIterator {
    It it_;
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept   = std::bidirectional_iterator_tag;
    using value_type        = typename std::iterator_traits<It>::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::iterator_traits<It>::pointer;
    using reference         = typename std::iterator_traits<It>::reference;

    BidirectionalIterator() = default;
    explicit BidirectionalIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer   operator->() const { return &*it_; }
    BidirectionalIterator& operator++() { ++it_; return *this; }
    BidirectionalIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    BidirectionalIterator& operator--() { --it_; return *this; }
    BidirectionalIterator operator--(int) { auto tmp = *this; --it_; return tmp; }
    friend bool operator==(const BidirectionalIterator& a, const BidirectionalIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const BidirectionalIterator& a, const BidirectionalIterator& b) { return !(a == b); }
    It base() const { return it_; }
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
/// Helper: has_type (static assertion on expression type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list (single elements)
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

/// Overload for std::vector<my_data> (used in projection tests)
template<typename T>
void check_equal(const std::vector<my_data>& actual, std::initializer_list<my_data> expected) {
    auto it = actual.begin();
    for (auto const& val : expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->i, val.i);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

/// Overload for constexpr binding test (empty check – just uses CHECK)
/// Not needed separately because we use EXPECT_TRUE(empty(rng)).

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(RemoveIfTest, BasicArrayRemoveEven) {
    using namespace fermat::ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng = rgi | views::remove_if(is_even());

    has_type<int&>(*begin(rgi));
    has_type<int&>(*begin(rng));
    // concept checks omitted
    check_equal(rng, {1,3,5,7,9});
    check_equal(rng | views::reverse, {9,7,5,3,1});
    auto tmp = rng | views::reverse;
    EXPECT_EQ(&*begin(tmp), &rgi[8]);
}

TEST(RemoveIfTest, CountedViewRemoveOdd) {
    using namespace fermat::ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng2 = views::counted(rgi, 10) | views::remove_if(not_fn(is_odd()));

    has_type<int&>(*begin(rng2));
    check_equal(rng2, {1,3,5,7,9});
    EXPECT_EQ(&*begin(rng2), &rgi[0]);
}

TEST(RemoveIfTest, BidirectionalIteratorRemoveEven) {
    using namespace fermat::ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng3 = views::counted(BidirectionalIterator<int*>{rgi}, 10)
              | views::remove_if(is_even());

    has_type<int&>(*begin(rng3));
    check_equal(rng3, {1,3,5,7,9});
    EXPECT_EQ(&*begin(rng3), &rgi[0]);

    auto it = next(begin(rng3));
    EXPECT_EQ(&*prev(it), &rgi[0]);
}

TEST(RemoveIfTest, MutableLambda) {
    using namespace fermat::ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    bool flag = true;
    auto mutable_rng = views::remove_if(rgi, [flag](int) mutable { return flag = !flag; });
    check_equal(mutable_rng, {1,3,5,7,9});
    // const version does not model view (concept check omitted)
}

TEST(RemoveIfTest, ConcatRemoveIf) {
    using namespace fermat::ranges;

    const std::array<int, 3> a{{0, 1, 2}};
    const std::vector<int> b{3, 4, 5, 6};
    auto r = views::concat(a, b);
    auto f = [](int i) { return i != 1 && i != 5; };
    auto r2 = r | views::remove_if(f);
    check_equal(r2, {1,5});
}

TEST(RemoveIfTest, DebugInputView) {
    using namespace fermat::ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng = debug_input_view<int const>{rgi, 10} | views::remove_if(is_even{});
    check_equal(rng, {1,3,5,7,9});
}

TEST(RemoveIfTest, Regression793) {
    using namespace fermat::ranges;

    int const some_ints[] = {1, 2, 3};
    auto a = some_ints | views::remove_if([](int val) { return val > 0; });
    EXPECT_TRUE(a.empty());
}

TEST(RemoveIfTest, Projection) {
    using namespace fermat::ranges;

    const std::vector<my_data> some_my_datas{{1}, {2}, {3}, {4}};

    // without pipe
    {
        auto rng = views::remove_if(some_my_datas, is_even(), &my_data::i);
        check_equal(rng, {my_data{1}, my_data{3}});
    }
    // with pipe
    {
        auto rng = some_my_datas | views::remove_if(is_even(), &my_data::i);
        check_equal(rng, {my_data{1}, my_data{3}});
    }
}

TEST(FilterTest, Projection) {
    using namespace fermat::ranges;

    const std::vector<my_data> some_my_datas{{1}, {2}, {3}, {4}};

    // without pipe
    {
        auto rng = views::filter(some_my_datas, is_even(), &my_data::i);
        check_equal(rng, {my_data{2}, my_data{4}});
    }
    // with pipe
    {
        auto rng = some_my_datas | views::filter(is_even(), &my_data::i);
        check_equal(rng, {my_data{2}, my_data{4}});
    }
}

TEST(RemoveIfTest, ConstexprBinding) {
    using namespace fermat::ranges;

    constexpr std::array<int, 4> is = {{1,2,3,4}};
    constexpr auto filter = views::remove_if(is_even()) | views::remove_if(is_odd());
    auto rng = is | filter;
    EXPECT_TRUE(rng.empty());
}

TEST(RemoveIfTest, ConstexprBindingWithProjection) {
    using namespace fermat::ranges;

    const std::vector<my_data> some_my_datas{{1}, {2}, {3}, {4}};
    constexpr auto filter = views::remove_if(is_even(), &my_data::i) |
                            views::remove_if(is_odd(), &my_data::i);
    auto rng = some_my_datas | filter;
    EXPECT_TRUE(rng.empty());
}

TEST(FilterTest, Issue1424) {
    using namespace fermat::ranges;

    std::vector<int> v{1, 2, 3, 4};
    auto rng = views::filter(v, is_odd());
    check_equal(rng, {1, 3});
}
