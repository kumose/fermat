// Range v3 library
//
//  Copyright Casey Carter 2016
//  Copyright Eric Niebler 2018
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/subrange.h>
#include <fermat/view/ref.h>
#include <fermat/view/iota.h>
#include <fermat/algorithm/find.h>
#include <vector>
#include <gtest/gtest.h>

#if defined(__clang__)
RANGES_DIAGNOSTIC_IGNORE("-Wunused-const-variable")
#endif

// Helper: custom array type for testing
namespace X {
    template<class T, std::size_t N>
    struct array {
        T elements_[N];
        constexpr bool empty() const noexcept { return N == 0; }
        constexpr T *data() noexcept { return elements_; }
        constexpr T const *data() const noexcept { return elements_; }
    };

    template<class T, std::size_t N>
    constexpr T *begin(array<T, N> &a) noexcept { return a.elements_; }

    template<class T, std::size_t N>
    constexpr T *end(array<T, N> &a) noexcept { return a.elements_ + N; }

    template<class T, std::size_t N>
    constexpr T const *begin(array<T, N> const &a) noexcept { return a.elements_; }

    template<class T, std::size_t N>
    constexpr T const *end(array<T, N> const &a) noexcept { return a.elements_ + N; }
} // namespace X

using I = int *;
using CI = int const *;

// Concept checks (compile-time)
static_assert(ranges::input_or_output_iterator<I>, "I must be an input_or_output_iterator");
static_assert(ranges::input_or_output_iterator<CI>, "CI must be an input_or_output_iterator");

// Test suite for range access
TEST(RangeAccess, ambiguity) {
    std::vector<ranges::reverse_iterator<int *> > vri{};
    using namespace ranges;
    (void) begin(vri);
    (void) end(vri);
    (void) cbegin(vri);
    (void) cend(vri);
    (void) rbegin(vri);
    (void) rend(vri);
    (void) crbegin(vri);
    (void) crend(vri);
}

TEST(RangeAccess, initializer_list) {
    std::initializer_list<int> il = {0, 1, 2};
    {
        int count = 0;
        for (auto p = ranges::begin(il), e = ranges::end(il); p != e; ++p) {
            EXPECT_EQ(*p, count++);
        }
    }
    {
        int count = 3;
        for (auto p = ranges::rbegin(il), e = ranges::rend(il); p != e; ++p) {
            EXPECT_EQ(*p, --count);
        }
    }
    EXPECT_EQ(ranges::size(il), std::size_t{3});
    EXPECT_EQ(ranges::data(il), &*il.begin());
    EXPECT_FALSE(ranges::empty(il));
}

template<class Value, typename T, T... Is>
void test_array_impl(std::integer_sequence<T, Is...>) {
    Value a[sizeof...(Is)] = {Is...};
    {
        int count = 0;
        for (auto p = ranges::begin(a), e = ranges::end(a); p != e; ++p) {
            EXPECT_EQ(*p, count++);
        }
    }
    {
        int count = sizeof...(Is);
        for (auto p = ranges::rbegin(a), e = ranges::rend(a); p != e; ++p) {
            EXPECT_EQ(*p, --count);
        }
    }
    EXPECT_EQ(ranges::size(a), sizeof...(Is));
    EXPECT_EQ(ranges::data(a), a + 0);
    EXPECT_FALSE(ranges::empty(a));
}

TEST(RangeAccess, array) {
    test_array_impl<int>(std::make_integer_sequence<int, 3>{});
    test_array_impl<int const>(std::make_integer_sequence<int, 3>{});
}

namespace begin_testing {
    // Helper concepts (compile-time checks only)
    template<class R>
    CPP_requires(can_begin_,
                 requires(R&& r) //
                 (
                     ranges::begin((R &&) r)
                 ));

    template<class R>
    CPP_concept can_begin =
            CPP_requires_ref(can_begin_, R);

    template<class R>
    CPP_requires(can_cbegin_,
                 requires(R&& r) //
                 (
                     ranges::cbegin((R &&) r)
                 ));

    template<class R>
    CPP_concept can_cbegin =
            CPP_requires_ref(can_cbegin_, R);

    struct A {
        int *begin();

        int *end();

        int const *begin() const;

        int const *end() const;
    };

    struct B : A {
    };

    void *begin(B &);

