#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/copy_if.h>

/// Helper constexpr predicate
constexpr bool is_even(int i) {
    return i % 2 == 0;
}

/// Constexpr test for copy_if
constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    std::array<int, 4> a{{1, 2, 3, 4}};
    std::array<int, 4> b{{0, 0, 0, 0}};
    const auto res = copy_if(a, fermat::ranges::begin(b), is_even);
    bool ok = (res.in == end(a)) &&
              (res.out == begin(b) + 2) &&
              (a[0] == 1) &&
              (a[1] == 2) &&
              (a[2] == 3) &&
              (a[3] == 4) &&
              (b[0] == 2) &&
              (b[1] == 4) &&
              (b[2] == 0) &&
              (b[3] == 0);
    return ok;
}
static_assert(test_constexpr(), "");

TEST(CopyIfTest, ConstexprTest) {
    // All checks are done at compile time; runtime test simply passes.
    SUCCEED();
}