#include <gtest/gtest.h>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/action/drop.h>
#include <fermat/range/conversion.h>

TEST(ActionDropTest, Example) {
    using namespace fermat::ranges;

    auto v = views::ints(1, 21) | to<std::vector>();
    auto& v2 = actions::drop(v, 3);
    EXPECT_EQ(&v, &v2);
    EXPECT_EQ(v.size(), 17u);
    EXPECT_EQ(v[0], 4);

    v = std::move(v) | actions::drop(3);
    EXPECT_EQ(v.size(), 14u);
    EXPECT_EQ(v[0], 7);

    v |= actions::drop(3);
    EXPECT_EQ(v.size(), 11u);
    EXPECT_EQ(v[0], 10);

    v |= actions::drop(100);
    EXPECT_EQ(v.size(), 0u);
}