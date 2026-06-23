#include <gtest/gtest.h>
#include <array>
#include <initializer_list>
#include <fermat/core.h>
#include <fermat/algorithm/count.h>

struct S {
    int i;
};

TEST(CountTest, Basic) {
    using namespace fermat::ranges;

    int ia[] = {0, 1, 2, 2, 0, 1, 2, 3};
    constexpr auto cia = sizeof(ia) / sizeof(ia[0]);

    // 使用原始指针作为迭代器
    EXPECT_EQ(count(ia, ia + cia, 2), 3);
    EXPECT_EQ(count(ia, ia + cia, 7), 0);
    EXPECT_EQ(count(ia, ia, 2), 0);

    // 使用子范围
    EXPECT_EQ(count(make_subrange(ia, ia + cia), 2), 3);
    EXPECT_EQ(count(make_subrange(ia, ia + cia), 7), 0);
    EXPECT_EQ(count(make_subrange(ia, ia), 2), 0);
}

TEST(CountTest, WithProjection) {
    using namespace fermat::ranges;

    S sa[] = {{0}, {1}, {2}, {2}, {0}, {1}, {2}, {3}};
    constexpr auto csa = sizeof(sa) / sizeof(sa[0]);

    EXPECT_EQ(count(sa, sa + csa, 2, &S::i), 3);
    EXPECT_EQ(count(sa, sa + csa, 7, &S::i), 0);
    EXPECT_EQ(count(sa, sa, 2, &S::i), 0);

    EXPECT_EQ(count(make_subrange(sa, sa + csa), 2, &S::i), 3);
    EXPECT_EQ(count(make_subrange(sa, sa + csa), 7, &S::i), 0);
    EXPECT_EQ(count(make_subrange(sa, sa), 2, &S::i), 0);
}

TEST(CountTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    static_assert(fermat::ranges::count(IL{0, 1, 2, 1, 3, 1, 4}, 1) == 3, "");
    static_assert(fermat::ranges::count(IL{0, 1, 2, 1, 3, 1, 4}, 5) == 0, "");
}