    struct C : A {
    };

    void begin(C &);

    struct D : A {
    };

    char *begin(D &);
} // namespace begin_testing

TEST(RangeAccess, begin_end_overloads) {
    using namespace begin_testing;

    // Compile-time checks for can_begin/can_cbegin (we use static_assert)
    static_assert(can_begin<int(&)[2]>, "int(&)[2] should have begin");
    static_assert(std::is_same_v<decltype(ranges::begin(std::declval<int(&)[2]>())), int *>, "begin returns int*");
    static_assert(can_begin<int const(&)[2]>, "int const(&)[2] should have begin");
    static_assert(std::is_same_v<decltype(ranges::begin(std::declval<int const(&)[2]>())), int const *>,
                  "begin returns int const*");

    static_assert(can_cbegin<int(&)[2]>, "int(&)[2] should have cbegin");
    static_assert(std::is_same_v<decltype(ranges::cbegin(std::declval<int(&)[2]>())), int const *>,
                  "cbegin returns int const*");
    static_assert(can_cbegin<int const(&)[2]>, "int const(&)[2] should have cbegin");
    static_assert(std::is_same_v<decltype(ranges::cbegin(std::declval<int const(&)[2]>())), int const *>,
                  "cbegin returns int const*");

#ifndef RANGES_WORKAROUND_MSVC_573728
    static_assert(!can_begin<int(&&)[2]>, "array rvalue should not have begin");
    static_assert(!can_begin<int const(&&)[2]>, "array rvalue should not have begin");
    static_assert(!can_cbegin<int(&&)[2]>, "array rvalue should not have cbegin");
    static_assert(!can_cbegin<int const(&&)[2]>, "array rvalue should not have cbegin");
#endif

    static_assert(can_begin<A &> && !can_begin<A>, "A& should have begin, A should not");
    static_assert(std::is_same_v<decltype(ranges::begin(std::declval<A &>())), int *>, "begin returns int*");
    static_assert(can_begin<const A &> && !can_begin<const A>, "const A& should have begin, const A should not");
    static_assert(std::is_same_v<decltype(ranges::begin(std::declval<const A &>())), int const *>,
                  "begin returns int const*");

    static_assert(can_begin<B &> && !can_begin<B>, "B& should have begin, B should not");
    static_assert(std::is_same_v<decltype(ranges::begin(std::declval<B &>())), int *>, "begin returns int*");
    static_assert(can_begin<const B &> && !can_begin<const B>, "const B& should have begin, const B should not");
    static_assert(std::is_same_v<decltype(ranges::begin(std::declval<const B &>())), int const *>,
                  "begin returns int const*");

    static_assert(can_begin<C &> && !can_begin<C>, "C& should have begin, C should not");
    static_assert(can_begin<const C &> && !can_begin<const C>, "const C& should have begin, const C should not");

    static_assert(can_begin<D &> && !can_begin<D>, "D& should have begin, D should not");
    static_assert(std::is_same_v<int *, decltype(ranges::begin(std::declval<D &>()))>, "begin returns int*");
    static_assert(can_begin<const D &> && !can_begin<const D>, "const D& should have begin, const D should not");
    static_assert(std::is_same_v<int const *, decltype(ranges::begin(std::declval<const D &>()))>,
                  "begin returns int const*");

    {
        using T = std::initializer_list<int>;
        static_assert(std::is_same_v<int const *, decltype(ranges::begin(std::declval<T &>()))>,
                      "begin returns int const*");
        static_assert(std::is_same_v<int const *, decltype(ranges::begin(std::declval<const T &>()))>,
                      "begin returns int const*");
        static_assert(!can_begin<T>, "T should not have begin");
        static_assert(!can_begin<T const>, "T const should not have begin");
    }

    static_assert(can_begin<ranges::subrange<int *, int *> &>, "subrange& should have begin");
    static_assert(can_begin<const ranges::subrange<int *, int *> &>, "const subrange& should have begin");
    static_assert(can_begin<ranges::subrange<int *, int *> >, "subrange should have begin");
    static_assert(can_begin<const ranges::subrange<int *, int *>>, "const subrange should have begin");

    static_assert(can_cbegin<ranges::subrange<int *, int *> &>, "subrange& should have cbegin");
    static_assert(can_cbegin<const ranges::subrange<int *, int *> &>, "const subrange& should have cbegin");
    static_assert(can_cbegin<ranges::subrange<int *, int *> >, "subrange should have cbegin");
    static_assert(can_cbegin<const ranges::subrange<int *, int *>>, "const subrange should have cbegin");

    static_assert(can_begin<ranges::ref_view<int[5]> &>, "ref_view& should have begin");
    static_assert(can_begin<const ranges::ref_view<int[5]> &>, "const ref_view& should have begin");
    static_assert(can_begin<ranges::ref_view<int[5]> >, "ref_view should have begin");
    static_assert(can_begin<const ranges::ref_view<int[5]>>, "const ref_view should have begin");

    static_assert(can_cbegin<ranges::ref_view<int[5]> &>, "ref_view& should have cbegin");
    static_assert(can_cbegin<const ranges::ref_view<int[5]> &>, "const ref_view& should have cbegin");
    static_assert(can_cbegin<ranges::ref_view<int[5]> >, "ref_view should have cbegin");
    static_assert(can_cbegin<const ranges::ref_view<int[5]>>, "const ref_view should have cbegin");
}

