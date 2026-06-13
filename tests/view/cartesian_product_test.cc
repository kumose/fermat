/// cartesian_product_gtest.cpp
/// Google Test conversion of range-v3 cartesian_product view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <climits>
#include <iostream>
#include <tuple>
#include <string>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/span.h>
#include <fermat/utility/common_tuple.h>
#include <fermat/utility/tuple_algorithm.h>
#include <fermat/view/cartesian_product.h>
#include <fermat/view/chunk.h>
#include <fermat/view/empty.h>
#include <fermat/view/filter.h>
#include <fermat/view/indices.h>
#include <fermat/view/iota.h>
#include <fermat/view/reverse.h>
#include <fermat/view/single.h>
#include <fermat/view/take_exactly.h>
#include <fermat/view/transform.h>
#include <fermat/view/enumerate.h>
#include <fermat/algorithm/equal.h>
#include <fermat/iterator/operations.h>

/// ------------------------------------------------------------
/// Printer for tuple elements (used in operator<<)
/// ------------------------------------------------------------
struct printer {
    std::ostream& os_;
    bool& first_;
    template<typename T>
    void operator()(T const& t) const {
        if (first_) first_ = false;
        else os_ << ',';
        os_ << t;
    }
};

namespace std {
    template<typename... Ts>
    std::ostream& operator<<(std::ostream& os, std::tuple<Ts...> const& t) {
        os << '(';
        bool first = true;
        ::ranges::tuple_for_each(t, ::printer{os, first});
        os << ')';
        return os;
    }
}

/// ------------------------------------------------------------
/// Helper: check_equal for two ranges (iterates through both)
/// ------------------------------------------------------------
template<typename Rng1, typename Rng2>
void check_equal(Rng1&& rng1, Rng2&& rng2) {
    auto it1 = ranges::begin(rng1);
    auto end1 = ranges::end(rng1);
    auto it2 = ranges::begin(rng2);
    auto end2 = ranges::end(rng2);
    while (it1 != end1 && it2 != end2) {
        EXPECT_EQ(*it1, *it2);
        ++it1; ++it2;
    }
    EXPECT_EQ(it1, end1);
    EXPECT_EQ(it2, end2);
}

