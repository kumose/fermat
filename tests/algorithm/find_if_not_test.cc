#include <gtest/gtest.h>
#include <initializer_list>
#include <fermat/algorithm/find_if_not.h>

constexpr bool is_three(int i) { return i == 3; }

template<class Rng>
constexpr bool contains_other_than_three(Rng r) {
    auto it = ranges::find_if_not(r, is_three);
    return it != ranges::end(r);
}

TEST(FindIfNotTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    static_assert(contains_other_than_three(IL{3, 3, 2, 3}), "");
    static_assert(!contains_other_than_three(IL{3, 3, 3}), "");
}