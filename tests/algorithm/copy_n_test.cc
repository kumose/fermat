#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/copy_n.h>

constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    std::array<int, 4> a{{1, 2, 3, 4}};
    std::array<int, 4> b{{0, 0, 0, 0}};
    const auto res = copy_n(begin(a), 2, begin(b));
    bool ok = (res.in == begin(a) + 2) &&
              (res.out == begin(b) + 2) &&
              (a[0] == 1) &&
              (a[1] == 2) &&
              (a[2] == 3) &&
              (a[3] == 4) &&
              (b[0] == 1) &&
              (b[1] == 2) &&
              (b[2] == 0) &&
              (b[3] == 0);
    return ok;
}

TEST(CopyNTest, ConstexprTest) {
    EXPECT_TRUE(test_constexpr());
}