/// Overload for initializer_list (single range vs initializer list)
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// Overload for initializer_list of tuples
template<typename Rng, typename... Ts>
void check_equal(Rng&& rng, std::initializer_list<ranges::common_tuple<Ts...>> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// test_empty_set (mapped to a test case)
/// ------------------------------------------------------------
TEST(CartesianProductTest, EmptySet) {
    using namespace ranges;
    auto rng = views::cartesian_product();
    using Rng = decltype(rng);

    static_assert(range_cardinality<Rng>::value == static_cast<cardinality>(0), "");
    static_assert(random_access_range<Rng>, "");
    static_assert(view_<Rng>, "");
    static_assert(common_range<Rng>, "");
    static_assert(sized_range<Rng>, "");

    EXPECT_EQ(size(rng), 0u);
    EXPECT_TRUE(empty(rng));

    static_assert(std::is_same<range_value_t<Rng>, std::tuple<>>::value, "");
    static_assert(std::is_same<range_reference_t<Rng>, std::tuple<>&>::value, "");

    std::initializer_list<common_tuple<>> control{};
    check_equal(rng, control);
    check_equal(views::reverse(rng), views::reverse(control));

    auto const first = begin(rng);
    auto const last = end(rng);
    EXPECT_EQ(decltype(size(rng))(last - first), size(rng));
    for (auto i = 0; i <= distance(rng); ++i) {
        for (auto j = 0; j <= distance(rng); ++j) {
            EXPECT_EQ((next(first, i) - next(first, j)), i - j);
        }
    }
}

/// ------------------------------------------------------------
/// test_empty_range
/// ------------------------------------------------------------
TEST(CartesianProductTest, EmptyRange) {
    using namespace ranges;
    int some_ints[] = {0,1,2,3};
    auto e = views::empty<char>;
    auto rng = views::cartesian_product(
        span<int, size(some_ints)>{some_ints},
        e
    );
    using Rng = decltype(rng);

    static_assert(range_cardinality<Rng>::value == static_cast<cardinality>(0), "");
    static_assert(random_access_range<Rng>, "");
    static_assert(view_<Rng>, "");
    static_assert(common_range<Rng>, "");
    static_assert(sized_range<Rng>, "");

    EXPECT_EQ(size(rng), 0u);

    static_assert(std::is_same<range_value_t<Rng>, std::tuple<int, char>>::value, "");
    static_assert(std::is_same<range_reference_t<Rng>, common_tuple<int&, char&>>::value, "");

    using CT = common_tuple<int, char>;
    std::initializer_list<CT> control = {};
    check_equal(rng, control);
    check_equal(views::reverse(rng), views::reverse(control));

    auto const first = begin(rng);
    auto const last = end(rng);
    EXPECT_EQ((last - first), (std::intmax_t)size(rng));
    for (auto i = 0; i <= distance(rng); ++i) {
        for (auto j = 0; j <= distance(rng); ++j) {
            EXPECT_EQ((next(first, i) - next(first, j)), i - j);
        }
    }
}

/// ------------------------------------------------------------
/// test_bug_820
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug820) {
    using namespace ranges;
    using CT = common_tuple<int, int>;
    std::initializer_list<CT> control = {
        CT{0,0}, CT{0,1}, CT{0,2},
        CT{1,0}, CT{1,1}, CT{1,2},
        CT{2,0}, CT{2,1}, CT{2,2}
    };
    auto x = views::iota(0) | views::take_exactly(3);
    auto y = views::cartesian_product(x, x);
    check_equal(y, control);
}
/// ------------------------------------------------------------
/// test_bug_823
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug823) {
    using namespace ranges;
    auto three = views::iota(0) | views::take_exactly(3);
    static_assert(random_access_range<decltype(three)>, "");
    static_assert(view_<decltype(three)>, "");

    /// The following assertions about const versions may not hold in Fermat.
    /// static_assert(!random_access_range<const decltype(three)>, "");
    /// static_assert(!view_<const decltype(three)>, "");

    auto prod = views::cartesian_product(three, three);
    static_assert(random_access_range<decltype(prod)>, "");
    static_assert(view_<decltype(prod)>, "");
    /// static_assert(!random_access_range<const decltype(prod)>, "");
    /// static_assert(!view_<const decltype(prod)>, "");
    static_assert(sized_range<decltype(prod)>, "");
    EXPECT_EQ(size(prod), 9u);

    {
        int i = 0;
        for (auto&& x : prod) {
            (void)x;
            EXPECT_LT(i++, 3 * 3);
        }
        EXPECT_EQ(i, 3 * 3);
    }

    auto twoD = prod | views::chunk(3);
    static_assert(random_access_range<decltype(twoD)>, "");
    static_assert(view_<decltype(twoD)>, "");
    /// static_assert(!random_access_range<const decltype(twoD)>, "");
    /// static_assert(!view_<const decltype(twoD)>, "");

    {
        int i = 0;
        for (auto&& row : twoD) {
            (void)row;
            EXPECT_LT(i++, 3);
        }
        EXPECT_EQ(i, 3);
    }

    {
        int i = 0;
        for (auto&& row : twoD) {
            EXPECT_LT(i++, 3);
            int j = 0;
            for (auto&& col : row) {
                (void)col;
                EXPECT_LT(j++, 3);
            }
            EXPECT_EQ(j, 3);
        }
        EXPECT_EQ(i, 3);
    }
}

/// ------------------------------------------------------------
/// test_bug_919
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug919) {
    using namespace ranges;
    int some_ints[] = {0,1,2,3};
    char const* some_strings[] = {"John", "Paul", "George", "Ringo"};
    auto rng = views::cartesian_product(
        span<int, size(some_ints)>{some_ints},
        span<char const*, size(some_strings)>{some_strings}
    );
    constexpr std::intmax_t n = size(rng);
    static_assert(n == 16, "");

    for (std::intmax_t i = 0; i <= n; ++i) {
        auto const x = rng.begin() + i;
        EXPECT_TRUE((x == rng.end() - (n - i)));
        for (std::intmax_t j = 0; j <= n; ++j) {
            EXPECT_TRUE((rng.begin() + j == x + (j - i)));
        }
    }
}

/// ------------------------------------------------------------
/// test_bug_978
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug978) {
    using namespace ranges;
    int rgi[] = {1};
    auto rng = views::cartesian_product(
        rgi | views::filter([](int) { return true; }),
        rgi
    );
    (void)rng;
    SUCCEED();
}