TEST(RangeAccess, custom_array) {
    using namespace ranges;
    static constexpr X::array<int, 4> some_ints = {{0, 1, 2, 3}};
    static_assert(begin_testing::can_begin<X::array<int, 4> &>, "array& should have begin");
    static_assert(begin_testing::can_begin<X::array<int, 4> const &>, "const array& should have begin");
    static_assert(!begin_testing::can_begin<X::array<int, 4> >, "array rvalue should not have begin");
    static_assert(!begin_testing::can_begin<X::array<int, 4> const>, "const array rvalue should not have begin");
    static_assert(begin_testing::can_cbegin<X::array<int, 4> &>, "array& should have cbegin");
    static_assert(begin_testing::can_cbegin<X::array<int, 4> const &>, "const array& should have cbegin");
    static_assert(!begin_testing::can_cbegin<X::array<int, 4> >, "array rvalue should not have cbegin");
    static_assert(!begin_testing::can_cbegin<X::array<int, 4> const>, "const array rvalue should not have cbegin");

    constexpr auto first = begin(some_ints);
    constexpr auto last = end(some_ints);
    static_assert(std::is_same_v<const CI, decltype(first)>, "first is const int*");
    static_assert(std::is_same_v<const CI, decltype(last)>, "last is const int*");
    static_assert(first == cbegin(some_ints), "first equals cbegin");
    static_assert(last == cend(some_ints), "last equals cend");

    static_assert(noexcept(begin(some_ints)), "begin is noexcept");
    static_assert(noexcept(end(some_ints)), "end is noexcept");
    static_assert(noexcept(cbegin(some_ints)), "cbegin is noexcept");
    static_assert(noexcept(cend(some_ints)), "cend is noexcept");
    static_assert(noexcept(empty(some_ints)), "empty is noexcept");
    static_assert(noexcept(data(some_ints)), "data is noexcept");

    static_assert(!empty(some_ints), "array is not empty");
    int count = 0;
    for (auto &&i: some_ints) {
        EXPECT_EQ(i, count++);
    }
}

#if defined(__cpp_lib_string_view) && __cpp_lib_string_view >= 201603L
TEST(RangeAccess, string_view_p0970) {
    // basic_string_views are non-dangling
    using I2 = ranges::iterator_t<std::string_view>;
    static_assert(std::is_same_v<I2, decltype(ranges::begin(std::declval<std::string_view>()))>,
                  "begin returns iterator");
    static_assert(std::is_same_v<I2, decltype(ranges::end(std::declval<std::string_view>()))>, "end returns iterator");
    static_assert(std::is_same_v<I2, decltype(ranges::begin(std::declval<const std::string_view>()))>,
                  "begin const returns iterator");
    static_assert(std::is_same_v<I2, decltype(ranges::end(std::declval<const std::string_view>()))>,
                  "end const returns iterator");

    const char hw[] = "Hello, World!";
    auto result = ranges::find(std::string_view{hw}, 'W');
    static_assert(std::is_same_v<I2, decltype(result)>, "find returns iterator");
    EXPECT_EQ(result, std::string_view{hw}.begin() + 7);
}
#endif // __cpp_lib_string_view
