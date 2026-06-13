#include <gtest/gtest.h>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/action/take.h>
#include <fermat/range/conversion.h>

using namespace ranges;

TEST(ActionTakeTest, Example) {
    auto v = views::ints(1, 21) | to<std::vector>();
    auto& v2 = actions::take(v, 17);
    EXPECT_EQ(&v2, &v);
    EXPECT_EQ(v.size(), 17u);
    EXPECT_EQ(v.back(), 17);

    v = std::move(v) | actions::take(14);
    EXPECT_EQ(v.size(), 14u);
    EXPECT_EQ(v.back(), 14);

    v |= actions::take(11);
    EXPECT_EQ(v.size(), 11u);
    EXPECT_EQ(v.back(), 11);

    v |= actions::take(100);
    EXPECT_EQ(v.size(), 11u);

    v |= actions::take(0);
    EXPECT_EQ(v.size(), 0u);
}