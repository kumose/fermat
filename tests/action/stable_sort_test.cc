#include <gtest/gtest.h>
#include <array>
#include <random>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/stride.h>
#include <fermat/view/take.h>
#include <fermat/algorithm/shuffle.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/is_sorted.h>
#include <fermat/algorithm/equal.h>
#include <fermat/action/shuffle.h>
#include <fermat/action/stable_sort.h>
#include <fermat/range/conversion.h>

using namespace fermat::ranges;

#if !defined(__clang__) || !defined(_MSVC_STL_VERSION) // Avoid #890

TEST(ActionStableSortTest, Bug632) {
    const std::vector<double> scores = {3.0, 1.0, 2.0};
    std::vector<int> indices = {0, 1, 2};

    indices |= actions::stable_sort(
        less{},
        [&](const int& x) { return scores[static_cast<std::size_t>(x)]; }
    );
    EXPECT_TRUE((indices == std::vector<int>{1, 2, 0}));
}

TEST(ActionStableSortTest, ShuffleThenStableSort) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    v |= actions::shuffle(gen);
    EXPECT_FALSE(is_sorted(v));

    auto v2 = v | copy | actions::stable_sort;
    EXPECT_EQ(v2.size(), v.size());
    EXPECT_TRUE(is_sorted(v2));
    EXPECT_FALSE(is_sorted(v));
    static_assert(std::is_same_v<decltype(v), decltype(v2)>);

    v |= actions::stable_sort;
    EXPECT_TRUE(is_sorted(v));

    v |= actions::shuffle(gen);
    EXPECT_FALSE(is_sorted(v));

    v = std::move(v) | actions::stable_sort(std::less<int>());
    EXPECT_TRUE(is_sorted(v));
    EXPECT_TRUE(equal(v, v2));
}

TEST(ActionStableSortTest, DirectCallByReference) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    shuffle(v, gen);
    EXPECT_FALSE(is_sorted(v));

    auto& v3 = actions::stable_sort(v);
    EXPECT_TRUE(is_sorted(v));
    EXPECT_EQ(&v3, &v);
}

TEST(ActionStableSortTest, PipeViewToStableSort) {
    std::mt19937 gen;

    auto v = views::ints(0, 100) | to<std::vector>();
    actions::stable_sort(v, std::greater<int>());

    v | views::stride(2) | actions::stable_sort;
    auto first10 = views::take(v, 10) | to<std::vector>();
    EXPECT_TRUE((first10 == std::vector<int>{1, 98, 3, 96, 5, 94, 7, 92, 9, 90}));
}

TEST(ActionStableSortTest, RefView) {
    auto v = views::ints(0, 100) | to<std::vector>();
    auto r = views::ref(v);
    r |= actions::stable_sort;
    EXPECT_TRUE(is_sorted(v));
}

#else
// Empty test suite when condition not met
TEST(ActionStableSortTest, Dummy) {
    SUCCEED();
}
#endif // Avoid #890