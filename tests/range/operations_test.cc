#include <forward_list>
#include <list>
#include <vector>
#include <limits>
#include <gtest/gtest.h>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/take_while.h>
#include <array>

namespace {
    template<typename I, typename S>
    void test_iterators(I first, S last, fermat::ranges::iter_difference_t<I> n) {
        using namespace fermat::ranges;
        EXPECT_EQ(distance(first, last), n);
        EXPECT_EQ(distance_compare(first, last, n), 0);
        EXPECT_GT(distance_compare(first, last, n - 1), 0);
        EXPECT_LT(distance_compare(first, last, n + 1), 0);
        EXPECT_GT(distance_compare(first, last, (std::numeric_limits<iter_difference_t<I>>::min)()), 0);
        EXPECT_LT(distance_compare(first, last, (std::numeric_limits<iter_difference_t<I>>::max)()), 0);
    }

    template<typename Rng>
    void test_range(Rng&& rng, fermat::ranges::range_difference_t<Rng> n) {
        using namespace fermat::ranges;
        EXPECT_EQ(distance(rng), n);
        EXPECT_EQ(distance_compare(rng, n), 0);
        EXPECT_GT(distance_compare(rng, n - 1), 0);
        EXPECT_LT(distance_compare(rng, n + 1), 0);
        EXPECT_GT(distance_compare(rng, (std::numeric_limits<range_difference_t<Rng>>::min)()), 0);
        EXPECT_LT(distance_compare(rng, (std::numeric_limits<range_difference_t<Rng>>::max)()), 0);
    }

    template<typename Rng>
    void test_infinite_range(Rng&& rng) {
        using namespace fermat::ranges;
        EXPECT_GT(distance_compare(rng, 0), 0);
        EXPECT_GT(distance_compare(rng, -1), 0);
        EXPECT_GT(distance_compare(rng, 1), 0);
        EXPECT_GT(distance_compare(rng, (std::numeric_limits<range_difference_t<Rng>>::min)()), 0);
        if (is_infinite<Rng>::value) {
            EXPECT_GT(distance_compare(rng, (std::numeric_limits<range_difference_t<Rng>>::max)()), 0);
        }
    }
    void test_constexpr_runtime() {
        using namespace fermat::ranges;
        auto rng = std::array<int, 10>{1,2,3,4,5,6,7,8,9,10};
        using Rng = decltype(rng);
        auto bit = fermat::ranges::begin(rng);
        using I = decltype(bit);
        auto it = bit + 5;
        auto eit = fermat::ranges::end(rng);
        auto n = fermat::ranges::distance(rng);
        auto en = fermat::ranges::enumerate(rng);

        EXPECT_EQ(n, 10);
        EXPECT_EQ(distance(bit, eit), n);
        EXPECT_EQ(distance(it, eit), 5);
        EXPECT_EQ(distance_compare(bit, eit, n), 0);
        EXPECT_GT(distance_compare(bit, eit, n - 1), 0);
        EXPECT_LT(distance_compare(bit, eit, n + 1), 0);
        EXPECT_GT(distance_compare(bit, eit, (std::numeric_limits<iter_difference_t<I>>::min)()), 0);
        EXPECT_LT(distance_compare(bit, eit, (std::numeric_limits<iter_difference_t<I>>::max)()), 0);
        EXPECT_EQ(distance(rng), n);
        EXPECT_EQ(distance_compare(rng, n), 0);
        EXPECT_GT(distance_compare(rng, n - 1), 0);
        EXPECT_LT(distance_compare(rng, n + 1), 0);
        EXPECT_GT(distance_compare(rng, (std::numeric_limits<range_difference_t<Rng>>::min)()), 0);
        EXPECT_LT(distance_compare(rng, (std::numeric_limits<range_difference_t<Rng>>::max)()), 0);
        EXPECT_EQ(en.first, 10);
        EXPECT_EQ(en.second, eit);
    }
} // anonymous namespace

TEST(RangeDistanceTest, Vector) {
    using cont_t = std::vector<int>;
    cont_t c{1, 2, 3, 4};
    test_range(c, 4);
    test_iterators(c.begin(), c.end(), 4);
    c.clear();
    test_range(c, 0);
    test_iterators(c.begin(), c.end(), 0);
}

TEST(RangeDistanceTest, List) {
    using cont_t = std::list<int>;
    cont_t c{1, 2, 3, 4};
    test_range(c, 4);
    test_iterators(c.begin(), c.end(), 4);
    c.clear();
    test_range(c, 0);
    test_iterators(c.begin(), c.end(), 0);
}

TEST(RangeDistanceTest, ForwardList) {
    using cont_t = std::forward_list<int>;
    cont_t c{1, 2, 3, 4};
    test_range(c, 4);
    test_iterators(c.begin(), c.end(), 4);
    c.clear();
    test_range(c, 0);
    test_iterators(c.begin(), c.end(), 0);
}

TEST(RangeDistanceTest, Array) {
    int a[] = {1, 2, 3, 4};
    test_iterators(a + 4, a, -4);
}

TEST(RangeDistanceTest, TakeWhileView) {
    using namespace fermat::ranges;
    auto rng = views::iota(0) | views::take_while([](int i) { return i < 4; });
    test_range(rng, 4);
}

TEST(RangeDistanceTest, InfiniteIota) {
    using namespace fermat::ranges;
    test_infinite_range(views::iota(0u));
}

TEST(RangeDistanceTest, ConstexprRuntime) {
    test_constexpr_runtime();
}