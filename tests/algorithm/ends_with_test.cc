#include <gtest/gtest.h>
#include <initializer_list>
#include <iterator>
#include <fermat/algorithm/ends_with.h>
#include <fermat/view/subrange.h>

namespace {
    int comparison_count = 0;

    template<typename T>
    bool counting_equals(const T& a, const T& b) {
        ++comparison_count;
        return a == b;
    }
} // namespace

TEST(EndsWithTest, Basic) {
    using namespace fermat::ranges;

    int ia[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int ib[] = {5, 6, 7, 8, 9};

    // RandomAccessIterator (plain pointers)
    EXPECT_TRUE(ends_with(ia, ia + 10, ib, ib + 5));
    EXPECT_FALSE(ends_with(ia, ia + 10, ib, ib + 4));
    EXPECT_FALSE(ends_with(ia, ia + 10, ib, ib + 5, std::not_equal_to<>{})); // not equal should fail

    // Subrange versions
    EXPECT_TRUE(ends_with(make_subrange(ia, ia + 10), make_subrange(ib, ib + 5)));
    EXPECT_FALSE(ends_with(make_subrange(ia, ia + 10), make_subrange(ib, ib + 4)));
    EXPECT_TRUE(ends_with(make_subrange(ia, ia + 10), make_subrange(ib, ib + 5), std::equal_to<>{}));
    EXPECT_FALSE(ends_with(make_subrange(ia, ia + 10), make_subrange(ib, ib + 5), std::not_equal_to<>{}));

    // Empty needle
    EXPECT_TRUE(ends_with(ia, ia + 10, ib, ib));           // empty range
    EXPECT_TRUE(ends_with(ia, ia, ib, ib));               // both empty
    EXPECT_FALSE(ends_with(ia, ia, ib, ib + 5));          // haystack empty, needle not
}

TEST(EndsWithTest, WithPredicateAndCount) {
    using namespace fermat::ranges;

    int ia[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int ib[] = {5, 6, 7, 8, 9};

    comparison_count = 0;
    EXPECT_FALSE(ends_with(ib, ib + 5, ia, ia + 10, counting_equals<int>));
    EXPECT_EQ(comparison_count, 0);   // needle longer than haystack -> immediate false

    comparison_count = 0;
    EXPECT_TRUE(ends_with(ia, ia + 10, ib, ib + 5, counting_equals<int>));
    EXPECT_GT(comparison_count, 0);

    comparison_count = 0;
    EXPECT_FALSE(ends_with(ia, ia + 10, ib, ib + 4, counting_equals<int>));
    EXPECT_GT(comparison_count, 0);
}

TEST(EndsWithTest, Constexpr) {
#if RANGES_CXX_CONSTEXPR >= RANGES_CXX_CONSTEXPR_14 && RANGES_CONSTEXPR_INVOKE
    using IL = std::initializer_list<int>;
    static_assert(fermat::ranges::ends_with(IL{0, 1, 2, 3, 4}, IL{3, 4}), "");
    static_assert(!fermat::ranges::ends_with(IL{0, 1, 2, 3, 4}, IL{2, 3}), "");
    static_assert(fermat::ranges::ends_with(IL{0, 1, 2, 3, 4}, IL{}), "");
    static_assert(fermat::ranges::ends_with(IL{}, IL{}), "");
#endif
}