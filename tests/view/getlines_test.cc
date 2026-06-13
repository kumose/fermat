/// getlines_gtest.cpp
/// Google Test conversion of range-v3 getlines view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/getlines.h>          /// ranges::views::getlines

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(GetlinesTest, Basic) {
    using namespace ranges;

    const char* text =
R"(Now is
the time
for all
good men
)";

    std::stringstream sin{text};
    auto rng = getlines(sin);
    check_equal(rng, {"Now is", "the time", "for all", "good men"});

    // Type checks (compile-time)
    static_assert(std::is_same<range_rvalue_reference_t<decltype(rng)>, std::string&&>::value,
                  "rvalue reference type mismatch");
}
