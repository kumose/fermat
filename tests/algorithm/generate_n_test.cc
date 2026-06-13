#include <gtest/gtest.h>
#include <vector>
#include <fermat/algorithm/generate_n.h>
#include <fermat/iterator/insert_iterators.h>

struct gen_test {
    int i_;
    constexpr gen_test() : i_{} {}
    constexpr gen_test(int i) : i_(i) {}
    constexpr int operator()() { return i_++; }
};

TEST(GenerateNTest, Basic) {
    const unsigned n = 4;
    int ia[n] = {0};
    auto res = ranges::generate_n(ia, n, gen_test(1));
    EXPECT_EQ(ia[0], 1);
    EXPECT_EQ(ia[1], 2);
    EXPECT_EQ(ia[2], 3);
    EXPECT_EQ(ia[3], 4);
    EXPECT_EQ(res.out, ia + n);
    EXPECT_EQ(res.fun.i_, 5);
}

TEST(GenerateNTest, OutputRange) {
    std::vector<int> v;
    ranges::generate_n(ranges::back_inserter(v), 5, gen_test(1));
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);
}

constexpr bool test_constexpr() {
    const unsigned n = 4;
    int ia[n] = {0};
    auto res = ranges::generate_n(ia, n, gen_test(1));
    bool ok = true;
    ok = ok && (ia[0] == 1);
    ok = ok && (ia[1] == 2);
    ok = ok && (ia[2] == 3);
    ok = ok && (ia[3] == 4);
    ok = ok && (res.out == ia + n);
    ok = ok && (res.fun.i_ == 5);
    return ok;
}

TEST(GenerateNTest, Constexpr) {
    static_assert(test_constexpr(), "");
}
