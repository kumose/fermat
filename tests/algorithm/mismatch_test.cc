#include <gtest/gtest.h>
#include <array>
#include <vector>
#include <functional>
#include <fermat/algorithm/mismatch.h>

/// Test mismatch with iterator pairs (plain pointers)
TEST(MismatchTest, IteratorPairs) {
    int ia[] = {0, 1, 2, 2, 0, 1, 2, 3};
    int ib[] = {0, 1, 2, 3, 0, 1, 2, 3};
    constexpr std::size_t sa = sizeof(ia)/sizeof(ia[0]);

    // three-iterator version (first2 only)
    auto res = ranges::mismatch(ia, ia + sa, ib);
    EXPECT_EQ(res.in1, ia + 3);
    EXPECT_EQ(res.in2, ib + 3);

    // four-iterator version
    res = ranges::mismatch(ia, ia + sa, ib, ib + sa);
    EXPECT_EQ(res.in1, ia + 3);
    EXPECT_EQ(res.in2, ib + 3);

    // different length for second range
    res = ranges::mismatch(ia, ia + sa, ib, ib + 2);
    EXPECT_EQ(res.in1, ia + 2);
    EXPECT_EQ(res.in2, ib + 2);

    // with predicate (three-iterator)
    res = ranges::mismatch(ia, ia + sa, ib, std::equal_to<int>());
    EXPECT_EQ(res.in1, ia + 3);
    EXPECT_EQ(res.in2, ib + 3);

    // with predicate (four-iterator)
    res = ranges::mismatch(ia, ia + sa, ib, ib + sa, std::equal_to<int>());
    EXPECT_EQ(res.in1, ia + 3);
    EXPECT_EQ(res.in2, ib + 3);

    // with predicate and different length
    res = ranges::mismatch(ia, ia + sa, ib, ib + 2, std::equal_to<int>());
    EXPECT_EQ(res.in1, ia + 2);
    EXPECT_EQ(res.in2, ib + 2);
}

/// Test mismatch with range versions (using containers)
TEST(MismatchTest, RangeVersions) {
    int ia[] = {0, 1, 2, 2, 0, 1, 2, 3};
    int ib[] = {0, 1, 2, 3, 0, 1, 2, 3};
    auto rng1 = ranges::make_subrange(ia, ia + 8);
    auto rng2 = ranges::make_subrange(ib, ib + 8);

    auto res = ranges::mismatch(rng1, rng2);
    EXPECT_EQ(res.in1, ia + 3);
    EXPECT_EQ(res.in2, ib + 3);

    // with predicate
    res = ranges::mismatch(rng1, rng2, std::equal_to<int>());
    EXPECT_EQ(res.in1, ia + 3);
    EXPECT_EQ(res.in2, ib + 3);

    // different lengths
    auto rng3 = ranges::make_subrange(ib, ib + 2);
    res = ranges::mismatch(rng1, rng3);
    EXPECT_EQ(res.in1, ia + 2);
    EXPECT_EQ(res.in2, ib + 2);
}

/// Test mismatch with projections
TEST(MismatchTest, Projection) {
    struct S { int i; };
    S s1[] = {{1},{2},{3},{4},{-4},{5},{6},{40},{7},{8},{9}};
    int const i1[] = {1,2,3,4,5,6,7,8,9};

    auto res = ranges::mismatch(s1, i1, std::equal_to<int>(), &S::i);
    EXPECT_EQ(res.in1->i, -4);
    EXPECT_EQ(*res.in2, 5);

    S s2[] = {{1},{2},{3},{4},{5},{6},{40},{7},{8},{9}};
    auto res2 = ranges::mismatch(s1, s2, std::equal_to<int>(), &S::i, &S::i);
    EXPECT_EQ(res2.in1->i, -4);
    EXPECT_EQ(res2.in2->i, 5);
}

/// Constexpr tests with std::array
TEST(MismatchTest, Constexpr) {
    constexpr std::array<int,11> r1{{1,2,3,4,-4,5,6,40,7,8,9}};
    constexpr std::array<int,9> r11{{1,2,3,4,5,6,7,8,9}};
    constexpr std::array<int,10> r2{{1,2,3,4,5,6,40,7,8,9}};

    static_assert(*ranges::mismatch(r1, r11, std::equal_to<int>()).in1 == -4, "");
    static_assert(*ranges::mismatch(r1, r11, std::equal_to<int>()).in2 == 5, "");
    static_assert(*ranges::mismatch(r1, r2, std::equal_to<int>()).in1 == -4, "");
    static_assert(*ranges::mismatch(r1, r2, std::equal_to<int>()).in2 == 5, "");
}
