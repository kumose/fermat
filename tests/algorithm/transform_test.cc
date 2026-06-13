// transform_test.cc
// Google Test conversion of range-v3 transform algorithm test.
// Uses raw pointers and lambda to avoid overload ambiguity.
// All comments in English.

#include <gtest/gtest.h>
#include <functional>
#include <vector>

#include <fermat/algorithm/transform.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/all.h>

// ------------------------------------------------------------
// Helper: check if two ranges are equal (for test_constexpr)
// ------------------------------------------------------------
template<typename It1, typename It2>
bool check_equal(It1 first1, It1 last1, It2 first2) {
    while (first1 != last1) {
        if (*first1 != *first2) return false;
        ++first1;
        ++first2;
    }
    return true;
}

// ------------------------------------------------------------
// Test functions (using raw pointers, no custom iterators)
// ------------------------------------------------------------

void test_unary() {
    // Use lambda instead of std::bind to avoid overload ambiguity
    auto plus_one = [](int x) { return x + 1; };

    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {0};
        auto res = ranges::transform(ia, ia + sa, ib, plus_one);
        EXPECT_EQ(res.in, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        EXPECT_EQ(ib[0], 1);
        EXPECT_EQ(ib[1], 2);
        EXPECT_EQ(ib[2], 3);
        EXPECT_EQ(ib[3], 4);
        EXPECT_EQ(ib[4], 5);
    }
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {0};
        auto rng = ranges::make_subrange(ia, ia + sa);
        auto res = ranges::transform(rng, ib, plus_one);
        EXPECT_EQ(res.in, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        EXPECT_EQ(ib[0], 1);
        EXPECT_EQ(ib[1], 2);
        EXPECT_EQ(ib[2], 3);
        EXPECT_EQ(ib[3], 4);
        EXPECT_EQ(ib[4], 5);
    }
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {0};
        auto rng = ranges::make_subrange(ia, ia + sa);
        auto res = ranges::transform(std::move(rng), ib, plus_one);
        EXPECT_EQ(res.in, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        EXPECT_EQ(ib[0], 1);
        EXPECT_EQ(ib[1], 2);
        EXPECT_EQ(ib[2], 3);
        EXPECT_EQ(ib[3], 4);
        EXPECT_EQ(ib[4], 5);
    }
}

void test_binary() {
    // Use the recommended overload with two sentinels
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {1, 2, 3, 4, 5};
        auto res = ranges::transform(ib, ib + sa, ia, ia + sa, ib, std::minus<int>());
        EXPECT_EQ(res.in1, ib + sa);
        EXPECT_EQ(res.in2, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        for (int i = 0; i < 5; ++i) EXPECT_EQ(ib[i], 1);
    }
    // Range + single iterator (still valid)
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {1, 2, 3, 4, 5};
        auto rng0 = ranges::make_subrange(ib, ib + sa);
        auto res = ranges::transform(rng0, ia, ib, std::minus<int>());
        EXPECT_EQ(res.in1, ib + sa);
        EXPECT_EQ(res.in2, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        for (int i = 0; i < 5; ++i) EXPECT_EQ(ib[i], 1);
    }
    // Rvalue range + single iterator
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {1, 2, 3, 4, 5};
        auto rng0 = ranges::make_subrange(ib, ib + sa);
        auto res = ranges::transform(std::move(rng0), ia, ib, std::minus<int>());
        EXPECT_EQ(res.in1, ib + sa);
        EXPECT_EQ(res.in2, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        for (int i = 0; i < 5; ++i) EXPECT_EQ(ib[i], 1);
    }
    // Two ranges
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {1, 2, 3, 4, 5};
        auto rng0 = ranges::make_subrange(ib, ib + sa);
        auto rng1 = ranges::make_subrange(ia, ia + sa);
        auto res = ranges::transform(rng0, rng1, ib, std::minus<int>());
        EXPECT_EQ(res.in1, ib + sa);
        EXPECT_EQ(res.in2, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        for (int i = 0; i < 5; ++i) EXPECT_EQ(ib[i], 1);
    }
    // Two ranges, both rvalue
    {
        int ia[] = {0, 1, 2, 3, 4};
        constexpr auto sa = ranges::size(ia);
        int ib[sa] = {1, 2, 3, 4, 5};
        auto rng0 = ranges::make_subrange(ib, ib + sa);
        auto rng1 = ranges::make_subrange(ia, ia + sa);
        auto res = ranges::transform(std::move(rng0), std::move(rng1), ib, std::minus<int>());
        EXPECT_EQ(res.in1, ib + sa);
        EXPECT_EQ(res.in2, ia + sa);
        EXPECT_EQ(res.out, ib + sa);
        for (int i = 0; i < 5; ++i) EXPECT_EQ(ib[i], 1);
    }
}

struct S { int i; };

constexpr int plus_one(int i) { return i + 1; }
constexpr bool test_constexpr() {
    using namespace ranges;
    int ia[] = {0, 1, 2, 3, 4};
    constexpr auto sa = ranges::size(ia);
    int ib[sa] = {0};
    auto r = transform(ia, ib, plus_one);
    if (r.in != ia + sa) return false;
    if (r.out != ib + sa) return false;
    for (int i = 0; i < 5; ++i) if (ib[i] != i + 1) return false;
    return true;
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------

TEST(TransformTest, Unary) {
    test_unary();
}

TEST(TransformTest, Binary) {
    test_binary();
}

TEST(TransformTest, ProjectionAndConstexpr) {
    int *p = nullptr;
    auto unary = [](int i){ return i + 1; };
    auto binary = [](int i, int j){ return i + j; };
    S const s[] = {S{1}, S{2}, S{3}, S{4}};
    int const i[] = {1, 2, 3, 4};
    static_assert(std::is_same<ranges::unary_transform_result<S const*, int*>,
                               decltype(ranges::transform(s, p, unary, &S::i))>::value,
                  "unary_transform_result type mismatch");
    static_assert(std::is_same<ranges::binary_transform_result<S const*, int const*, int*>,
                               decltype(ranges::transform(s, i, p, binary, &S::i))>::value,
                  "binary_transform_result type mismatch");
    static_assert(std::is_same<ranges::binary_transform_result<S const*, S const*, int*>,
                               decltype(ranges::transform(s, s, p, binary, &S::i, &S::i))>::value,
                  "binary_transform_result type mismatch");

    static_assert(test_constexpr(), "transform constexpr test failed");
}
