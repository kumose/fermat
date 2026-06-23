#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/partition_copy.h>

/// predicate for testing
struct is_odd {
    constexpr bool operator()(const int& i) const { return i & 1; }
};

/// constexpr test (compiler‑time)
constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    const int ia[] = {1, 2, 3, 4, 6, 8, 5, 7};
    int r1[10] = {0};
    int r2[10] = {0};
    auto p = partition_move(ia, r1, r2, is_odd{});
    bool ok = (p.in == std::end(ia)) &&
              (p.out1 == r1 + 4) &&
              (r1[0] == 1) && (r1[1] == 3) && (r1[2] == 5) && (r1[3] == 7) &&
              (p.out2 == r2 + 4) &&
              (r2[0] == 2) && (r2[1] == 4) && (r2[2] == 6) && (r2[3] == 8);
    return ok;
}
static_assert(test_constexpr(), "");

TEST(PartitionMoveTest, Constexpr) {
    // all checks done at compile time
    SUCCEED();
}
