// diffmax_t_gtest.cpp
// Google Test conversion of range-v3 diffmax_t test.
// All comments in English.

#include <gtest/gtest.h>
#include <functional>
#include <iomanip>

#include <fermat/iterator/diffmax_t.h>

using ranges::detail::diffmax_t;

template<template<typename> class Op>
void check_1(std::ptrdiff_t a, std::ptrdiff_t b) {
    EXPECT_EQ(Op<diffmax_t>{}(a, b), Op<std::ptrdiff_t>{}(a, b));
}
template<>
void check_1<std::divides>(std::ptrdiff_t a, std::ptrdiff_t b) {
    if (b != 0)
        EXPECT_EQ(std::divides<diffmax_t>{}(a, b), std::divides<std::ptrdiff_t>{}(a, b));
}
template<>
void check_1<std::modulus>(std::ptrdiff_t a, std::ptrdiff_t b) {
    if (b != 0)
        EXPECT_EQ(std::modulus<diffmax_t>{}(a, b), std::modulus<std::ptrdiff_t>{}(a, b));
}

template<template<typename> class Op>
void check() {
    check_1<Op>(0, 0);
    check_1<Op>(-1, 0);
    check_1<Op>(0, -1);
    check_1<Op>(1, 0);
    check_1<Op>(0, 1);
    check_1<Op>(1, 1);
    check_1<Op>(-1, -1);
    check_1<Op>(-5, -4);
    check_1<Op>(-4, -5);
    check_1<Op>(5, -4);
    check_1<Op>(-4, 5);
    check_1<Op>(-5, 4);
    check_1<Op>(4, -5);
}

TEST(DiffmaxTest, Operators) {
    check<std::plus>();
    check<std::minus>();
    check<std::multiplies>();
    check<std::divides>();
    check<std::modulus>();
    check<std::bit_and>();
    check<std::bit_or>();
    check<std::bit_xor>();
}
