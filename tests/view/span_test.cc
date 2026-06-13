/// span_gtest.cpp
/// Google Test conversion of range-v3 span test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <fermat/view/span.h>               /// fermat::ranges::span, fermat::ranges::dynamic_extent, fermat::ranges::make_span
#include <fermat/range/access.h>            /// fermat::ranges::data, fermat::ranges::size, fermat::ranges::begin, fermat::ranges::end
#include <fermat/range/concepts.h>          /// fermat::ranges::view_, fermat::ranges::contiguous_range

using fermat::ranges::span;
using fermat::ranges::dynamic_extent;
using fermat::ranges::make_span;
using fermat::ranges::as_bytes;
using fermat::ranges::as_writeable_bytes;
// Use the narrow_cast provided by fermat (in spans.h) - do not define our own.

namespace {
    struct BaseClass {};
    struct DerivedClass : BaseClass {};
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

TEST(SpanTest, DefaultConstructor) {
    {
        span<int> s;
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);

        span<const int> cs;
        EXPECT_EQ(cs.size(), 0u);
        EXPECT_EQ(cs.data(), nullptr);
    }

    {
        span<int, 0> s;
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);

        span<const int, 0> cs;
        EXPECT_EQ(cs.size(), 0u);
        EXPECT_EQ(cs.data(), nullptr);
    }

    {
        span<int, 1> s;
        EXPECT_EQ(s.size(), 1u);
        EXPECT_EQ(s.data(), nullptr);
    }

    {
        span<int> s{};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);

        span<const int> cs{};
        EXPECT_EQ(cs.size(), 0u);
        EXPECT_EQ(cs.data(), nullptr);
    }
}

TEST(SpanTest, SizeOptimization) {
    {
        span<int> s;
        EXPECT_EQ(sizeof(s), sizeof(int*) + sizeof(std::ptrdiff_t));
    }
    {
        span<int, 0> s;
        EXPECT_EQ(sizeof(s), sizeof(int*));
    }
}

TEST(SpanTest, FromNullptrConstructor) {
    static_assert(!std::is_constructible<span<int>, std::nullptr_t>::value, "");
    static_assert(!std::is_constructible<span<const int>, std::nullptr_t>::value, "");
    static_assert(!std::is_constructible<span<int, 0>, std::nullptr_t>::value, "");
    static_assert(!std::is_constructible<span<int, 1>, std::nullptr_t>::value, "");
}

TEST(SpanTest, FromNullptrSizeConstructor) {
    {
        span<int> s{nullptr, 0};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);

        span<const int> cs{nullptr, 0};
        EXPECT_EQ(cs.size(), 0u);
        EXPECT_EQ(cs.data(), nullptr);
    }
    {
        span<int, 0> s{nullptr, 0};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
    {
        span<int*> s{nullptr, 0};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
}

TEST(SpanTest, FromPointerSizeConstructor) {
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s{&arr[0], 2};
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        span<int, 2> s{&arr[0], 2};
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        int* p = nullptr;
        span<int> s{p, 0};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
    {
        auto s = make_span(&arr[0], 2);
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        int* p = nullptr;
        auto s = make_span(p, 0);
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
    {
        int i = 42;
        span<int> s{&i, 0};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), &i);
    }
}

TEST(SpanTest, FromPointerPointerConstructor) {
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s{&arr[0], &arr[2]};
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        span<int, 2> s{&arr[0], &arr[2]};
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        span<int> s{&arr[0], &arr[0]};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), &arr[0]);
    }
    {
        span<int, 0> s{&arr[0], &arr[0]};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), &arr[0]);
    }
    {
        int* p = nullptr;
        span<int> s{p, p};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
    {
        int* p = nullptr;
        span<int, 0> s{p, p};
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
    {
        auto s = make_span(&arr[0], &arr[2]);
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        auto s = make_span(&arr[0], &arr[0]);
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), &arr[0]);
    }
    {
        int* p = nullptr;
        auto s = make_span(p, p);
        EXPECT_EQ(s.size(), 0u);
        EXPECT_EQ(s.data(), nullptr);
    }
}

