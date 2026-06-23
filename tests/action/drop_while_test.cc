#include <gtest/gtest.h>
#include <vector>
#include <functional>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/action/drop_while.h>
#include <fermat/range/conversion.h>

TEST(ActionDropWhileTest, Example) {
    using namespace fermat::ranges;
    using namespace std::placeholders;

    auto v = views::ints(1, 21) | to<std::vector>();
    auto& v2 = actions::drop_while(v, std::bind(std::less<int>(), _1, 4));
    EXPECT_EQ(&v, &v2);
    EXPECT_EQ(v.size(), 17u);
    EXPECT_EQ(v[0], 4);

    v = std::move(v) | actions::drop_while([](int i) { return i < 7; });
    EXPECT_EQ(v.size(), 14u);
    EXPECT_EQ(v[0], 7);

    v |= actions::drop_while([](int i) { return i < 10; });
    EXPECT_EQ(v.size(), 11u);
    EXPECT_EQ(v[0], 10);

    v |= actions::drop_while([](int) { return true; });
    EXPECT_EQ(v.size(), 0u);
}