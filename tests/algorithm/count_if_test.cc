#include <gtest/gtest.h>
#include <initializer_list>
#include <functional>
#include <fermat/core.h>
#include <fermat/algorithm/count_if.h>

struct S {
    int i;
};

struct T {
    bool b;
    bool m() const { return b; }
};

static constexpr bool even(int i) { return i % 2 == 0; }

TEST(CountIfTest, Basic) {
    using namespace fermat::ranges;

    int ia[] = {0, 1, 2, 2, 0, 1, 2, 3};
    constexpr std::size_t cia = sizeof(ia) / sizeof(ia[0]);

    // With iterators
    EXPECT_EQ(count_if(ia, ia + cia, [](int i) { return i == 2; }), 3);
    EXPECT_EQ(count_if(ia, ia + cia, [](int i) { return i == 7; }), 0);
    EXPECT_EQ(count_if(ia, ia, [](int i) { return i == 2; }), 0);

    // With subrange
    EXPECT_EQ(count_if(make_subrange(ia, ia + cia), [](int i) { return i == 2; }), 3);
    EXPECT_EQ(count_if(make_subrange(ia, ia + cia), [](int i) { return i == 7; }), 0);
    EXPECT_EQ(count_if(make_subrange(ia, ia), [](int i) { return i == 2; }), 0);
}

TEST(CountIfTest, WithProjection) {
    using namespace fermat::ranges;

    S sa[] = {{0}, {1}, {2}, {2}, {0}, {1}, {2}, {3}};
    constexpr std::size_t csa = sizeof(sa) / sizeof(sa[0]);

    EXPECT_EQ(count_if(sa, sa + csa, [](int i) { return i == 2; }, &S::i), 3);
    EXPECT_EQ(count_if(sa, sa + csa, [](int i) { return i == 7; }, &S::i), 0);
    EXPECT_EQ(count_if(sa, sa, [](int i) { return i == 2; }, &S::i), 0);

    EXPECT_EQ(count_if(make_subrange(sa, sa + csa), [](int i) { return i == 2; }, &S::i), 3);
    EXPECT_EQ(count_if(make_subrange(sa, sa + csa), [](int i) { return i == 7; }, &S::i), 0);
    EXPECT_EQ(count_if(make_subrange(sa, sa), [](int i) { return i == 2; }, &S::i), 0);
}

TEST(CountIfTest, MemberFunctionPointer) {
    using namespace fermat::ranges;

    T ta[] = {{true}, {false}, {true}, {false}, {false}, {true}, {false}, {false}, {true}, {false}};
    constexpr std::size_t n = sizeof(ta) / sizeof(ta[0]);

    EXPECT_EQ(count_if(ta, ta + n, &T::m), 4);
    EXPECT_EQ(count_if(ta, ta + n, &T::b), 4);
    EXPECT_EQ(count_if(make_subrange(ta, ta + n), &T::m), 4);
    EXPECT_EQ(count_if(make_subrange(ta, ta + n), &T::b), 4);
}

TEST(CountIfTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    static_assert(fermat::ranges::count_if(IL{0, 1, 2, 1, 3, 1, 4}, even) == 3, "");
    static_assert(fermat::ranges::count_if(IL{1, 1, 3, 1}, even) == 0, "");
}