TEST(SpanTest, FromArrayConstructor) {
    int arr[5] = {1, 2, 3, 4, 5};
    {
        span<int> s{arr};
        EXPECT_EQ(s.size(), 5u);
        EXPECT_EQ(s.data(), &arr[0]);
    }
    {
        span<int, 5> s{arr};
        EXPECT_EQ(s.size(), 5u);
        EXPECT_EQ(s.data(), &arr[0]);
    }

    int arr2d[2][3] = {1, 2, 3, 4, 5, 6};
    static_assert(!std::is_constructible<span<int, 6>, int(&)[5]>::value, "");
    static_assert(!std::is_constructible<span<int, 0>, int(&)[5]>::value, "");
    static_assert(!std::is_constructible<span<int>, decltype((arr2d))>::value, "");
    static_assert(!std::is_constructible<span<int, 0>, decltype((arr2d))>::value, "");
    static_assert(!std::is_constructible<span<int, 6>, decltype((arr2d))>::value, "");

    {
        span<int[3]> s{&arr2d[0], 1};
        EXPECT_EQ(s.size(), 1u);
        EXPECT_EQ(s.data(), &arr2d[0]);
    }

    int arr3d[2][3][2] = {1,2,3,4,5,6,7,8,9,10,11,12};
    static_assert(!std::is_constructible<span<int>, decltype((arr3d))>::value, "");
    static_assert(!std::is_constructible<span<int, 0>, decltype((arr3d))>::value, "");
    static_assert(!std::is_constructible<span<int, 11>, decltype((arr3d))>::value, "");
    static_assert(!std::is_constructible<span<int, 12>, decltype((arr3d))>::value, "");

    {
        span<int[3][2]> s{&arr3d[0], 1};
        EXPECT_EQ(s.size(), 1u);
        EXPECT_EQ(s.data(), &arr3d[0]);
    }

    {
        auto s = make_span(arr);
        EXPECT_EQ(s.size(), 5u);
        EXPECT_EQ(s.data(), &arr[0]);
    }
    {
        auto s = make_span(&arr2d[0], 1);
        EXPECT_EQ(s.size(), 1u);
        EXPECT_EQ(s.data(), &arr2d[0]);
    }
    {
        auto s = make_span(&arr3d[0], 1);
        EXPECT_EQ(s.size(), 1u);
        EXPECT_EQ(s.data(), &arr3d[0]);
    }
}

