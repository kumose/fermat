#include <gtest/gtest.h>
#include <vector>
#include <initializer_list>
#include <fermat/algorithm/find_if.h>
#include <fermat/view/all.h>

struct S {
    int i_;
};

constexpr bool is_three(int i) { return i == 3; }

template<class Rng>
constexpr bool contains_three(Rng r) {
    auto it = ranges::find_if(r, is_three);
    return it != ranges::end(r);
}

TEST(FindIfTest, IteratorPairs) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr auto s = sizeof(ia) / sizeof(ia[0]);

    // find existing element
    auto it = find_if(ia, ia + s, [](int i) { return i == 3; });
    EXPECT_EQ(*it, 3);
    // find non-existing element
    it = find_if(ia, ia + s, [](int i) { return i == 10; });
    EXPECT_EQ(it, ia + s);

    // with sentinel (same as iterator pair)
    it = find_if(ia, ia + s, [](int i) { return i == 3; });
    EXPECT_EQ(*it, 3);
    it = find_if(ia, ia + s, [](int i) { return i == 10; });
    EXPECT_EQ(it, ia + s);
}

TEST(FindIfTest, Range) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr auto s = sizeof(ia) / sizeof(ia[0]);

    auto pi = find_if(ia, [](int i) { return i == 3; });
    EXPECT_EQ(*pi, 3);
    pi = find_if(ia, [](int i) { return i == 10; });
    EXPECT_EQ(pi, ia + s);
}

TEST(FindIfTest, RvalueContainer) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    std::vector<int> const vec(std::begin(ia), std::end(ia));
    // For rvalue container, result is dangling; we just check that it compiles.
    auto pj0 = find_if(std::move(vec), [](int i) { return i == 3; });
    auto pj1 = find_if(std::move(vec), [](int i) { return i == 10; });
    SUCCEED();
}

TEST(FindIfTest, WithProjection) {
    using namespace ranges;

    S sa[] = {{0}, {1}, {2}, {3}, {4}, {5}};
    S* ps = find_if(sa, [](int i) { return i == 3; }, &S::i_);
    EXPECT_EQ(ps->i_, 3);
    ps = find_if(sa, [](int i) { return i == 10; }, &S::i_);
    EXPECT_EQ(ps, std::end(sa));
}

TEST(FindIfTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    static_assert(contains_three(IL{0, 1, 2, 3}), "");
    static_assert(!contains_three(IL{0, 1, 2}), "");
}
