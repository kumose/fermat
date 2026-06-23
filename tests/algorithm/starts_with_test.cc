// starts_with_gtest.cpp
// Google Test conversion of range-v3 starts_with algorithm test.
// Uses fermat::range and standard library components.
// All comments in English.

#include <gtest/gtest.h>
#include <forward_list>
#include <sstream>
#include <vector>

#include <fermat/algorithm/starts_with.h>
#include <fermat/range/conversion.h>
#include <fermat/view/iota.h>
#include <fermat/view/slice.h>
#include <fermat/view/take_exactly.h>
#include <fermat/view/istream.h>


// ------------------------------------------------------------
// Test implementations (converted from original)
// ------------------------------------------------------------
void test_defaults() {
    using namespace fermat::ranges;
    // checks starts_with works for input ranges
    {
        constexpr auto full_latin_alphabet = "a b c d e f g h i j k l m n o p q r s t u v w x y z";
        auto const partial_latin_alphabet = "a b c d";

        {
            auto long_stream = std::istringstream{full_latin_alphabet};
            auto short_stream = std::istringstream{partial_latin_alphabet};

            auto r1 = istream<char>(long_stream);
            auto r2 = istream<char>(short_stream);
            EXPECT_TRUE(starts_with(begin(r1), end(r1), begin(r2), end(r2)));
        }
        {
            auto long_stream = std::istringstream{full_latin_alphabet};
            auto short_stream = std::istringstream{partial_latin_alphabet};

            auto r1 = istream<char>(long_stream);
            auto r2 = istream<char>(short_stream);
            EXPECT_FALSE(starts_with(begin(r2), end(r2), begin(r1), end(r1)));
        }
        {
            auto long_stream = std::istringstream{full_latin_alphabet};
            auto short_stream = std::istringstream{partial_latin_alphabet};
            EXPECT_TRUE(starts_with(istream<char>(long_stream), istream<char>(short_stream)));
        }
        {
            auto long_stream = std::istringstream{full_latin_alphabet};
            auto short_stream = std::istringstream{partial_latin_alphabet};
            EXPECT_FALSE(starts_with(istream<char>(short_stream), istream<char>(long_stream)));
        }
    }
    // checks starts_with works for random-access ranges
    {
#ifdef RANGES_WORKAROUND_MSVC_779708
        auto const long_range = views::iota(0, 100) | to<std::vector>();
        auto const short_range = views::iota(0, 10) | to<std::vector>();
#else
        auto const long_range = views::iota(0, 100) | to<std::vector>;
        auto const short_range = views::iota(0, 10) | to<std::vector>;
#endif
        EXPECT_TRUE(starts_with(begin(long_range), end(long_range), begin(short_range), end(short_range)));
        EXPECT_TRUE(starts_with(long_range, short_range));

        EXPECT_FALSE(starts_with(begin(short_range), end(short_range), begin(long_range), end(long_range)));
        EXPECT_FALSE(starts_with(short_range, long_range));
    }
    // checks starts_with works for random-access ranges with arbitrary sentinels
    {
        auto const long_range = views::iota(0);
        auto const short_range = views::iota(0) | views::take_exactly(100);

        EXPECT_TRUE(starts_with(begin(long_range), end(long_range), begin(short_range), end(short_range)));
        EXPECT_TRUE(starts_with(long_range, short_range));

        EXPECT_FALSE(starts_with(begin(short_range), end(short_range), begin(long_range), end(long_range)));
        EXPECT_FALSE(starts_with(short_range, long_range));
    }
    // checks starts_with identifies a subrange
    {
        auto const range = views::iota(0) | views::slice(50, 100);
        auto const offset = views::iota(50, 100);

        EXPECT_TRUE(starts_with(begin(range), end(range), begin(offset), end(offset)));
        EXPECT_TRUE(starts_with(range, offset));

        EXPECT_TRUE(starts_with(begin(offset), end(offset), begin(range), end(range)));
        EXPECT_TRUE(starts_with(offset, range));
    }
    // checks starts_with identifies when two ranges don't have the same start sequence
    {
        auto const first = views::iota(0, 1'000);
        auto const second = views::iota(10, 1'000);

        EXPECT_FALSE(starts_with(begin(first), end(first), begin(second), end(second)));
        EXPECT_FALSE(starts_with(first, second));

        EXPECT_FALSE(starts_with(begin(second), end(second), begin(first), end(first)));
        EXPECT_FALSE(starts_with(second, first));
    }
}

void test_comparison() {
    using namespace fermat::ranges;
    auto const long_range = views::iota(0, 100);
    auto const short_range = views::iota(1, 51);
    EXPECT_TRUE(starts_with(begin(long_range), end(long_range), begin(short_range), end(short_range),
                            less{}));
    EXPECT_TRUE(starts_with(long_range, short_range, less{}));

    EXPECT_FALSE(starts_with(begin(long_range), end(long_range), begin(short_range), end(short_range),
                             greater{}));
    EXPECT_FALSE(starts_with(long_range, short_range, greater{}));
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------
TEST(StartsWithTest, Defaults) {
    test_defaults();
}

TEST(StartsWithTest, Comparison) {
    test_comparison();
}