TEST(SpanTest, FromStdArrayConstructor) {
    std::array<int, 4> arr = {1, 2, 3, 4};

    {
        span<int> s{arr};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
    {
        span<int, 4> s{arr};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
    static_assert(!std::is_constructible<span<int, 2>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<int, 0>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<int, 5>, decltype(arr)>::value, "");

    {
        auto get_an_array = []() -> std::array<int, 4> { return {1,2,3,4}; };
        auto take_a_span = [](span<int>) {};
        take_a_span(get_an_array());
    }
    {
        auto get_an_array = []() -> std::array<int, 4> { return {1,2,3,4}; };
        auto take_a_span = [](span<const int>) {};
        take_a_span(get_an_array());
    }
    {
        auto s = make_span(arr);
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
}

TEST(SpanTest, FromConstStdArrayConstructor) {
    const std::array<int, 4> arr = {1, 2, 3, 4};

    {
        span<const int> s{arr};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
    {
        span<const int, 4> s{arr};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
    static_assert(!std::is_constructible<span<const int, 2>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<const int, 0>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<const int, 5>, decltype(arr)>::value, "");

    {
        auto get_an_array = []() -> const std::array<int, 4> { return {1,2,3,4}; };
        auto take_a_span = [](span<const int>) {};
        take_a_span(get_an_array());
    }
    {
        auto s = make_span(arr);
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
}

TEST(SpanTest, FromStdArrayConstConstructor) {
    std::array<const int, 4> arr = {1, 2, 3, 4};

    {
        span<const int> s{arr};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
    {
        span<const int, 4> s{arr};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
    static_assert(!std::is_constructible<span<const int, 2>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<const int, 0>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<const int, 5>, decltype(arr)>::value, "");
    static_assert(!std::is_constructible<span<int, 4>, decltype(arr)>::value, "");

    {
        auto s = make_span(arr);
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(arr.size()));
        EXPECT_EQ(s.data(), arr.data());
    }
}

TEST(SpanTest, FromContainerConstructor) {
    std::vector<int> v = {1, 2, 3};
    const std::vector<int> cv = v;

    {
        span<int> s{v};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(v.size()));
        EXPECT_EQ(s.data(), v.data());
    }
    {
        span<const int> cs{v};
        EXPECT_EQ(cs.size(), static_cast<std::ptrdiff_t>(v.size()));
        EXPECT_EQ(cs.data(), v.data());
    }

    std::string str = "hello";
    const std::string cstr = "hello";

    {
        span<char> s{str};
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(str.size()));
        EXPECT_EQ(s.data(), str.data());
    }
    {
        auto get_temp_string = []() -> std::string { return {}; };
        auto use_span = [](span<char>) {};
        use_span(get_temp_string());
    }
    {
        span<const char> cs{str};
        EXPECT_EQ(cs.size(), static_cast<std::ptrdiff_t>(str.size()));
        EXPECT_EQ(cs.data(), str.data());
    }
    {
        auto get_temp_string = []() -> std::string { return {}; };
        auto use_span = [](span<const char>) {};
        use_span(get_temp_string());
    }
    {
        static_assert(!std::is_constructible<span<char>, decltype(cstr)>::value, "");
        span<const char> cs{cstr};
        EXPECT_EQ(cs.size(), static_cast<std::ptrdiff_t>(cstr.size()));
        EXPECT_EQ(cs.data(), cstr.data());
    }
    {
        auto get_temp_vector = []() -> std::vector<int> { return {}; };
        auto use_span = [](span<int>) {};
        use_span(get_temp_vector());
    }
    {
        auto get_temp_vector = []() -> std::vector<int> { return {}; };
        auto use_span = [](span<const int>) {};
        use_span(get_temp_vector());
    }
    static_assert(!fermat::ranges::detail::is_convertible<const std::vector<int>, span<const char>>::value, "");
    {
        auto get_temp_string = []() -> const std::string { return {}; };
        auto use_span = [](span<const char> s) { static_cast<void>(s); };
        use_span(get_temp_string());
    }
    static_assert(!std::is_constructible<span<int>, std::map<int, int>&>::value, "");

    {
        auto s = make_span(v);
        EXPECT_EQ(s.size(), static_cast<std::ptrdiff_t>(v.size()));
        EXPECT_EQ(s.data(), v.data());
        auto cs = make_span(cv);
        EXPECT_EQ(cs.size(), static_cast<std::ptrdiff_t>(cv.size()));
        EXPECT_EQ(cs.data(), cv.data());
    }
}

TEST(SpanTest, FromConvertibleSpanConstructor) {
    {
        span<DerivedClass> avd;
        span<const DerivedClass> avcd = avd;
        (void)avcd;
    }
    static_assert(!std::is_constructible<span<BaseClass>, span<DerivedClass>>::value, "");
    static_assert(!std::is_constructible<span<DerivedClass>, span<BaseClass>>::value, "");
    static_assert(!std::is_constructible<span<unsigned int>, span<int>>::value, "");
}

TEST(SpanTest, CopyMoveAndAssignment) {
    span<int> s1;
    EXPECT_TRUE(s1.empty());

    int arr[] = {3, 4, 5};
    span<const int> s2 = arr;
    EXPECT_EQ(s2.size(), 3u);
    EXPECT_EQ(s2.data(), &arr[0]);

    s2 = s1;
    EXPECT_TRUE(s2.empty());

    auto get_temp_span = [&]() -> span<int> { return {&arr[1], 2}; };
    auto use_span = [&](span<const int> s) {
        EXPECT_EQ(s.size(), 2u);
        EXPECT_EQ(s.data(), &arr[1]);
    };
    use_span(get_temp_span());

    s1 = get_temp_span();
    EXPECT_EQ(s1.size(), 2u);
    EXPECT_EQ(s1.data(), &arr[1]);
}

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
TEST(SpanTest, ClassTemplateArgumentDeduction) {
    int arr[] = {1,2,3,4,5};
    {
        span s{arr};
        static_assert(std::is_same<span<int,5>, decltype(s)>::value, "");
    }
    {
        span s{fermat::ranges::data(arr), fermat::ranges::size(arr)};
        static_assert(std::is_same<span<int>, decltype(s)>::value, "");
    }
    {
        span s{fermat::ranges::begin(arr), fermat::ranges::end(arr)};
        static_assert(std::is_same<span<int>, decltype(s)>::value, "");
    }
    std::array<int,5> std_arr = {1,2,3,4,5};
    {
        span s{std_arr};
        static_assert(std::is_same<span<int,5>, decltype(s)>::value, "");
    }
    std::vector<int> vec = {1,2,3,4,5};
    {
        span s{vec};
        static_assert(std::is_same<span<int>, decltype(s)>::value, "");
    }
}
#endif // deduction guides

TEST(SpanTest, First) {
    int arr[5] = {1,2,3,4,5};
    {
        span<int,5> av = arr;
        auto s1 = av.first<2>();
        EXPECT_EQ(s1.size(), 2u);
        auto s2 = av.first(2);
        EXPECT_EQ(s2.size(), 2u);
        auto s3 = av.first<0>();
        EXPECT_EQ(s3.size(), 0u);
        auto s4 = av.first(0);
        EXPECT_EQ(s4.size(), 0u);
        auto s5 = av.first<5>();
        EXPECT_EQ(s5.size(), 5u);
        auto s6 = av.first(5);
        EXPECT_EQ(s6.size(), 5u);
    }
    {
        span<int> av;
        auto s1 = av.first<0>();
        EXPECT_EQ(s1.size(), 0u);
        auto s2 = av.first(0);
        EXPECT_EQ(s2.size(), 0u);
    }
}

TEST(SpanTest, Last) {
    int arr[5] = {1,2,3,4,5};
    {
        span<int,5> av = arr;
        auto s1 = av.last<2>();
        EXPECT_EQ(s1.size(), 2u);
        auto s2 = av.last(2);
        EXPECT_EQ(s2.size(), 2u);
        auto s3 = av.last<0>();
        EXPECT_EQ(s3.size(), 0u);
        auto s4 = av.last(0);
        EXPECT_EQ(s4.size(), 0u);
        auto s5 = av.last<5>();
        EXPECT_EQ(s5.size(), 5u);
        auto s6 = av.last(5);
        EXPECT_EQ(s6.size(), 5u);
    }
    {
        span<int> av;
        auto s1 = av.last<0>();
        EXPECT_EQ(s1.size(), 0u);
        auto s2 = av.last(0);
        EXPECT_EQ(s2.size(), 0u);
    }
}

TEST(SpanTest, Subspan) {
    int arr[5] = {1,2,3,4,5};
    {
        span<int,5> av = arr;
        auto s1 = av.subspan<2,2>();
        EXPECT_EQ(s1.size(), 2u);
        auto s2 = av.subspan(2,2);
        EXPECT_EQ(s2.size(), 2u);
        auto s3 = av.subspan(2,3);
        EXPECT_EQ(s3.size(), 3u);
        auto s4 = av.subspan<0,0>();
        EXPECT_EQ(s4.size(), 0u);
        auto s5 = av.subspan(0,0);
        EXPECT_EQ(s5.size(), 0u);
        auto s6 = av.subspan<0,5>();
        EXPECT_EQ(s6.size(), 5u);
        auto s7 = av.subspan(0,5);
        EXPECT_EQ(s7.size(), 5u);
        auto s8 = av.subspan<4,0>();
        EXPECT_EQ(s8.size(), 0u);
        auto s9 = av.subspan(4,0);
        EXPECT_EQ(s9.size(), 0u);
        auto s10 = av.subspan(5,0);
        EXPECT_EQ(s10.size(), 0u);
    }
    {
        span<int> av;
        auto s1 = av.subspan<0,0>();
        EXPECT_EQ(s1.size(), 0u);
        auto s2 = av.subspan(0,0);
        EXPECT_EQ(s2.size(), 0u);
        auto s3 = av.subspan(0);
        EXPECT_EQ(s3.size(), 0u);
    }
    {
        span<int> av = arr;
        auto s1 = av.subspan(0);
        EXPECT_EQ(s1.size(), 5u);
        auto s2 = av.subspan(1);
        EXPECT_EQ(s2.size(), 4u);
        auto s3 = av.subspan(4);
        EXPECT_EQ(s3.size(), 1u);
        auto s4 = av.subspan(5);
        EXPECT_EQ(s4.size(), 0u);
        const auto av2 = av.subspan(1);
        for (int i = 0; i < 4; ++i) EXPECT_EQ(av2[i], i+2);
    }
    {
        span<int,5> av = arr;
        auto s1 = av.subspan(0);
        EXPECT_EQ(s1.size(), 5u);
        auto s2 = av.subspan(1);
        EXPECT_EQ(s2.size(), 4u);
        auto s3 = av.subspan(4);
        EXPECT_EQ(s3.size(), 1u);
        auto s4 = av.subspan(5);
        EXPECT_EQ(s4.size(), 0u);
        const auto av2 = av.subspan(1);
        for (int i = 0; i < 4; ++i) EXPECT_EQ(av2[i], i+2);
    }
    {
        span<int,5> av = arr;
        auto s1 = av.subspan<0, dynamic_extent>();
        static_assert(std::is_same<decltype(s1), span<int,5>>::value, "");
        EXPECT_EQ(s1.size(), 5u);
        auto s2 = av.subspan<1, dynamic_extent>();
        static_assert(std::is_same<decltype(s2), span<int,4>>::value, "");
        EXPECT_EQ(s2.size(), 4u);
    }
    {
        span<int> av = arr;
        auto s1 = av.subspan<0, dynamic_extent>();
        static_assert(std::is_same<decltype(s1), span<int>>::value, "");
        EXPECT_EQ(s1.size(), 5u);
    }
}

TEST(SpanTest, IteratorValueInit) {
    span<int>::iterator it1{};
    span<int>::iterator it2{};
    EXPECT_EQ(it1, it2);
}

TEST(SpanTest, IteratorComparisons) {
    int a[] = {1,2,3,4};
    span<int> s = a;
    auto it = s.begin();
    auto it2 = it + 1;
    EXPECT_EQ(it, it);
    EXPECT_NE(it, it2);
    EXPECT_NE(it, s.end());
    EXPECT_NE(it2, s.end());
    EXPECT_LT(it, it2);
    EXPECT_LE(it, it2);
    EXPECT_LE(it2, s.end());
    EXPECT_LT(it, s.end());
    EXPECT_GT(it2, it);
    EXPECT_GE(it2, it);
    EXPECT_GT(s.end(), it2);
    EXPECT_GE(s.end(), it2);
}

TEST(SpanTest, BeginEnd) {
    int a[] = {1,2,3,4};
    span<int> s = a;
    auto it = s.begin();
    auto first = it;
    EXPECT_EQ(it, first);
    EXPECT_EQ(*it, 1);
    auto beyond = s.end();
    EXPECT_NE(it, beyond);
    EXPECT_EQ(beyond - first, 4);
    EXPECT_EQ(first - first, 0);
    EXPECT_EQ(beyond - beyond, 0);
    ++it;
    EXPECT_EQ(it - first, 1);
    EXPECT_EQ(*it, 2);
    *it = 22;
    EXPECT_EQ(*it, 22);
    EXPECT_EQ(beyond - it, 3);
    it = first;
    while (it != s.end()) {
        *it = 5;
        ++it;
    }
    EXPECT_EQ(it, beyond);
    EXPECT_EQ(it - beyond, 0);
    for (const auto& n : s) {
        EXPECT_EQ(n, 5);
    }
}

TEST(SpanTest, RbeginRend) {
    int a[] = {1,2,3,4};
    span<int> s = a;
    auto it = s.rbegin();
    auto first = it;
    EXPECT_EQ(it, first);
    EXPECT_EQ(*it, 4);
    auto beyond = s.rend();
    EXPECT_NE(it, beyond);
    EXPECT_EQ(beyond - first, 4);
    EXPECT_EQ(first - first, 0);
    EXPECT_EQ(beyond - beyond, 0);
    ++it;
    EXPECT_EQ(it - first, 1);
    EXPECT_EQ(*it, 3);
    *it = 22;
    EXPECT_EQ(*it, 22);
    EXPECT_EQ(beyond - it, 3);
    it = first;
    while (it != s.rend()) {
        *it = 5;
        ++it;
    }
    EXPECT_EQ(it, beyond);
    for (const auto& n : s) {
        EXPECT_EQ(n, 5);
    }
}

TEST(SpanTest, ComparisonOperators) {
    {
        span<int> s1, s2;
        EXPECT_EQ(s1, s2);
        EXPECT_FALSE(s1 != s2);
        EXPECT_FALSE(s1 < s2);
        EXPECT_LE(s1, s2);
        EXPECT_FALSE(s1 > s2);
        EXPECT_GE(s1, s2);
    }
    {
        int arr[] = {2,1};
        span<int> s1 = arr;
        span<int> s2 = arr;
        EXPECT_EQ(s1, s2);
        EXPECT_FALSE(s1 != s2);
        EXPECT_FALSE(s1 < s2);
        EXPECT_LE(s1, s2);
        EXPECT_FALSE(s1 > s2);
        EXPECT_GE(s1, s2);
    }
    {
        int arr[] = {2,1};
        span<int> s1;
        span<int> s2 = arr;
        EXPECT_NE(s1, s2);
        EXPECT_LT(s1, s2);
        EXPECT_LE(s1, s2);
        EXPECT_GT(s2, s1);
        EXPECT_GE(s2, s1);
    }
    {
        int arr1[] = {1,2};
        int arr2[] = {1,2};
        span<int> s1 = arr1;
        span<int> s2 = arr2;
        EXPECT_EQ(s1, s2);
        EXPECT_FALSE(s1 < s2);
    }
    {
        int arr[] = {1,2,3};
        span<int> s1 = {&arr[0], 2};
        span<int> s2 = arr;
        EXPECT_NE(s1, s2);
        EXPECT_LT(s1, s2);
        EXPECT_LE(s1, s2);
        EXPECT_GT(s2, s1);
        EXPECT_GE(s2, s1);
    }
    {
        int arr1[] = {1,2};
        int arr2[] = {2,1};
        span<int> s1 = arr1;
        span<int> s2 = arr2;
        EXPECT_NE(s1, s2);
        EXPECT_LT(s1, s2);
        EXPECT_LE(s1, s2);
        EXPECT_GT(s2, s1);
        EXPECT_GE(s2, s1);
    }
}

TEST(SpanTest, AsBytes) {
    int a[] = {1,2,3,4};
    {
        const span<const int> s = a;
        const auto bs = as_bytes(s);
        EXPECT_EQ(static_cast<const void*>(bs.data()), static_cast<const void*>(s.data()));
        EXPECT_EQ(bs.size(), s.size_bytes());
    }
    {
        span<int> s;
        const auto bs = as_bytes(s);
        EXPECT_EQ(bs.size(), 0u);
        EXPECT_EQ(bs.size_bytes(), 0u);
        EXPECT_EQ(bs.data(), nullptr);
    }
    {
        span<int> s = a;
        const auto bs = as_bytes(s);
        EXPECT_EQ(static_cast<const void*>(bs.data()), static_cast<const void*>(s.data()));
        EXPECT_EQ(bs.size(), s.size_bytes());
    }
}

TEST(SpanTest, AsWriteableBytes) {
    int a[] = {1,2,3,4};
    {
        span<int> s;
        const auto bs = as_writeable_bytes(s);
        EXPECT_EQ(bs.size(), 0u);
        EXPECT_EQ(bs.size_bytes(), 0u);
        EXPECT_EQ(bs.data(), nullptr);
    }
    {
        span<int> s = a;
        const auto bs = as_writeable_bytes(s);
        EXPECT_EQ(static_cast<void*>(bs.data()), static_cast<void*>(s.data()));
        EXPECT_EQ(bs.size(), s.size_bytes());
    }
}

TEST(SpanTest, FixedSizeConversions) {
    int arr[] = {1,2,3,4};
    span<int,4> s4 = arr;
    EXPECT_EQ(s4.size(), 4u);
    {
        span<int> s = s4;
        EXPECT_EQ(s.size(), s4.size());
    }
    static_assert(!fermat::ranges::detail::is_convertible<decltype(arr), span<int,2>>::value, "");
    static_assert(!fermat::ranges::detail::is_convertible<span<int,4>, span<int,2>>::value, "");
    {
        const span<int,2> s2 = {arr, 2};
        (void)s2;
    }
    {
        const span<int,1> s1 = s4.first<1>();
        (void)s1;
    }
    {
        span<int,1> s1 = s4.first(1);
        (void)s1;
    }
    int arr2[2] = {1,2};
    static_assert(!std::is_constructible<span<int,4>, decltype(arr2)>::value, "");
    static_assert(!std::is_constructible<span<int,4>, span<int,2>>::value, "");
}

TEST(SpanTest, InteropWithStdRegex) {
    char lat[] = {'1','2','3','4','5','6','E','F','G'};
    span<char> s = lat;
    const auto f_it = s.begin() + 7;
    std::match_results<span<char>::iterator> match;
    std::regex_match(s.begin(), s.end(), match, std::regex(".*"));
    EXPECT_TRUE(match.ready());
    EXPECT_FALSE(match.empty());
    EXPECT_TRUE(match[0].matched);
    EXPECT_EQ(match[0].first, s.begin());
    EXPECT_EQ(match[0].second, s.end());
    std::regex_search(s.begin(), s.end(), match, std::regex("F"));
    EXPECT_TRUE(match.ready());
    EXPECT_FALSE(match.empty());
    EXPECT_TRUE(match[0].matched);
    EXPECT_EQ(match[0].first, f_it);
    EXPECT_EQ(match[0].second, f_it+1);
}

TEST(SpanTest, DefaultConstructible) {
    static_assert(std::is_default_constructible<span<int>>::value, "");
    static_assert(std::is_default_constructible<span<int,0>>::value, "");
    static_assert(std::is_default_constructible<span<int,42>>::value, "");
}

// Compile-time concept checks
static_assert(fermat::ranges::view_<span<int>>, "");
static_assert(fermat::ranges::contiguous_range<span<int>>, "");
static_assert(fermat::ranges::view_<span<int, 42>>, "");
static_assert(fermat::ranges::contiguous_range<span<int, 42>>, "");
