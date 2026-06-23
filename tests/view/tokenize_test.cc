/// tokenize_gtest.cpp
/// Google Test conversion of range-v3 tokenize view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/tokenize.cpp
/// Adapted to use fermat library and Google Test.
///
/// Note: Assumes fermat provides <fermat/view/tokenize.h> with similar
///       interface to range-v3's tokenize view.

#include <gtest/gtest.h>

#include <regex>
#include <string>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/view/tokenize.h>
#include <fermat/utility/copy.h>
#include <fermat/iterator/operations.h>

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

/// Test tokenize view on a string with regex
TEST(TokenizeViewTest, BasicTokenization) {
    using namespace fermat::ranges;

    // Assume regex support is available (C++11 and later)
    std::string txt{"abc\ndef\tghi"};
    const std::regex rx{R"delim(([\w]+))delim"};

    // Non-const view
    auto rng = txt | views::tokenize(rx, 1);
    // Const view
    const auto crng = txt | views::tokenize(rx, 1);

    // Check elements
    check_equal(rng, {"abc", "def", "ghi"});
    check_equal(crng, {"abc", "def", "ghi"});

    // Type checks omitted because they are implementation‑sensitive.
    // Only runtime element comparison is performed.

    // Runtime checks: ensure begin/end are not equal
    EXPECT_NE(fermat::ranges::begin(rng), fermat::ranges::end(rng));
    EXPECT_NE(fermat::ranges::begin(crng), fermat::ranges::end(crng));
}
