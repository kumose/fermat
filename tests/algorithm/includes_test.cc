#include <gtest/gtest.h>
#include <fermat/algorithm/set_algorithm.h>

TEST(IncludesTest, Basic) {
    using namespace fermat::ranges;

    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    const unsigned sa = sizeof(ia)/sizeof(ia[0]);
    int ib[] = {2, 4};
    int ic[] = {1, 2};
    int id[] = {3, 3, 3, 3};

    // Empty sequences
    EXPECT_TRUE(includes(ia, ia, ib, ib));
    EXPECT_FALSE(includes(ia, ia, ib, ib+1));
    EXPECT_TRUE(includes(ia, ia+1, ib, ib));
    EXPECT_TRUE(includes(ia, ia+sa, ia, ia+sa));

    // Normal cases
    EXPECT_TRUE(includes(ia, ia+sa, ib, ib+2));
    EXPECT_FALSE(includes(ib, ib+2, ia, ia+sa));

    EXPECT_TRUE(includes(ia, ia+2, ic, ic+2));
    EXPECT_FALSE(includes(ia, ia+2, ib, ib+2));

    EXPECT_TRUE(includes(ia, ia+sa, id, id+1));
    EXPECT_TRUE(includes(ia, ia+sa, id, id+2));
    EXPECT_TRUE(includes(ia, ia+sa, id, id+3));
    EXPECT_FALSE(includes(ia, ia+sa, id, id+4));
}

TEST(IncludesTest, WithComparator) {
    using namespace fermat::ranges;

    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    const unsigned sa = sizeof(ia)/sizeof(ia[0]);
    int ib[] = {2, 4};
    int ic[] = {1, 2};
    int id[] = {3, 3, 3, 3};

    EXPECT_TRUE(includes(ia, ia, ib, ib, std::less<int>()));
    EXPECT_FALSE(includes(ia, ia, ib, ib+1, std::less<int>()));
    EXPECT_TRUE(includes(ia, ia+1, ib, ib, std::less<int>()));
    EXPECT_TRUE(includes(ia, ia+sa, ia, ia+sa, std::less<int>()));

    EXPECT_TRUE(includes(ia, ia+sa, ib, ib+2, std::less<int>()));
    EXPECT_FALSE(includes(ib, ib+2, ia, ia+sa, std::less<int>()));

    EXPECT_TRUE(includes(ia, ia+2, ic, ic+2, std::less<int>()));
    EXPECT_FALSE(includes(ia, ia+2, ib, ib+2, std::less<int>()));

    EXPECT_TRUE(includes(ia, ia+sa, id, id+1, std::less<int>()));
    EXPECT_TRUE(includes(ia, ia+sa, id, id+2, std::less<int>()));
    EXPECT_TRUE(includes(ia, ia+sa, id, id+3, std::less<int>()));
    EXPECT_FALSE(includes(ia, ia+sa, id, id+4, std::less<int>()));
}

TEST(IncludesTest, WithProjection) {
    using namespace fermat::ranges;

    struct S { int i; };
    struct T { int j; };
    S ia[] = {{1}, {2}, {2}, {3}, {3}, {3}, {4}, {4}, {4}, {4}};
    T id[] = {{3}, {3}, {3}};

    EXPECT_TRUE(includes(ia, id, std::less<int>(), &S::i, &T::j));
}

TEST(IncludesTest, Constexpr) {
    using IL = std::initializer_list<int>;
    static_assert(fermat::ranges::includes(IL{1, 2, 2, 3, 3, 3, 4, 4, 4, 4}, IL{3, 3, 3}, std::less<int>()), "");
}