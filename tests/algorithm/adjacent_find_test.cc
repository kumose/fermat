#include <gtest/gtest.h>
#include <fermat/core.h>
#include <fermat/algorithm/adjacent_find.h>

namespace {
    // Helper for constexpr testing: use std::array
    template<typename T, std::size_t N>
    using test_array = std::array<T, N>;
} // namespace

TEST(AdjacentFindTest, Basic) {
    int v1[] = {0, 2, 2, 4, 6};
    EXPECT_EQ(fermat::ranges::adjacent_find(fermat::ranges::begin(v1), fermat::ranges::end(v1)), &v1[1]);
    EXPECT_EQ(fermat::ranges::adjacent_find(v1), &v1[1]);

    std::pair<int, int> v2[] = {{0, 0}, {0, 2}, {0, 2}, {0, 4}, {0, 6}};
    EXPECT_EQ(fermat::ranges::adjacent_find(fermat::ranges::begin(v2), fermat::ranges::end(v2),
                                    fermat::ranges::equal_to{}, &std::pair<int, int>::second),
              &v2[1]);
    EXPECT_EQ(fermat::ranges::adjacent_find(v2, fermat::ranges::equal_to{}, &std::pair<int, int>::second),
              &v2[1]);

    static_assert(std::is_same_v<std::pair<int,int>*,
                                 decltype(fermat::ranges::adjacent_find(v2, fermat::ranges::equal_to{},
                                      &std::pair<int, int>::second))>,
                  "");
}

TEST(AdjacentFindTest, Constexpr) {
    constexpr auto a1 = test_array<int, 5>{0, 2, 2, 4, 6};
    static_assert(fermat::ranges::adjacent_find(a1.begin(), a1.end()) == (a1.begin() + 1));
    static_assert(fermat::ranges::adjacent_find(a1) == (a1.begin() + 1));

    constexpr std::pair<int, int> a2[] = {{0, 0}, {0, 2}, {0, 2}, {0, 4}, {0, 6}};
    static_assert(fermat::ranges::adjacent_find(a2, fermat::ranges::equal_to{}) == (a2 + 1));
}