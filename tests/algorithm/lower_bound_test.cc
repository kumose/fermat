#include <gtest/gtest.h>
#include <vector>
#include <utility>
#include <fermat/algorithm/lower_bound.h>
#include <fermat/view/all.h>

struct my_int {
    int value;
};

bool compare(my_int lhs, my_int rhs) {
    return lhs.value < rhs.value;
}

// Compile‑time test that this overload is callable (no runtime check)
void not_totally_ordered() {
    std::vector<my_int> vec;
    ranges::lower_bound(vec, my_int{10}, compare);
}

TEST(LowerBoundTest, Basic) {
    using namespace ranges;
    constexpr std::pair<int, int> a[] = {{0,0},{0,1},{1,2},{1,3},{3,4},{3,5}};
    constexpr const std::pair<int, int> c[] = {{0,0},{0,1},{1,2},{1,3},{3,4},{3,5}};

    // lower_bound with iterator pairs
    EXPECT_EQ(lower_bound(begin(a), end(a), a[0]), &a[0]);
    EXPECT_EQ(lower_bound(begin(a), end(a), a[1], less{}), &a[1]);
    EXPECT_EQ(lower_bound(begin(a), end(a), 1, less{}, &std::pair<int,int>::first), &a[2]);

    // lower_bound with range
    EXPECT_EQ(lower_bound(a, a[2]), &a[2]);
    EXPECT_EQ(lower_bound(c, c[3]), &c[3]);

    EXPECT_EQ(lower_bound(a, a[4], less{}), &a[4]);
    EXPECT_EQ(lower_bound(c, c[5], less{}), &c[5]);

    // projection
    EXPECT_EQ(lower_bound(a, 1, less{}, &std::pair<int,int>::first), &a[2]);
    EXPECT_EQ(lower_bound(c, 1, less{}, &std::pair<int,int>::first), &c[2]);

    // view
    EXPECT_EQ(lower_bound(views::all(a), 1, less{}, &std::pair<int,int>::first), &a[2]);
    EXPECT_EQ(lower_bound(views::all(c), 1, less{}, &std::pair<int,int>::first), &c[2]);
}

TEST(LowerBoundTest, Constexpr) {
    using namespace ranges;
    constexpr std::pair<int,int> a[] = {{0,0},{0,1},{1,2},{1,3},{3,4},{3,5}};
    constexpr std::size_t N = sizeof(a)/sizeof(a[0]);
    // avoid unused variable warning
    (void)N;

    static_assert(lower_bound(begin(a), end(a), a[0]) == &a[0], "");
    static_assert(lower_bound(begin(a), end(a), a[1], less{}) == &a[1], "");
    static_assert(lower_bound(a, a[2]) == &a[2], "");
    static_assert(lower_bound(a, a[4], less{}) == &a[4], "");
    static_assert(lower_bound(a, std::make_pair(1,2), less{}) == &a[2], "");
#if RANGES_CXX_CONSTEXPR >= 17
    // requires constexpr std::addressof
    static_assert(lower_bound(views::all(a), std::make_pair(1,2), less{}) == &a[2], "");
#endif
}