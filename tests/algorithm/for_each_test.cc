#include <gtest/gtest.h>
#include <vector>
#include <fermat/algorithm/for_each.h>
#include <fermat/view/all.h>

struct S {
    void p() const { *p_ += i_; }
    int* p_;
    int i_;
};

constexpr int void_f(int const&) { return 3; }

TEST(ForEachTest, Basic) {
    int sum = 0;
    auto fun = [&](int i) { sum += i; };
    std::vector<int> v1{0, 2, 4, 6};

    auto res1 = ranges::for_each(v1.begin(), v1.end(), fun);
    EXPECT_EQ(res1.in, v1.end());
    EXPECT_EQ(sum, 12);

    sum = 0;
    auto res2 = ranges::for_each(v1, fun);
    EXPECT_EQ(res2.in, v1.end());
    EXPECT_EQ(sum, 12);

    sum = 0;
    auto rfun = [&](int& i) { sum += i; };
    auto res3 = ranges::for_each(v1.begin(), v1.end(), rfun);
    EXPECT_EQ(res3.in, v1.end());
    EXPECT_EQ(sum, 12);

    sum = 0;
    auto res4 = ranges::for_each(v1, rfun);
    EXPECT_EQ(res4.in, v1.end());
    EXPECT_EQ(sum, 12);
}

TEST(ForEachTest, MemberFunctionPointer) {
    int sum = 0;
    std::vector<S> v2{{&sum, 0}, {&sum, 2}, {&sum, 4}, {&sum, 6}};

    auto res1 = ranges::for_each(v2.begin(), v2.end(), &S::p);
    EXPECT_EQ(res1.in, v2.end());
    EXPECT_EQ(sum, 12);

    sum = 0;
    auto res2 = ranges::for_each(v2, &S::p);
    EXPECT_EQ(res2.in, v2.end());
    EXPECT_EQ(sum, 12);
}

TEST(ForEachTest, RvalueRange) {
    int sum = 0;
    auto fun = [&](int i) { sum += i; };
    std::vector<int> v1{0, 2, 4, 6};

    auto res = ranges::for_each(std::move(v1), fun);
    // res.in may be dangling; we only check compilation.
    SUCCEED();
}

TEST(ForEachTest, ConstexprRuntime) {
    constexpr std::array<int, 4> rng{0, 2, 4, 6};
    auto res = ranges::for_each(rng, void_f);
    EXPECT_EQ(res.in, rng.end());
}