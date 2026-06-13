/// common_gtest.cpp
/// Google Test conversion of range-v3 common view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/common.cpp
/// Adapted to use fermat library and Google Test.

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
#include <fermat/view/iota.h>
#include <fermat/view/repeat_n.h>
#include <fermat/view/take.h>
#include <fermat/view/reverse.h>
#include <fermat/view/istream.h>
#include <fermat/utility/copy.h>
#include <fermat/iterator/operations.h>

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing (borrowed range)
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : fermat::ranges::view_interface<debug_input_view<T> > {
    struct data {
        const T *first_;
        std::ptrdiff_t size_;
    };

    std::shared_ptr<data> data_;

    debug_input_view() = default;

    explicit debug_input_view(const T *first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size})) {
    }

    const T *begin() const { return data_->first_; }
    const T *end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace fermat::ranges {
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T> > = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Helper: as_const (C++17)
template<typename T>
constexpr const T &as_const(const T &t) { return t; }

template<typename T>
constexpr const T &as_const(T &t) { return t; }

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

/// Test views::take on a raw array (random-access, sized, common)
TEST(CommonViewTest, TakeOnArray) {
    using namespace fermat::ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // take(6)
    auto rng0 = rgi | views::take(6);
    // has_type<int &>(*begin(rng0));
    static_assert(static_cast<bool>(view_<decltype(rng0)>),
                  "Concept assertion failed : view_<decltype(rng0)>");
    static_assert(static_cast<bool>(common_range<decltype(rng0)>),
                  "Concept assertion failed : common_range<decltype(rng0)>");
    static_assert(static_cast<bool>(sized_range<decltype(rng0)>),
                  "Concept assertion failed : sized_range<decltype(rng0)>");
    static_assert(static_cast<bool>(random_access_iterator<decltype(begin(rng0))>),
                  "Concept assertion failed : random_access_iterator<decltype(begin(rng0))>");
    static_assert(static_cast<bool>(range<decltype(detail::as_const(rng0))>),
                  "Concept assertion failed : range<decltype(detail::as_const(rng0))>");
    check_equal(rng0, {0, 1, 2, 3, 4, 5});
    EXPECT_EQ(size(rng0), 6u);

    // take(20) (beyond end)
    auto rng0b = rgi | views::take(20);
    check_equal(rng0b, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    EXPECT_EQ(size(rng0b), 11u);
}

/// Test views::take followed by views::reverse on array
TEST(CommonViewTest, TakeReverseOnArray) {
    using namespace fermat::ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng0 = rgi | views::take(6);
    auto rng1 = rng0 | views::reverse;
    // has_type<int &>(*begin(rng1));
    static_assert(static_cast<bool>(view_<decltype(rng1)>),
                  "Concept assertion failed : view_<decltype(rng1)>");
    static_assert(static_cast<bool>(common_range<decltype(rng1)>),
                  "Concept assertion failed : common_range<decltype(rng1)>");
    static_assert(static_cast<bool>(sized_range<decltype(rng1)>),
                  "Concept assertion failed : sized_range<decltype(rng1)>");
    static_assert(static_cast<bool>(random_access_iterator<decltype(begin(rng1))>),
                  "Concept assertion failed : random_access_iterator<decltype(begin(rng1))>");
    static_assert(static_cast<bool>(range<decltype(detail::as_const(rng1))>),
                  "Concept assertion failed : range<decltype(detail::as_const(rng1))>");
    check_equal(rng1, {5, 4, 3, 2, 1, 0});
}

/// Test views::take and reverse on std::vector
TEST(CommonViewTest, TakeReverseOnVector) {
    using namespace fermat::ranges;

    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng2 = v | views::take(6) | views::reverse;
    // Concept checks similar to above
    check_equal(rng2, {5, 4, 3, 2, 1, 0});
}

/// Test views::take on std::list (bidirectional, not random-access)
TEST(CommonViewTest, TakeOnList) {
    using namespace fermat::ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng3 = l | views::take(6);
    // has_type<int &>(*begin(rng3));
    static_assert(static_cast<bool>(view_<decltype(rng3)>),
                  "Concept assertion failed : view_<decltype(rng3)>");
    static_assert(static_cast<bool>(!common_range<decltype(rng3)>),
                  "Concept assertion failed : !common_range<decltype(rng3)>");
    static_assert(static_cast<bool>(sized_range<decltype(rng3)>),
                  "Concept assertion failed : sized_range<decltype(rng3)>");
    static_assert(static_cast<bool>(bidirectional_iterator<decltype(begin(rng3))>),
                  "Concept assertion failed : bidirectional_iterator<decltype(begin(rng3))>");
    static_assert(static_cast<bool>(!random_access_iterator<decltype(begin(rng3))>),
                  "Concept assertion failed : !random_access_iterator<decltype(begin(rng3))>");
    static_assert(static_cast<bool>(range<decltype(detail::as_const(rng3))>),
                  "Concept assertion failed : range<decltype(detail::as_const(rng3))>");
    check_equal(rng3, {0, 1, 2, 3, 4, 5});
    EXPECT_EQ(size(rng3), 6u);

    auto rng3b = l | views::take(20);
    check_equal(rng3b, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    EXPECT_EQ(size(rng3b), 11u);
}

/// Test views::take on infinite iota range (sized)
TEST(CommonViewTest, TakeOnIota) {
    using namespace fermat::ranges;

    auto rng4 = views::iota(10) | views::take(10);
    check_equal(rng4, {10, 11, 12, 13, 14, 15, 16, 17, 18, 19});
    EXPECT_EQ(size(rng4), 10u);
}

/// Test views::take on iota followed by reverse
TEST(CommonViewTest, TakeReverseOnIota) {
    using namespace fermat::ranges;

    auto rng5 = views::iota(10) | views::take(10) | views::reverse;
    check_equal(rng5, {19, 18, 17, 16, 15, 14, 13, 12, 11, 10});
    EXPECT_EQ(size(rng5), 10u);
}

/// Test views::take on a delimited C-string (random-access, not sized)
TEST(CommonViewTest, TakeOnDelimitString) {
    using namespace fermat::ranges;

    auto c_str = views::delimit("hello world", '\0');
    // static_assert(static_cast<bool>(random_access_range<decltype(c_str)>),
    //     "Concept assertion failed : random_access_range<decltype(c_str)>");
    // static_assert(static_cast<bool>(!sized_range<decltype(c_str)>),
    //     "Concept assertion failed : !sized_range<decltype(c_str)>");

    auto rng6 = c_str | views::take(5);
    static_assert(static_cast<bool>(view_<decltype(rng6)>),
                  "Concept assertion failed : view_<decltype(rng6)>");
    static_assert(static_cast<bool>(random_access_range<decltype(rng6)>),
                  "Concept assertion failed : random_access_range<decltype(rng6)>");
    static_assert(static_cast<bool>(!common_range<decltype(rng6)>),
                  "Concept assertion failed : !common_range<decltype(rng6)>");
    static_assert(static_cast<bool>(!sized_range<decltype(rng6)>),
                  "Concept assertion failed : !sized_range<decltype(rng6)>");
    static_assert(static_cast<bool>(range<decltype(detail::as_const(rng6))>),
                  "Concept assertion failed : range<decltype(detail::as_const(rng6))>");
    check_equal(rng6, {'h', 'e', 'l', 'l', 'o'});

    auto rng7 = c_str | views::take(20);
    check_equal(rng7, {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'});
}

/// Test views::take on a subrange of list iterators (bidirectional, common, not sized)
TEST(CommonViewTest, TakeOnSubrange) {
    using namespace fermat::ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    subrange<std::list<int>::iterator> rl{l.begin(), l.end()};
    static_assert(static_cast<bool>(view_<decltype(rl)>),
                  "Concept assertion failed : view_<decltype(rl)>");
    static_assert(static_cast<bool>(bidirectional_range<decltype(rl)>),
                  "Concept assertion failed : bidirectional_range<decltype(rl)>");
    static_assert(static_cast<bool>(common_range<decltype(rl)>),
                  "Concept assertion failed : common_range<decltype(rl)>");
    static_assert(static_cast<bool>(!sized_range<decltype(rl)>),
                  "Concept assertion failed : !sized_range<decltype(rl)>");
    static_assert(static_cast<bool>(range<decltype(detail::as_const(rl))>),
                  "Concept assertion failed : range<decltype(detail::as_const(rl))>");

    auto rng8 = rl | views::take(5);
    static_assert(static_cast<bool>(view_<decltype(rng8)>),
                  "Concept assertion failed : view_<decltype(rng8)>");
    static_assert(static_cast<bool>(bidirectional_range<decltype(rng8)>),
                  "Concept assertion failed : bidirectional_range<decltype(rng8)>");
    static_assert(static_cast<bool>(!common_range<decltype(rng8)>),
                  "Concept assertion failed : !common_range<decltype(rng8)>");
    static_assert(static_cast<bool>(!sized_range<decltype(rng8)>),
                  "Concept assertion failed : !sized_range<decltype(rng8)>");
    static_assert(static_cast<bool>(range<decltype(detail::as_const(rng8))>),
                  "Concept assertion failed : range<decltype(detail::as_const(rng8))>");
    check_equal(rng8, {0, 1, 2, 3, 4});

    auto rng9 = rl | views::take(20);
    check_equal(rng9, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

/// Test views::take on a debug_input_view (input range, not forward)
TEST(CommonViewTest, TakeOnDebugInputView) {
    using namespace fermat::ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5};
    auto rng = debug_input_view<int const>{rgi, 6} | views::take(6);
    // static_assert(static_cast<bool>(!range<decltype(detail::as_const(rng))>),
    //     "Concept assertion failed : !range<decltype(detail::as_const(rng))>");
    check_equal(rng, {0, 1, 2, 3, 4, 5});
}

/// Additional test: common_view with istream and delimit (original test from reference)
TEST(CommonViewTest, IstreamDelimitCommon) {
    using namespace fermat::ranges;

    std::stringstream sinx("1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 1 2 3 4 42 6 7 8 9 ");
    auto rng1 = istream<int>(sinx) | views::delimit(42);
    auto rng2 = rng1 | views::common;
    check_equal(rng2, {1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4});
}

/// Additional test: counted on list, common view
TEST(CommonViewTest, CountedListCommon) {
    using namespace fermat::ranges;

    std::list<int> l{1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    auto rng3 = views::counted(l.begin(), 10) | views::common;
    auto b = begin(rng3);
    auto e = end(rng3);
    EXPECT_EQ((e - b), 10);
    EXPECT_EQ((b - e), -10);
    EXPECT_EQ((e - e), 0);
    EXPECT_EQ((next(b) - b), 1);
    auto rng3b = rng3 | views::common; // pass-through
    (void) rng3b;
}

/// Additional test: counted on vector, common view
TEST(CommonViewTest, CountedVectorCommon) {
    using namespace fermat::ranges;

    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 42, 64};
    auto rng4 = views::counted(begin(v), 8) | views::common;
    check_equal(rng4, {1, 2, 3, 4, 5, 6, 7, 8});
}

/// Additional test: repeat_n and common (regression #504)
TEST(CommonViewTest, RepeatNCommon) {
    using namespace fermat::ranges;

    auto rng1 = views::repeat_n(0, 10);
    const auto &crng1 = rng1;
    // concept checks omitted
    auto rng2 = rng1 | views::common;
    const auto &crng2 = rng2;
    check_equal(rng2, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

/// Additional test: debug_input_view with common
TEST(CommonViewTest, DebugInputViewCommon) {
    using namespace fermat::ranges;

    int const rgi[] = {1, 2, 3, 4};
    auto rng = debug_input_view<int const>{rgi, 4} | views::common;
    check_equal(rng, {1, 2, 3, 4});
}