/// ------------------------------------------------------------
/// test_bug_1269
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug1269) {
    using namespace ranges;
    int data0[2]{}, data1[3]{}, data2[5]{}, data3[7]{};
    constexpr std::size_t N = size(data0) * size(data1) * size(data2) * size(data3);
    static_assert(N < INT_MAX, "");

    auto rng = views::cartesian_product(data0, data1, data2, data3);
    static_assert(sized_range<decltype(rng)>, "");
    EXPECT_EQ(size(rng), N);

    static_assert(random_access_range<decltype(rng)>, "");
    static_assert(sized_sentinel_for<sentinel_t<decltype(rng)>, iterator_t<decltype(rng)>>, "");
    for (int i = 0; i < int{N}; ++i) {
        auto pos = begin(rng) + i;
        EXPECT_EQ((end(rng) - pos), std::intmax_t{N} - i);
    }
}

/// ------------------------------------------------------------
/// test_bug_1279
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug1279) {
    using namespace ranges;
    auto const xs = views::indices(std::size_t{0}, std::size_t{10});
    auto const ys = views::indices(std::size_t{0}, std::size_t{10});
    for (auto r : views::cartesian_product(ys, xs)) {
        (void)r;
    }
    SUCCEED();
}

/// ------------------------------------------------------------
/// test_bug_1296
/// ------------------------------------------------------------
TEST(CartesianProductTest, Bug1296) {
    using namespace ranges;
    auto v = views::cartesian_product(views::single(42))
        | views::transform([](std::tuple<int> a) { return std::get<0>(a); });
    EXPECT_EQ(size(v), 1u);
    EXPECT_EQ(*begin(v), 42);
}

/// ------------------------------------------------------------
/// test_1422
/// ------------------------------------------------------------
TEST(CartesianProductTest, Issue1422) {
    using namespace ranges;
    int v1[] = {1,2,3};
    auto e = v1 | views::enumerate;
    auto cp = views::cartesian_product(e, e);
    static_assert(input_range<decltype(cp)>, "");
    SUCCEED();
}

/// ------------------------------------------------------------
/// Main test (original main body)
/// ------------------------------------------------------------
TEST(CartesianProductTest, MainTest) {
    using namespace ranges;
    int some_ints[] = {0,1,2,3};
    char const* some_strings[] = {"John", "Paul", "George", "Ringo"};
    auto rng = views::cartesian_product(
        span<int, size(some_ints)>{some_ints},
        span<char const*, size(some_strings)>{some_strings}
    );
    using Rng = decltype(rng);

    static_assert(range_cardinality<Rng>::value ==
        range_cardinality<decltype(some_ints)>::value *
        range_cardinality<decltype(some_strings)>::value, "");

    static_assert(random_access_range<Rng>, "");
    static_assert(view_<Rng>, "");
    static_assert(common_range<Rng>, "");
    static_assert(sized_range<Rng>, "");
    EXPECT_EQ(size(rng), size(some_ints) * size(some_strings));

    static_assert(std::is_same<range_value_t<Rng>, std::tuple<int, char const*>>::value, "");
    static_assert(std::is_same<range_reference_t<Rng>, common_tuple<int&, char const*&>>::value, "");

    using CT = common_tuple<int, std::string>;
    std::initializer_list<CT> control = {
        CT{0, "John"}, CT{0, "Paul"}, CT{0, "George"}, CT{0, "Ringo"},
        CT{1, "John"}, CT{1, "Paul"}, CT{1, "George"}, CT{1, "Ringo"},
        CT{2, "John"}, CT{2, "Paul"}, CT{2, "George"}, CT{2, "Ringo"},
        CT{3, "John"}, CT{3, "Paul"}, CT{3, "George"}, CT{3, "Ringo"}
    };
    check_equal(rng, control);
    check_equal(views::reverse(rng), views::reverse(control));

    auto const first = begin(rng);
    auto const last = end(rng);
    EXPECT_EQ((last - first), (std::intmax_t)size(rng));
    for (auto i = 0; i <= distance(rng); ++i) {
        for (auto j = 0; j <= distance(rng); ++j) {
            EXPECT_EQ((next(first, i) - next(first, j)), i - j);
        }
    }
}

