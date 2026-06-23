// on_gtest.cpp
// Google Test conversion of range-v3 on algorithm test.
// Tests the on() functional adaptor.

#include <gtest/gtest.h>
#include <fermat/functional/on.h>
#include <fermat/functional/concepts.h>


int square(int i) { return i * i; }

TEST(OnTest, Basic) {
    auto fn = fermat::ranges::on(std::multiplies<>{}, square);
    EXPECT_EQ(fn(2, 4), 64);
}
