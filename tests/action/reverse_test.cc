#include <gtest/gtest.h>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/repeat_n.h>
#include <fermat/view/for_each.h>
#include <fermat/action/reverse.h>
#include <fermat/action/unique.h>
#include <fermat/range/conversion.h>

using namespace fermat::ranges;

TEST(ActionUniqueReverseTest, Example) {
    /// Build vector: [1,2,2,3,3,3,4,4,4,4,5,5,5,5,5]
    auto v = views::for_each(views::ints(1, 6), [](int i) {
                return yield_from(views::repeat_n(i, i));
            }) | to<std::vector>();
    EXPECT_TRUE((v == std::vector<int>{1,2,2,3,3,3,4,4,4,4,5,5,5,5,5}));

    /// Apply unique then reverse
    v |= actions::unique | actions::reverse;
    EXPECT_TRUE((v == std::vector<int>{5,4,3,2,1}));
}