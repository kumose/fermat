#include <gtest/gtest.h>
#include <array>
#include <initializer_list>
#include <fermat/algorithm/contains_subrange.h>
#include <fermat/iterator/operations.h>
#include <fermat/view/subrange.h>

using namespace fermat::ranges;

namespace {
    int comparison_count = 0;

    template<typename T>
    bool counting_equals(const T& a, const T& b) {
        ++comparison_count;
        return a == b;
    }
} // namespace

TEST(ContainsSubrangeTest, Basic) {
    std::array<int, 10> full{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::array<int, 5> valid{2, 3, 4, 5, 6};
    std::array<int, 5> invalid{3, 4, 5, 6, 2};

    // Iterators
    EXPECT_TRUE(contains_subrange(full.begin(), full.end(), valid.begin(), valid.end()));
    EXPECT_FALSE(contains_subrange(full.begin(), full.end(), invalid.begin(), invalid.end()));

    // Ranges
    EXPECT_FALSE(contains_subrange(full, invalid));
    EXPECT_TRUE(contains_subrange(make_subrange(full.begin(), full.end()),
                                  make_subrange(valid.begin(), valid.end())));

    // Empty needle
    EXPECT_TRUE(contains_subrange(full.begin(), full.end(), full.begin(), full.begin()));
    EXPECT_TRUE(contains_subrange(full, make_subrange(full.begin(), full.begin())));
}

TEST(ContainsSubrangeTest, WithPredicate) {
    std::array<int, 10> full{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::array<int, 5> valid{2, 3, 4, 5, 6};
    std::array<int, 5> invalid{3, 4, 5, 6, 2};

    comparison_count = 0;
    EXPECT_FALSE(contains_subrange(valid.begin(), valid.end(),
                                   full.begin(), full.end(),
                                   counting_equals<int>));
    EXPECT_EQ(comparison_count, 0);

    comparison_count = 0;
    EXPECT_TRUE(contains_subrange(full.begin(), full.end(),
                                  valid.begin(), valid.end(),
                                  counting_equals<int>));
    EXPECT_GT(comparison_count, 0);

    comparison_count = 0;
    EXPECT_TRUE(contains_subrange(make_subrange(full.begin(), full.end()),
                                  make_subrange(valid.begin(), valid.end()),
                                  counting_equals<int>));
    EXPECT_GT(comparison_count, 0);

    comparison_count = 0;
    EXPECT_FALSE(contains_subrange(make_subrange(full.begin(), full.begin()),
                                   make_subrange(valid.begin(), valid.end()),
                                   counting_equals<int>));
    EXPECT_EQ(comparison_count, 0);
}

TEST(ContainsSubrangeTest, Constexpr) {
#if RANGES_CXX_CONSTEXPR >= RANGES_CXX_CONSTEXPR_14 && RANGES_CONSTEXPR_INVOKE
    using IL = std::initializer_list<int>;
    static_assert(contains_subrange(IL{0, 1, 2, 3, 4}, IL{3, 4}), "");
    static_assert(!contains_subrange(IL{0, 1, 2, 3, 4}, IL{2, 8}), "");
    static_assert(contains_subrange(IL{}, IL{}), "");
#endif
}