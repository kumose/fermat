#include <gtest/gtest.h>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/equal.h>
#include <fermat/action/transform.h>
#include <fermat/range/conversion.h>

using namespace fermat::ranges;

TEST(ActionTransformTest, Example) {
    auto v = views::ints(0, 10) | to<std::vector>();

    auto v0 = v | copy | actions::transform([](int i) { return i * i; });
    static_assert(std::is_same_v<decltype(v), decltype(v0)>);
    EXPECT_TRUE((v0 == std::vector<int>{0, 1, 4, 9, 16, 25, 36, 49, 64, 81}));

    actions::transform(v, [](int i) { return i * i; });
    EXPECT_TRUE((v == std::vector<int>{0, 1, 4, 9, 16, 25, 36, 49, 64, 81}));
}