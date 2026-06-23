#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <cmath>
#include <functional>
#include <fermat/algorithm/fold.h>
#include <fermat/algorithm/min.h>
#include <fermat/view/subrange.h>
#include <fermat/iterator.h>

struct Approx {
    double value;
    Approx(double v) : value(v) {}
    friend bool operator==(Approx a, double d) {
        return std::fabs(a.value - d) < 0.001;
    }
    friend bool operator==(double d, Approx a) {
        return a == d;
    }
};

template<class Iter, class Sent = Iter>
void test_left() {
    using namespace fermat::ranges;
    double da[] = {0.25, 0.75};
    EXPECT_EQ(fold_left(Iter(da), Sent(da), 1, std::plus<>()), Approx{1.0});
    EXPECT_EQ(fold_left(Iter(da), Sent(da + 2), 1, std::plus<>()), Approx{2.0});

    auto res1 = fold_left_first(Iter(da), Sent(da), fermat::ranges::min);
    EXPECT_EQ(res1, std::nullopt);

    auto res2 = fold_left_first(Iter(da), Sent(da + 2), fermat::ranges::min);
    ASSERT_TRUE(res2.has_value());
    EXPECT_EQ(*res2, Approx(0.25));

    EXPECT_EQ(fold_left(make_subrange(Iter(da), Sent(da)), 1, std::plus<>()), Approx{1.0});
    EXPECT_EQ(fold_left(make_subrange(Iter(da), Sent(da + 2)), 1, std::plus<>()), Approx{2.0});

    auto res3 = fold_left_first(make_subrange(Iter(da), Sent(da)), fermat::ranges::min);
    EXPECT_EQ(res3, std::nullopt);

    auto res4 = fold_left_first(make_subrange(Iter(da), Sent(da + 2)), fermat::ranges::min);
    ASSERT_TRUE(res4.has_value());
    EXPECT_EQ(*res4, Approx(0.25));
}

void test_right() {
    using namespace fermat::ranges;
    double da[] = {0.25, 0.75};
    EXPECT_EQ(fold_right(da, da + 2, 1, std::plus<>()), Approx{2.0});
    EXPECT_EQ(fold_right(da, da + 2, 1, std::minus<>()), Approx{0.5});

    int xs[] = {1, 2, 3};
    auto concat = [](int i, std::string s) { return s + std::to_string(i); };
    EXPECT_EQ(fold_right(xs, xs + 2, std::string(), concat), "21");
    EXPECT_EQ(fold_right(xs, xs + 3, std::string(), concat), "321");
}

TEST(FoldTest, Right) {
    test_right();
}