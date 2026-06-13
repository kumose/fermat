/// drop_last_gtest.cpp
/// Google Test conversion of range-v3 drop_last view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <type_traits>
#include <vector>
#include <list>
#include <forward_list>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/drop_last.h>
#include <fermat/view/take_exactly.h>
#include <fermat/view/transform.h>
#include <fermat/view/generate_n.h>
#include <fermat/view/adaptor.h>
#include <fermat/view/all.h>

using namespace ranges;

/// ------------------------------------------------------------
/// view_non_const_only: a view that is only usable as non‑const
/// ------------------------------------------------------------
template<class Rng>
struct view_non_const_only
        : view_adaptor<view_non_const_only<Rng>, Rng> {
private:
    friend range_access;
    adaptor_base begin_adaptor() { return {}; }
    adaptor_base end_adaptor() { return {}; }

public:
    using view_non_const_only::view_adaptor::view_adaptor;

    template<typename R = Rng>
    auto size() -> std::enable_if_t<sized_range<R>, range_size_t<R> > {
        return ranges::size(this->base());
    }
};

template<class Rng>
view_non_const_only<views::all_t<Rng> > non_const_only(Rng &&rng) {
    return view_non_const_only<views::all_t<Rng> >{views::all(static_cast<Rng &&>(rng))};
}

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

/// ------------------------------------------------------------
/// Test helper functions (adapted from original)
/// ------------------------------------------------------------
template<class Rng>
void test_range(Rng &&src) {
    {
        auto src_ = src;
        check_equal(src_, {1, 2, 3, 4});
    }
    {
        auto src_ = src;
        auto list = src_ | views::drop_last(2);
        check_equal(list, {1, 2});
    }
    {
        auto src_ = src;
        auto list = src_ | views::drop_last(0);
        check_equal(list, {1, 2, 3, 4});
    }
    {
        auto src_ = src;
        auto list = src_ | views::drop_last(4);
        EXPECT_TRUE(ranges::empty(list));
    }
    {
        auto src_ = src;
        auto list = src_ | views::drop_last(5);
        EXPECT_TRUE(ranges::empty(list));
    }
}

template<class Rng>
void test_size(Rng &&src) {
    EXPECT_EQ((src | views::drop_last(0)).size(), 4u);
    EXPECT_EQ((src | views::drop_last(2)).size(), 2u);
    EXPECT_EQ((src | views::drop_last(4)).size(), 0u);
    EXPECT_EQ((src | views::drop_last(5)).size(), 0u);
}

template<class Rng>
void test_non_convert_range(Rng &&src) {
    auto tr = src | views::transform([](const int &i) -> const int & { return i; });
    test_range(tr);
}

void random_access_test() {
    using Src = std::vector<int>;
    static_assert(random_access_range<Src>, "Must be exactly RA.");
    static_assert(
        std::is_same<
            drop_last_view<views::all_t<Src &> >,
            drop_last_view<views::all_t<Src &>, detail::drop_last_view::mode_bidi>
        >::value,
        "Must have correct view."
    );

    Src src = {1, 2, 3, 4};
    test_range(src);
    test_range(non_const_only(src));
    test_size(src);
    test_non_convert_range(src);
}

void bidirectional_test() {
    using Src = std::list<int>;
    static_assert(!random_access_range<Src> && bidirectional_range<Src>, "Must be exactly bidirectional.");
    static_assert(
        std::is_same<
            drop_last_view<views::all_t<Src &> >,
            drop_last_view<views::all_t<Src &>, detail::drop_last_view::mode_bidi>
        >::value,
        "Must have correct view."
    );

    Src src = {1, 2, 3, 4};
    test_range(src);
    test_range(non_const_only(src));
    test_size(src);
    test_non_convert_range(src);
}

void forward_test() {
    using Src = std::forward_list<int>;
    static_assert(!bidirectional_range<Src> && forward_range<Src>, "Must be exactly forward.");
    static_assert(
        std::is_same<
            drop_last_view<views::all_t<Src &> >,
            drop_last_view<views::all_t<Src &>, detail::drop_last_view::mode_forward>
        >::value,
        "Must have correct view."
    );

    Src src = {1, 2, 3, 4};
    test_range(src);
    test_range(non_const_only(src));
    test_size(src | views::take_exactly(4));
    test_non_convert_range(src);
}

/// Sized test: uses generate_n to create an input range with known size.
/// Each sub‑test creates its own fresh generator to avoid state pollution.
void sized_test() {
    // Helper that creates a fresh generate_n view each call.
    // The generator captures a shared_ptr<int> to ensure the state outlives the view.
    auto make_src = [] {
        auto state = std::make_shared<int>(0);
        return views::generate_n([state]() mutable -> int { return ++(*state); }, 4);
    };

    // drop_last(2)
    {
        auto src = make_src();
        auto rng = src | views::drop_last(2);
        check_equal(rng, {1, 2});
    }
    // drop_last(0)
    {
        auto src = make_src();
        auto rng = src | views::drop_last(0);
        check_equal(rng, {1, 2, 3, 4});
    }
    // drop_last(4) -> empty
    {
        auto src = make_src();
        auto rng = src | views::drop_last(4);
        EXPECT_TRUE(ranges::empty(rng));
    }
    // drop_last(5) -> empty
    {
        auto src = make_src();
        auto rng = src | views::drop_last(5);
        EXPECT_TRUE(ranges::empty(rng));
    }
    // size checks
    {
        auto src = make_src();
        EXPECT_EQ((src | views::drop_last(0)).size(), 4u);
        EXPECT_EQ((src | views::drop_last(2)).size(), 2u);
        EXPECT_EQ((src | views::drop_last(4)).size(), 0u);
        EXPECT_EQ((src | views::drop_last(5)).size(), 0u);
    }
    // non_const_only
    {
        auto src = make_src();
        auto rng = non_const_only(std::move(src)) | views::drop_last(2);
        check_equal(rng, {1, 2});
    }
    // transform non-convertible
    {
        auto src = make_src();
        auto tr = src | views::transform([](const int &i) -> const int & { return i; });
        auto rng = tr | views::drop_last(2);
        check_equal(rng, {1, 2});
    }
}

// ------------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------------

TEST(DropLastTest, RandomAccess) { random_access_test(); }
TEST(DropLastTest, Bidirectional) { bidirectional_test(); }
TEST(DropLastTest, Forward) { forward_test(); }
TEST(DropLastTest, Sized) { sized_test(); }
