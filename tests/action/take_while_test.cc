#include <gtest/gtest.h>
#include <vector>
#include <functional>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/action/take_while.h>
#include <fermat/range/conversion.h>

using namespace fermat::ranges;

TEST(ActionTakeWhileTest, Example) {
    using namespace std::placeholders;

    auto v = views::ints(1, 21) | to<std::vector>();
    auto& v2 = actions::take_while(v, std::bind(std::less<int>(), _1, 18));
    EXPECT_EQ(&v2, &v);
    EXPECT_EQ(v.size(), 17u);
    EXPECT_EQ(v.back(), 17);

    v = std::move(v) | actions::take_while([](int i) { return i < 15; });
    EXPECT_EQ(v.size(), 14u);
    EXPECT_EQ(v.back(), 14);

    v |= actions::take_while([](int i) { return i < 12; });
    EXPECT_EQ(v.size(), 11u);
    EXPECT_EQ(v.back(), 11);

    v |= actions::take_while([](int) { return true; });
    EXPECT_EQ(v.size(), 11u);

    v |= actions::take_while([](int) { return false; });
    EXPECT_EQ(v.size(), 0u);
}