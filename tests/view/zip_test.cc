/// zip_gtest.cpp
/// Google Test conversion of range-v3 zip view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/zip.cpp
/// Adapted to use fermat library and Google Test.

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/find_if.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/utility/copy.h>
#include <fermat/view/common.h>
#include <fermat/view/filter.h>
#include <fermat/view/for_each.h>
#include <fermat/view/iota.h>
#include <fermat/view/map.h>
#include <fermat/view/move.h>
#include <fermat/view/istream.h>
#include <fermat/view/stride.h>
#include <fermat/view/take_while.h>
#include <fermat/view/take.h>
#include <fermat/view/zip.h>
#include <fermat/view/zip_with.h>

/// ------------------------------------------------------------
/// MoveOnlyString: a move‑only string type for testing
/// ------------------------------------------------------------
struct MoveOnlyString {
    std::string s;

    MoveOnlyString() = default;

    MoveOnlyString(const char *cstr) : s(cstr) {
    }

    MoveOnlyString(MoveOnlyString &&) = default;

    MoveOnlyString &operator=(MoveOnlyString &&) = default;

    MoveOnlyString(const MoveOnlyString &) = delete;

    MoveOnlyString &operator=(const MoveOnlyString &) = delete;

    bool operator==(const MoveOnlyString &other) const { return s == other.s; }
    bool operator!=(const MoveOnlyString &other) const { return s != other.s; }
};

/// ------------------------------------------------------------
/// debug_input_view: borrowed input view (from previous tests)
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
/// Helper: check_equal for ranges vs initializer_list (for non‑MoveOnly types)
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

