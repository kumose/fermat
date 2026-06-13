#include <gtest/gtest.h>
#include <vector>
#include <fermat/algorithm/generate.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/view/counted.h>
#include <fermat/view/subrange.h>

struct gen_test {
    int i_;
    constexpr gen_test() : i_{} {}
    constexpr gen_test(int i) : i_(i) {}
    constexpr int operator()() { return i_++; }
};

TEST(GenerateTest, Basic) {
    const unsigned n = 4;
    int ia[n] = {0};
    auto res = fermat::ranges::generate(ia, ia + n, gen_test(1));
    EXPECT_EQ(ia[0], 1);
    EXPECT_EQ(ia[1], 2);
    EXPECT_EQ(ia[2], 3);
    EXPECT_EQ(ia[3], 4);
    EXPECT_EQ(res.out, ia + n);
    EXPECT_EQ(res.fun.i_, 5);

    auto rng = fermat::ranges::make_subrange(ia, ia + n);
    res = fermat::ranges::generate(rng, res.fun);
    EXPECT_EQ(ia[0], 5);
    EXPECT_EQ(ia[1], 6);
    EXPECT_EQ(ia[2], 7);
    EXPECT_EQ(ia[3], 8);
    EXPECT_EQ(res.out, ia + n);
    EXPECT_EQ(res.fun.i_, 9);

    auto res2 = fermat::ranges::generate(std::move(rng), res.fun);
    EXPECT_EQ(ia[0], 9);
    EXPECT_EQ(ia[1], 10);
    EXPECT_EQ(ia[2], 11);
    EXPECT_EQ(ia[3], 12);
    EXPECT_EQ(res2.out, ia + n);
    EXPECT_EQ(res2.fun.i_, 13);
}

TEST(GenerateTest, OutputRange) {
    std::vector<int> v;
    auto rng = fermat::ranges::views::counted(fermat::ranges::back_inserter(v), 5);
    fermat::ranges::generate(rng, gen_test(1));
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);
}

constexpr bool test_constexpr_helper() {
    const unsigned n = 4;
    int ia[n] = {0};
    auto res = fermat::ranges::generate(ia, ia + n, gen_test(1));
    bool ok = true;
    ok = ok && (ia[0] == 1) && (ia[1] == 2) && (ia[2] == 3) && (ia[3] == 4);
    ok = ok && (res.out == ia + n) && (res.fun.i_ == 5);

    auto rng = fermat::ranges::make_subrange(ia, ia + n);
    auto res2 = fermat::ranges::generate(rng, res.fun);
    ok = ok && (ia[0] == 5) && (ia[1] == 6) && (ia[2] == 7) && (ia[3] == 8);
    ok = ok && (res2.out == ia + n) && (res2.fun.i_ == 9);

    auto res3 = fermat::ranges::generate(std::move(rng), res2.fun);
    ok = ok && (ia[0] == 9) && (ia[1] == 10) && (ia[2] == 11) && (ia[3] == 12);
    ok = ok && (res3.out == ia + n) && (res3.fun.i_ == 13);
    return ok;
}

TEST(GenerateTest, Constexpr) {
    static_assert(test_constexpr_helper(), "");
}