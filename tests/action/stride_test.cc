#include <gtest/gtest.h>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/equal.h>
#include <fermat/action/stride.h>
#include <fermat/range/conversion.h>

using namespace ranges;

TEST(ActionStrideTest, Basic) {
    auto v = views::ints(0, 100) | to<std::vector>();

    auto v2 = v | copy | actions::stride(10);
    EXPECT_EQ(v2.size(), 10u);
    static_assert(std::is_same_v<decltype(v), decltype(v2)>);
    EXPECT_TRUE((v2 == std::vector<int>{0, 10, 20, 30, 40, 50, 60, 70, 80, 90}));

    v2 = std::move(v2) | actions::stride(4);
    EXPECT_TRUE((v2 == std::vector<int>{0, 40, 80}));

    v2 |= actions::stride(2);
    EXPECT_TRUE((v2 == std::vector<int>{0, 80}));

    v2 |= actions::stride(1);
    EXPECT_TRUE((v2 == std::vector<int>{0, 80}));

    v2 |= actions::stride(10);
    EXPECT_TRUE((v2 == std::vector<int>{0}));

    auto& v3 = actions::stride(v, 30);
    EXPECT_EQ(&v3, &v);
    EXPECT_TRUE((v == std::vector<int>{0, 30, 60, 90}));
}