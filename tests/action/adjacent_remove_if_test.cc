#include <gtest/gtest.h>
#include <fermat/action/adjacent_remove_if.h>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/range/conversion.h>
#include <vector>

TEST(ActionAdjacentRemoveIfTest, Example) {
    using namespace fermat::ranges;

    auto v = views::ints(1, 21) | to<std::vector>();
    auto& v2 = actions::adjacent_remove_if(v, [](int x, int y) { return (x + y) % 3 == 0; });
    EXPECT_EQ(&v, &v2);  // same object

    std::vector<int> expected1 = {2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18, 20};
    EXPECT_EQ(v, expected1);

    v |= actions::adjacent_remove_if([](int x, int y) { return (y - x) == 2; });
    std::vector<int> expected2 = {2, 5, 8, 11, 14, 17, 20};
    EXPECT_EQ(v, expected2);
}