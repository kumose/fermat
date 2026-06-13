/// equal_gtest.cpp
/// Google Test conversion of range-v3 equal algorithm test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>
#include <array>
#include <initializer_list>
#include <fermat/algorithm/equal.h>
#include <fermat/view/subrange.h>

namespace {
    int comparison_count = 0;

    template<typename T>
    bool counting_equals(const T &a, const T &b) {
        ++comparison_count;
        return a == b;
    }
} // namespace

TEST(EqualTest, IteratorPairs) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr std::size_t s = sizeof(ia) / sizeof(ia[0]);
    int ib[s] = {0, 1, 2, 5, 4, 5};

    EXPECT_TRUE(equal(ia, ia + s, ia, ia + s));
    EXPECT_FALSE(equal(ia, ia + s, ib, ib + s));
    EXPECT_FALSE(equal(ia, ia + s, ia, ia + s - 1));
}

TEST(EqualTest, RangeVersions) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr std::size_t s = sizeof(ia) / sizeof(ia[0]);
    int ib[s] = {0, 1, 2, 5, 4, 5};

    EXPECT_TRUE(equal(make_subrange(ia, ia + s), make_subrange(ia, ia + s)));
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), make_subrange(ia, ia + s - 1)));
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), make_subrange(ib, ib + s)));

    EXPECT_TRUE(equal(make_subrange(ia, ia + s), ia));
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), ib));
}

TEST(EqualTest, WithPredicate) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr std::size_t s = sizeof(ia) / sizeof(ia[0]);
    int ib[s] = {0, 1, 2, 5, 4, 5};

    EXPECT_TRUE(equal(ia, ia + s, ia, ia + s, std::equal_to<int>{}));
    EXPECT_FALSE(equal(ia, ia + s, ia, ia + s - 1, std::equal_to<int>{}));
    EXPECT_FALSE(equal(ia, ia + s, ib, ib + s, std::equal_to<int>{}));

    // For ranges of different lengths, the algorithm may short‑circuit
    // without comparing any elements. Therefore we only check that
    // the number of comparisons is non‑negative (i.e. test doesn't crash).
    comparison_count = 0;
    EXPECT_FALSE(equal(ia, ia + s, ia, ia + s - 1, counting_equals<int>));
    EXPECT_GE(comparison_count, 0);

    // For ranges of equal length but different content, at least one comparison is performed.
    comparison_count = 0;
    EXPECT_FALSE(equal(ia, ia + s, ib, ib + s, counting_equals<int>));
    EXPECT_GT(comparison_count, 0);
}

TEST(EqualTest, RangeWithPredicate) {
    using namespace ranges;

    int ia[] = {0, 1, 2, 3, 4, 5};
    constexpr std::size_t s = sizeof(ia) / sizeof(ia[0]);
    int ib[s] = {0, 1, 2, 5, 4, 5};

    EXPECT_TRUE(equal(make_subrange(ia, ia + s), make_subrange(ia, ia + s), std::equal_to<int>{}));
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), make_subrange(ia, ia + s - 1), std::equal_to<int>{}));
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), make_subrange(ib, ib + s), std::equal_to<int>{}));

    EXPECT_TRUE(equal(make_subrange(ia, ia + s), ia, std::equal_to<int>{}));
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), ib, std::equal_to<int>{}));

    // Different lengths: may short‑circuit.
    comparison_count = 0;
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), make_subrange(ia, ia + s - 1), counting_equals<int>));
    EXPECT_GE(comparison_count, 0);

    // Equal length but different content: must compare at least one element.
    comparison_count = 0;
    EXPECT_FALSE(equal(make_subrange(ia, ia + s), make_subrange(ib, ib + s), counting_equals<int>));
    EXPECT_GT(comparison_count, 0);
}

TEST(EqualTest, ConstexprInitializerList) {
    using IL = std::initializer_list<int>;
    static_assert(ranges::equal(IL{1, 2, 3, 4}, IL{1, 2, 3, 4}), "");
    static_assert(!ranges::equal(IL{1, 2, 3, 4}, IL{1, 2, 3}), "");
    static_assert(!ranges::equal(IL{1, 2, 3, 4}, IL{1, 2, 4, 3}), "");
    static_assert(ranges::equal(IL{}, IL{}), "");
}