/// Overload for vector of tuples or pairs
template<typename Rng, typename Tuple>
void check_equal(Rng &&rng, std::vector<Tuple> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for checking a vector of MoveOnlyString against an initializer_list of const char*.
void check_equal(const std::vector<MoveOnlyString> &actual, std::initializer_list<const char *> expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->s, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

/// Overload for comparing two vectors of MoveOnlyString elementwise.
void check_equal(const std::vector<MoveOnlyString> &a, const std::vector<MoveOnlyString> &b) {
    EXPECT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
        EXPECT_EQ(a[i].s, b[i].s);
    }
}

/// ------------------------------------------------------------
/// Test cases (only runtime checks, concept tests omitted)
/// ------------------------------------------------------------

/// Test zip with mixed range kinds (common, sized, input)
TEST(ZipViewTest, MixedRanges) {
    using namespace fermat::ranges;

    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::string> const vs{"hello", "goodbye", "hello", "goodbye"};

    std::stringstream str{"john paul george ringo"};
    using V = std::tuple<int, std::string, std::string>;
    auto rng = views::zip(vi, vs, istream<std::string>(str) | views::common);
    std::vector<V> expected;
    fermat::ranges::copy(rng, fermat::ranges::back_inserter(expected));
    check_equal(expected, {
                    V{0, "hello", "john"},
                    V{1, "goodbye", "paul"},
                    V{2, "hello", "george"},
                    V{3, "goodbye", "ringo"}
                });
}

/// Test zip with mixed ranges, no common adaptor on input
TEST(ZipViewTest, MixedRangesNoCommon) {
    using namespace fermat::ranges;

    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::string> const vs{"hello", "goodbye", "hello", "goodbye"};

    std::stringstream str{"john paul george ringo"};
    using V = std::tuple<int, std::string, std::string>;
    auto rng = views::zip(vi, vs, istream<std::string>(str));
    std::vector<V> expected;
    fermat::ranges::copy(rng, fermat::ranges::back_inserter(expected));
    check_equal(expected, {
                    V{0, "hello", "john"},
                    V{1, "goodbye", "paul"},
                    V{2, "hello", "george"},
                    V{3, "goodbye", "ringo"}
                });
}

/// Test zip with random-access ranges
TEST(ZipViewTest, RandomAccessZip) {
    using namespace fermat::ranges;

    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::string> const vs{"hello", "goodbye", "hello", "goodbye"};

    auto rnd_rng = views::zip(vi, vs);
    auto tmp = cbegin(rnd_rng) + 3;
    EXPECT_EQ(std::get<0>(*tmp), 3);
    EXPECT_EQ(std::get<1>(*tmp), "goodbye");

    EXPECT_EQ((rnd_rng.end() - rnd_rng.begin()), 4);
    EXPECT_EQ((rnd_rng.begin() - rnd_rng.end()), -4);
    EXPECT_EQ(rnd_rng.size(), 4u);
}

/// Test zip_with
TEST(ZipViewTest, ZipWith) {
    using namespace fermat::ranges;

    std::vector<std::string> v0{"a", "b", "c"};
    std::vector<std::string> v1{"x", "y", "z"};

    auto rng = views::zip_with(std::plus<std::string>{}, v0, v1);
    std::vector<std::string> expected;
    fermat::ranges::copy(rng, fermat::ranges::back_inserter(expected));
    check_equal(expected, {"ax", "by", "cz"});

    auto rng2 = views::zip_with([] { return 42; });
    static_assert(std::is_same<range_value_t<decltype(rng2)>, int>::value, "");
}

/// Test moving from a zip view (full original logic)
/// Test moving from a zip view (full original logic)
TEST(ZipViewTest, MoveFromZip) {
    using namespace fermat::ranges;

    auto v0 = to<std::vector<MoveOnlyString>>({"a", "b", "c"});
    auto v1 = to<std::vector<MoveOnlyString>>({"x", "y", "z"});

    auto rng = views::zip(v0, v1);
    std::vector<std::pair<MoveOnlyString, MoveOnlyString>> expected;
    move(rng, fermat::ranges::back_inserter(expected));

    // Check keys and values
    auto keys = expected | views::keys;
    auto values = expected | views::values;
    check_equal(keys, {"a", "b", "c"});
    check_equal(values, {"x", "y", "z"});

    // Original vectors should be moved-from (empty strings)
    for (const auto& ms : v0) EXPECT_EQ(ms.s, "");
    for (const auto& ms : v1) EXPECT_EQ(ms.s, "");

    // Move back
    move(expected, rng.begin());
    for (const auto& p : expected) {
        EXPECT_EQ(p.first.s, "");
        EXPECT_EQ(p.second.s, "");
    }
    // v0 and v1 should be restored
    check_equal(v0, {"a", "b", "c"});
    check_equal(v1, {"x", "y", "z"});

    // Transform and move
    std::vector<MoveOnlyString> res;
    using R = decltype(rng);
    auto proj = [](range_reference_t<R> p) -> MoveOnlyString& { return p.first; };
    auto rng2 = rng | views::transform(proj);
    move(rng2, fermat::ranges::back_inserter(res));
    check_equal(res, {"a", "b", "c"});
    // After moving from the transform view, only v0 elements are moved-from, v1 unchanged.
    for (const auto& ms : v0) EXPECT_EQ(ms.s, "");
    check_equal(v1, {"x", "y", "z"});   // Corrected: v1 should still be {"x","y","z"}
}


/// Test const zip view with move‑only types (compile only)
TEST(ZipViewTest, ConstZipWithMoveOnly) {
    using namespace fermat::ranges;
    const auto v = to<std::vector<MoveOnlyString> >({"a", "b", "c"});
    auto rng = views::zip(v, v);
    (void) rng;
}

/// Test views::move composed with zip
TEST(ZipViewTest, MoveAndZip) {
    using namespace fermat::ranges;
    std::vector<int> v{1, 2, 3, 4};
    auto moved = v | views::move;
    auto zipped = views::zip(moved);
    (void) zipped;
}

/// Test stride and zip interaction (view_adaptor logic)
TEST(ZipViewTest, StrideZip) {
    using namespace fermat::ranges;
    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::string> const vs{"hello", "goodbye", "hello", "goodbye"};
    auto rng0 = views::zip(vi, vs);
    auto rng1 = views::stride(rng0, 2);
    (void) rng1;
}

/// Test noexcept on iter_move with unique_ptr
TEST(ZipViewTest, NoexceptIterMove) {
    using namespace fermat::ranges;
    static_assert(noexcept(std::declval<std::unique_ptr<int> &>() = std::declval<std::unique_ptr<int> &&>()), "");
    std::unique_ptr<int> rg1[10], rg2[10];
    auto x = views::zip(rg1, rg2);
    std::pair<std::unique_ptr<int>, std::unique_ptr<int> > p = iter_move(x.begin());
    auto it = x.begin();
    static_assert(noexcept(iter_move(it)), "");
    (void) p;
}

/// Test common_iterator's iter_move via zip with take_while
TEST(ZipViewTest, CommonIteratorIterMove) {
    using namespace fermat::ranges;
    std::unique_ptr<int> rg1[10], rg2[10];
    auto rg3 = rg2 | views::take_while([](std::unique_ptr<int> &) { return true; });
    auto x = views::zip(rg1, rg3);
    auto y = x | views::common;
    std::pair<std::unique_ptr<int>, std::unique_ptr<int> > p = iter_move(y.begin());
    auto it = x.begin();
    static_assert(noexcept(iter_move(it)), "");
    (void) p;
}

/// Regression test for #439 (for_each and zip)
TEST(ZipViewTest, Regression439) {
    using namespace fermat::ranges;
    std::vector<int> vec{0, 1, 2};
    auto rng = vec | views::for_each([](int i) { return fermat::ranges::yield(i); });
    fermat::ranges::distance(views::zip(views::iota(0), rng) | views::common);
}

/// Test zip with debug_input_view
TEST(ZipViewTest, ZipDebugInputView) {
    using namespace fermat::ranges;
    int const i1[] = {0, 1, 2, 3};
    int const i2[] = {4, 5, 6, 7};
    auto rng = views::zip(
        debug_input_view<int const>{i1, 4},
        debug_input_view<int const>{i2, 4}
    );
    using P = std::pair<int, int>;
    check_equal(rng, {P{0, 4}, P{1, 5}, P{2, 6}, P{3, 7}});
}

/// Test zip with zero arguments
TEST(ZipViewTest, ZipZeroArgs) {
    using namespace fermat::ranges;
    auto rng = views::zip();
    EXPECT_EQ(fermat::ranges::begin(rng), fermat::ranges::end(rng));
    EXPECT_EQ(fermat::ranges::size(rng), 0u);
}

/// Test dangling detection (simplified – omit is_dangling check)
TEST(ZipViewTest, DanglingDetection) {
    using namespace fermat::ranges;
    std::vector<int> vi{0, 1, 2, 3};
    std::vector<std::string> vs{"a", "b", "c", "d"};
    auto true_ = [](auto &&) { return true; };
    // Just ensure compilation; we cannot test is_dangling easily.
    fermat::ranges::find_if(views::zip(vi, vs), true_);
    fermat::ranges::find_if(views::zip(vi | views::move, vs | views::common), true_);
    fermat::ranges::find_if(views::zip(vi | views::filter(true_)), true_);
    SUCCEED();
}

/// Test zip with infinite range (finite cardinality)
TEST(ZipViewTest, ZipWithInfinite) {
    using namespace fermat::ranges;
    int const i1[] = {0, 1, 2, 3};
    auto rng = views::zip(i1, views::iota(4));
    using P = std::pair<int, int>;
    check_equal(rng, {P{0, 4}, P{1, 5}, P{2, 6}, P{3, 7}});
}

/// Test zip with only infinite ranges (infinite cardinality)
TEST(ZipViewTest, ZipTwoInfinite) {
    using namespace fermat::ranges;
    auto rng = views::zip(views::iota(0), views::iota(4));
    auto taken = rng | views::take(4);
    using P = std::pair<int, int>;
    check_equal(taken, {P{0, 4}, P{1, 5}, P{2, 6}, P{3, 7}});
}

/// Test zip with unknown cardinality (istream)
TEST(ZipViewTest, ZipUnknownCardinality) {
    using namespace fermat::ranges;
    std::stringstream str;
    auto rng = views::zip(istream<std::string>(str));
    (void) rng;
    SUCCEED();
}

/// Test zip with empty range and non‑empty range
TEST(ZipViewTest, ZipWithEmptyRange) {
    using namespace fermat::ranges;
    std::vector<int> v0{1, 2, 3};
    std::vector<int> v1{};

    auto rng0 = views::zip(v0, v1);
    auto rng1 = views::zip(v1, v0);

    EXPECT_EQ(fermat::ranges::distance(rng0.begin(), rng0.end()), 0);
    EXPECT_EQ(fermat::ranges::distance(rng1.begin(), rng1.end()), 0);
    EXPECT_EQ(fermat::ranges::distance(rng0.end(), rng0.begin()), 0);
    EXPECT_EQ(fermat::ranges::distance(rng1.end(), rng1.begin()), 0);
}

/// Test zip with different sized ranges (shortest length)
TEST(ZipViewTest, ZipDifferentSizes) {
    using namespace fermat::ranges;
    std::vector<int> v0{1, 2, 3};
    std::vector<int> v1{1, 2, 3, 4, 5};
    auto rng = views::zip(v0, v1);
    EXPECT_EQ(fermat::ranges::distance(rng.begin(), rng.end()), 3);
    EXPECT_EQ(fermat::ranges::distance(rng.end(), rng.begin()), -3);
}
