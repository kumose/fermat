// adjacent_difference_gtest.cpp
// Google Test conversion of range-v3 adjacent_difference test.
// All comments in English.

#include <gtest/gtest.h>
#include <numeric>   // for std::adjacent_difference (fallback)
#include <fermat/numeric/adjacent_difference.h>   // assume Fermat provides this
#include <fermat/iterator/operations.h>           // for ranges::begin, ranges::end

struct S {
    int i;
};

// Helper to get array size
template<typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) { return N; }

// ------------------------------------------------------------------
// Test function using raw pointers (satisfy all iterator concepts)
// ------------------------------------------------------------------
template<typename InIter, typename OutIter>
void test_iterators() {
    using ranges::adjacent_difference;

    int ia[] = {15, 10, 6, 3, 1};
    int ir[] = {15, -5, -4, -3, -2};
    constexpr std::size_t s = array_size(ia);
    int ib[s] = {0};

    // 1) iterator pair version (input and output)
    auto r = adjacent_difference(InIter(ia), InIter(ia + s), OutIter(ib));
    EXPECT_EQ(r.in, ia + s);
    EXPECT_EQ(r.out, ib + s);
    for (std::size_t i = 0; i < s; ++i)
        EXPECT_EQ(ib[i], ir[i]);

    // 2) iterator pair version with binary operation
    int ir_plus[] = {15, 25, 16, 9, 4};
    int ib2[s] = {0};
    r = adjacent_difference(InIter(ia), InIter(ia + s), OutIter(ib2), std::plus<int>());
    EXPECT_EQ(r.in, ia + s);
    EXPECT_EQ(r.out, ib2 + s);
    for (std::size_t i = 0; i < s; ++i)
        EXPECT_EQ(ib2[i], ir_plus[i]);
}

// ------------------------------------------------------------------
// Google Test cases – only using raw pointers
// ------------------------------------------------------------------
TEST(AdjacentDifferenceTest, RawPointer) {
    test_iterators<const int*, int*>();
}

TEST(AdjacentDifferenceTest, Projection) {
    using ranges::adjacent_difference;
    using ranges::begin;

    S ia[] = {{15}, {10}, {6}, {3}, {1}};
    int ir[] = {15, -5, -4, -3, -2};
    constexpr std::size_t s = array_size(ir);
    int ib[s] = {0};

    auto r = adjacent_difference(begin(ia), begin(ia) + s,
                                 begin(ib), std::minus<int>(), &S::i);
    EXPECT_EQ(r.in, ia + s);
    EXPECT_EQ(r.out, ib + s);
    for (std::size_t i = 0; i < s; ++i)
        EXPECT_EQ(ib[i], ir[i]);
}

TEST(AdjacentDifferenceTest, BinaryOpDirect) {
    using ranges::adjacent_difference;
    using ranges::begin;

    int ia[] = {15, 10, 6, 3, 1};
    int ir[] = {15, 25, 16, 9, 4};
    constexpr std::size_t s = array_size(ir);
    int ib[s] = {0};

    // Use iterator version (since arrays decay to pointers)
    auto r = adjacent_difference(ia, ia + s, begin(ib), std::plus<int>());
    EXPECT_EQ(r.in, ia + s);
    EXPECT_EQ(r.out, ib + s);
    for (std::size_t i = 0; i < s; ++i)
        EXPECT_EQ(ib[i], ir[i]);
}

TEST(AdjacentDifferenceTest, ArrayToArray) {
    using ranges::adjacent_difference;

    int ia[] = {15, 10, 6, 3, 1};
    int ir[] = {15, 25, 16, 9, 4};
    constexpr std::size_t s = array_size(ir);
    int ib[s] = {0};

    // This overload takes two arrays (or pointers to first elements)
    auto r = adjacent_difference(ia, ib, std::plus<int>());
    EXPECT_EQ(r.in, ia + s);
    EXPECT_EQ(r.out, ib + s);
    for (std::size_t i = 0; i < s; ++i)
        EXPECT_EQ(ib[i], ir[i]);
}
