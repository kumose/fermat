/// istream_gtest.cpp
/// Google Test conversion of range-v3 istream view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>

#include <fermat/algorithm/equal.h>
#include <fermat/view/istream.h>
#include <fermat/view/subrange.h>

TEST(IstreamTest, CharStream) {
    using namespace ranges;

    static const char test[] = "abcd3210";
    std::istringstream ss{test};
    auto rng = istream<char>(ss);
    auto expected = subrange<const char*>(test, test + sizeof(test) - 1);
    EXPECT_TRUE(equal(rng, expected));
}
