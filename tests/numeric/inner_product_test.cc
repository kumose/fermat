/// inner_product_gtest.cpp
/// Google Test conversion of range-v3 inner_product test.
/// All comments in English, using /// Doxygen style.

#include <gtest/gtest.h>
#include <fermat/numeric/inner_product.h>   /// Assume Fermat provides this

struct S {
    int i;
};

/// Helper to get array size at compile time.
template<typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) { return N; }

/// Test basic inner_product with raw pointers (satisfy all Fermat iterator concepts).
TEST(InnerProductTest, BasicWithPointers) {
    int a[] = {1, 2, 3, 4, 5, 6};
    int b[] = {6, 5, 4, 3, 2, 1};
    constexpr std::size_t n = array_size(a);

    /// Two-iterator version (first range given by [a, a+n), second range starts at b)
    auto r1 = ranges::inner_product(a, a + n, b, 0);
    EXPECT_EQ(r1, 56);
    auto r2 = ranges::inner_product(a, a + n, b, 10);
    EXPECT_EQ(r2, 66);

    /// Four-iterator version (both ranges fully specified)
    auto r3 = ranges::inner_product(a, a + n, b, b + n, 0);
    EXPECT_EQ(r3, 56);
    auto r4 = ranges::inner_product(a, a + n, b, b + n, 10);
    EXPECT_EQ(r4, 66);

    /// With custom binary operations: multiply and plus
    auto r5 = ranges::inner_product(a, a + n, b, b + n, 1,
                                    std::multiplies<int>(), std::plus<int>());
    /// Expected value: 1 * (1+6) * (2+5) * (3+4) * (4+3) * (5+2) * (6+1) = 7^6 = 117649
    EXPECT_EQ(r5, 117649);

    /// Array overload (if Fermat provides it)
    auto r6 = ranges::inner_product(a, b, 0);
    EXPECT_EQ(r6, 56);
    auto r7 = ranges::inner_product(a, b, 10);
    EXPECT_EQ(r7, 66);
}

/// Test inner_product with projection.
TEST(InnerProductTest, WithProjection) {
    S a[] = {{1}, {2}, {3}, {4}, {5}, {6}};
    S b[] = {{6}, {5}, {4}, {3}, {2}, {1}};
    constexpr std::size_t n = array_size(a);

    auto r = ranges::inner_product(a, a + n, b, b + n, 1,
                                   std::multiplies<int>(), std::plus<int>(),
                                   &S::i, &S::i);
    EXPECT_EQ(r, 117649);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}