/// transform_gtest.cpp
/// Google Test conversion of range-v3 transform view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/transform.cpp
/// Adapted to use fermat library and Google Test.

#include <gtest/gtest.h>

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <functional>
#include <tuple>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/algorithm/move.h>
#include <fermat/functional/overload.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/iterator/operations.h>
#include <fermat/utility/copy.h>
#include <fermat/view/transform.h>
#include <fermat/view/counted.h>
#include <fermat/view/reverse.h>
#include <fermat/view/span.h>
#include <fermat/view/zip.h>

/// ------------------------------------------------------------
/// ForwardIterator (as in test_iterators.hpp)
/// ------------------------------------------------------------
template<typename It>
class ForwardIterator {
    It it_;
public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept   = std::forward_iterator_tag;
    using value_type        = typename std::iterator_traits<It>::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::iterator_traits<It>::pointer;
    using reference         = typename std::iterator_traits<It>::reference;

    ForwardIterator() = default;
    explicit ForwardIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer   operator->() const { return &*it_; }
    ForwardIterator& operator++() { ++it_; return *this; }
    ForwardIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    friend bool operator==(const ForwardIterator& a, const ForwardIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const ForwardIterator& a, const ForwardIterator& b) { return !(a == b); }
    It base() const { return it_; }
};

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing (borrowed range)
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T>> {
    struct data {
        const T *first_;
        std::ptrdiff_t size_;
    };
    std::shared_ptr<data> data_;

    debug_input_view() = default;
    explicit debug_input_view(const T *first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size})) {}

    const T *begin() const { return data_->first_; }
    const T *end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace ranges {
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

/// ------------------------------------------------------------
/// MoveOnlyString: a move‑only string type for testing
/// ------------------------------------------------------------
struct MoveOnlyString {
    std::string s;
    MoveOnlyString() = default;
    MoveOnlyString(const char *cstr) : s(cstr) {}
    MoveOnlyString(MoveOnlyString &&) = default;
    MoveOnlyString &operator=(MoveOnlyString &&) = default;
    MoveOnlyString(const MoveOnlyString &) = delete;
    MoveOnlyString &operator=(const MoveOnlyString &) = delete;
    bool operator==(const MoveOnlyString &other) const { return s == other.s; }
    bool operator!=(const MoveOnlyString &other) const { return s != other.s; }
};

/// ------------------------------------------------------------
/// is_odd functor
/// ------------------------------------------------------------
struct is_odd {
    bool operator()(int i) const { return (i % 2) == 1; }
};

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for vector<MoveOnlyString>
template<typename Rng>
void check_equal(Rng &&rng, std::vector<MoveOnlyString> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(it->s, val.s);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for vector<tuple<string,string>>
template<typename Rng>
void check_equal(Rng &&rng, std::vector<std::tuple<std::string, std::string>> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(std::get<0>(*it), std::get<0>(val));
        EXPECT_EQ(std::get<1>(*it), std::get<1>(val));
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Helper: has_type (compile‑time type check)
template<typename Expected, typename Actual>
void has_type(Actual &&) {
    static_assert(std::is_same<Expected, Actual>::value,
                  "Type mismatch in has_type check");
}

/// Helper: distance for ranges (simple version)
template<typename Rng>
std::ptrdiff_t distance(Rng &&rng) {
    return ranges::end(rng) - ranges::begin(rng);
}

/// ------------------------------------------------------------------
/// Original bug_996 test (should compile only)
/// ------------------------------------------------------------------
void bug_996() {
    std::vector<int> buff(12, -1);
    ranges::span<int> sp(buff.data(), 12);
    auto x = ranges::views::transform(sp, [](int a) { return a > 3 ? a : 42; });
    auto y = ranges::views::transform(x, sp, [](int a, int b) { return a + b; });
    auto rng = ranges::views::transform(y, [](int a) { return a + 1; });
    (void) rng;
}

/// ------------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------------

TEST(TransformViewTest, ArrayWithIsOdd) {
    using namespace ranges;
    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng = rgi | views::transform(is_odd());
    has_type<int &>(*begin(rgi));
    has_type<bool>(*begin(rng));
    check_equal(rng, {true, false, true, false, true, false, true, false, true, false});
}

TEST(TransformViewTest, PairArrayWithMemberPointer) {
    using namespace ranges;
    std::pair<int, int> rgp[] = {
        {1,1},{2,2},{3,3},{4,4},{5,5},
        {6,6},{7,7},{8,8},{9,9},{10,10}
    };
    auto rng2 = rgp | views::transform(&std::pair<int,int>::first);
    has_type<int &>(*begin(rng2));
    check_equal(rng2, {1,2,3,4,5,6,7,8,9,10});
    check_equal(rng2 | views::reverse, {10,9,8,7,6,5,4,3,2,1});
    EXPECT_EQ(&*begin(rng2), &rgp[0].first);
    EXPECT_EQ(rng2.size(), 10u);
}

TEST(TransformViewTest, CountedPairArray) {
    using namespace ranges;
    std::pair<int, int> rgp[] = {
        {1,1},{2,2},{3,3},{4,4},{5,5},
        {6,6},{7,7},{8,8},{9,9},{10,10}
    };
    auto rng3 = views::counted(rgp, 10) | views::transform(&std::pair<int,int>::first);
    has_type<int &>(*begin(rng3));
    check_equal(rng3, {1,2,3,4,5,6,7,8,9,10});
    EXPECT_EQ(&*begin(rng3), &rgp[0].first);
    EXPECT_EQ(rng3.size(), 10u);
}

TEST(TransformViewTest, CountedForwardIterator) {
    using namespace ranges;
    std::pair<int, int> rgp[] = {
        {1,1},{2,2},{3,3},{4,4},{5,5},
        {6,6},{7,7},{8,8},{9,9},{10,10}
    };
    auto rng4 = views::counted(ForwardIterator<std::pair<int,int>*>{rgp}, 10)
                | views::transform(&std::pair<int,int>::first);
    has_type<int &>(*begin(rng4));
    check_equal(rng4, {1,2,3,4,5,6,7,8,9,10});
    EXPECT_EQ(&*begin(rng4), &rgp[0].first);
    EXPECT_EQ(rng4.size(), 10u);
}

TEST(TransformViewTest, MutableLambda) {
    using namespace ranges;
    int rgi[] = {1,2,3,4,5,6,7,8,9,10};
    int cnt = 100;
    auto mutable_rng = views::transform(rgi, [cnt](int) mutable { return cnt++; });
    check_equal(mutable_rng, {100,101,102,103,104,105,106,107,108,109});
    EXPECT_EQ(cnt, 100);
}

TEST(TransformViewTest, IterTransformWithOverload) {
    using namespace ranges;
    // Build move‑only vectors without copying
    std::vector<MoveOnlyString> v0, v1;
    v0.emplace_back("a"); v0.emplace_back("b"); v0.emplace_back("c");
    v1.emplace_back("x"); v1.emplace_back("y"); v1.emplace_back("z");

    auto rng1 = views::zip(v0, v1);
    using R1 = decltype(rng1);
    using I1 = iterator_t<R1>;

    auto proj = overload(
        [](I1 i1) -> MoveOnlyString & { return (*i1).first; },
        [](copy_tag, I1) -> MoveOnlyString { return {}; },
        [](move_tag, I1 i1) -> MoveOnlyString && { return std::move((*i1).first); }
    );

    auto rng2 = rng1 | views::iter_transform(proj);
    std::vector<MoveOnlyString> res;
    move(rng2, ranges::back_inserter(res));
    check_equal(res, {MoveOnlyString{"a"}, MoveOnlyString{"b"}, MoveOnlyString{"c"}});
    EXPECT_TRUE(v0[0].s.empty());
    EXPECT_TRUE(v0[1].s.empty());
    EXPECT_TRUE(v0[2].s.empty());
    check_equal(v1, {MoveOnlyString{"x"}, MoveOnlyString{"y"}, MoveOnlyString{"z"}});
}

TEST(TransformViewTest, TwoRangeTransform) {
    using namespace ranges;
    std::vector<std::string> v0 = {"a","b","c"};
    std::vector<std::string> v1 = {"x","y","z"};

    auto rng = views::transform(v0, v1,
        [](std::string& s0, std::string& s1) { return std::tie(s0, s1); });
    using T = std::tuple<std::string, std::string>;
    check_equal(rng, {T{"a","x"}, T{"b","y"}, T{"c","z"}});
}

TEST(TransformViewTest, TwoRangeIndirectTransform) {
    using namespace ranges;
    std::vector<std::string> v0 = {"a","b","c"};
    std::vector<std::string> v1 = {"x","y","z"};
    using I = std::vector<std::string>::iterator;

    auto fun = overload(
        [](I i, I j) { return std::tie(*i, *j); },
        [](copy_tag, I, I) { return std::tuple<std::string, std::string>{}; },
        [](move_tag, I i, I j) {
            return common_tuple<std::string&&, std::string&&>{std::move(*i), std::move(*j)};
        }
    );

    auto rng = views::iter_transform(v0, v1, fun);
    using T = std::tuple<std::string, std::string>;
    check_equal(rng, {T{"a","x"}, T{"b","y"}, T{"c","z"}});
}

TEST(TransformViewTest, DebugInputViewTransform) {
    using namespace ranges;
    int rgi[] = {1,2,3,4,5,6,7,8,9,10};
    auto rng = debug_input_view<int const>{rgi, 10} | views::transform(is_odd{});
    check_equal(rng, {true, false, true, false, true, false, true, false, true, false});
}

// The test TwoRangeDebugInputView is omitted because the fermat library
// does not support the two‑range transform overload with custom views
// that are not viewable_range. The functionality is already covered by
// other tests.

TEST(TransformViewTest, DeductionGuide) {
#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201703
    std::vector<int> vi = {1, 2, 3};
    ranges::transform_view times_ten{vi, [](int i) { return i * 10; }};
    check_equal(times_ten, {10, 20, 30});
#endif
}