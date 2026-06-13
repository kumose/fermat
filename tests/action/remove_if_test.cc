#include <gtest/gtest.h>
#include <vector>
#include <functional>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/algorithm/move.h>
#include <fermat/action/remove_if.h>
#include <fermat/range/conversion.h>

using namespace ranges;

TEST(ActionRemoveIfTest, Example) {
    auto v = views::ints(1, 21) | to<std::vector>();
    auto& v2 = actions::remove_if(v, [](int i) { return i % 2 == 0; });
    EXPECT_EQ(&v, &v2);
    EXPECT_TRUE((v == std::vector<int>{1, 3, 5, 7, 9, 11, 13, 15, 17, 19}));

    auto&& v3 = std::move(v) | actions::remove_if(std::bind(std::less<int>{}, std::placeholders::_1, 10));
    EXPECT_TRUE((v3 == std::vector<int>{11, 13, 15, 17, 19}));
}
