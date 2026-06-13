// unformatted_ostream_iterator_gtest.cpp
// Google Test conversion of range-v3 unformatted_ostream_iterator test.
// All comments in English.

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include <fermat/algorithm/copy.h>
#include <fermat/iterator/stream_iterators.h>
#include <fermat/range/conversion.h>
#include <fermat/view/for_each.h>
#include <fermat/view/reverse.h>
#include <fermat/view/sliding.h>
#include <fermat/view/stride.h>

namespace {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    template<typename>
    std::string endian_adjust(std::string const &s) {
        return s;
    }
#else
    template<typename T>
    std::string endian_adjust(std::string const &s) {
        namespace rv = fermat::ranges::views;
        return rv::sliding(s, static_cast<std::ptrdiff_t>(sizeof(T))) //
               | rv::stride(static_cast<std::ptrdiff_t>(sizeof(T))) //
               | rv::for_each([](auto x) { return x | rv::reverse; }) //
               | fermat::ranges::to<std::string>;
    }
#endif // endianness
} // namespace

TEST(UnformattedOstreamIterator, CopyVectorOfChar) {
    constexpr auto expected = "\x21\x22\x23\x24";
    auto const input = std::vector<char>{0x21, 0x22, 0x23, 0x24};
    auto output = std::ostringstream();
    fermat::ranges::copy(input, fermat::ranges::unformatted_ostream_iterator<>(output));
    auto const actual = output.str();
    EXPECT_EQ(actual, expected);
}

TEST(UnformattedOstreamIterator, CopyVectorOfUnsignedShort) {
    constexpr auto expected = "\x21\x22\x23\x24";
    auto const input = std::vector<unsigned short>{0x2122, 0x2324};
    auto output = std::ostringstream();
    fermat::ranges::copy(input, fermat::ranges::unformatted_ostream_iterator<>(output));
    auto const actual = endian_adjust<unsigned short>(output.str());
    EXPECT_EQ(actual, expected);
}

#if __cplusplus > 201703L
TEST(UnformattedOstreamIterator, CopyVectorOfFloatSingle) {
    constexpr auto expected = "\x21\x22\x23\x24";
    // float computed to be *exactly* 0x21222324.
    auto const input = std::vector<float>{0x1.4446480000000000179Dp-61f};
    auto output = std::ostringstream();
    fermat::ranges::copy(input, fermat::ranges::unformatted_ostream_iterator<>(output));
    auto const actual = endian_adjust<float>(output.str());
    EXPECT_EQ(actual, expected);
}
#endif // __cplusplus > 201703L

TEST(UnformattedOstreamIterator, CopyVectorOfUnsignedInt) {
    constexpr auto expected = "\x21\x22\x23\x24\x25\x26\x27\x28";
    auto const input = std::vector<unsigned int>{0x21222324, 0x25262728};
    auto output = std::ostringstream();
    fermat::ranges::copy(input, fermat::ranges::unformatted_ostream_iterator<>(output));
    auto const actual = endian_adjust<unsigned int>(output.str());
    EXPECT_EQ(actual, expected);
}

#if __cplusplus > 201703L
TEST(UnformattedOstreamIterator, CopyVectorOfFloatTwo) {
    constexpr auto expected = "\x21\x22\x23\x24\x25\x26\x27\x28";
    // floats computed to be *exactly* 0x21222324 and 0x25262728.
    auto const input = std::vector<float>{0x1.4446480000000000179Dp-61f, 0x1.4C4E5p-53f};
    auto output = std::ostringstream();
    fermat::ranges::copy(input, fermat::ranges::unformatted_ostream_iterator<>(output));
    auto const actual = endian_adjust<unsigned int>(output.str());
    EXPECT_EQ(actual, expected);
}

TEST(UnformattedOstreamIterator, CopyVectorOfDouble) {
    constexpr auto expected = "\x21\x22\x23\x24\x25\x26\x27\x28";
    // double computed to be *exactly* 0x2122232425262728.
    auto const input = std::vector<double>{0x1.223242526272800006F2p-493};
    auto output = std::ostringstream();
    fermat::ranges::copy(input, fermat::ranges::unformatted_ostream_iterator<>(output));
    auto const actual = endian_adjust<double>(output.str());
    EXPECT_EQ(actual, expected);
}
#endif // __cplusplus > 201703L
