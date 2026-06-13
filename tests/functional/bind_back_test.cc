// bind_back_gtest.cpp
// Google Test conversion of range-v3 bind_back algorithm test.
// Tests function binding with copy semantics.

#include <gtest/gtest.h>
#include <fermat/functional/bind_back.h>
#include <fermat/functional/concepts.h>


int* test(int& i) {
    return &i;
}

TEST(BindBackTest, Basic) {
    int i = 42;
    auto fn = ranges::bind_back(test, i);
    int* pi = fn();
    EXPECT_NE(pi, &i);   // bind_back copies the argument
    EXPECT_EQ(*pi, i);
}

// Check invocable concepts at compile time
using FnType = decltype(ranges::bind_back(test, 0));
static_assert(!ranges::invocable<FnType>, "FnType should not be invocable as prvalue");
static_assert(ranges::invocable<FnType&>, "FnType& should be invocable");
static_assert(!ranges::invocable<const FnType&>, "const FnType& should not be invocable");
