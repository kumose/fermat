#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/repeat_n.h>
#include <fermat/view/for_each.h>
#include <fermat/view/take.h>
#include <fermat/algorithm/shuffle.h>
#include <fermat/algorithm/equal.h>
#include <fermat/algorithm/is_sorted.h>
#include <fermat/action/shuffle.h>
#include <fermat/action/sort.h>
#include <fermat/action/unique.h>
#include <fermat/range/conversion.h>

using namespace ranges;

TEST(ActionSortUniqueTest, ShuffleThenSortUnique) {
    std::mt19937 gen;

    // [1,2,2,3,3,3,4,4,4,4,5,5,5,5,5,...]
    auto v = views::for_each(views::ints(1, 100), [](int i) {
                return yield_from(views::repeat_n(i, i));
            }) | to<std::vector>();

    auto first15 = views::take(v, 15) | to<std::vector>();
    EXPECT_TRUE((first15 == std::vector<int>{1,2,2,3,3,3,4,4,4,4,5,5,5,5,5}));

    v |= actions::shuffle(gen);
    EXPECT_FALSE(is_sorted(v));

    v |= actions::sort | actions::unique;
    EXPECT_TRUE(equal(v, views::ints(1, 100)));
}