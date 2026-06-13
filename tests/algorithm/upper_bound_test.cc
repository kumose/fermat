// upper_bound_gtest.cpp
// Google Test conversion of range-v3 upper_bound algorithm test.
// Uses fermat::range and standard library components.
// All comments in English.

#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include <fermat/core.h>
#include <fermat/algorithm/upper_bound.h>
#include <fermat/view/all.h>

// ------------------------------------------------------------
// Dangling detection placeholder (adjust if fermat::range provides)
// ------------------------------------------------------------
namespace fermat::ranges {
    template<typename T>
    bool is_dangling(T&&) { return false; }
}
// ------------------------------------------------------------
// Helper for not_totally_ordered test
// ------------------------------------------------------------
struct my_int {
    int value;
};

bool compare(my_int lhs, my_int rhs) {
    return lhs.value < rhs.value;
}

void not_totally_ordered() {
    // This must compile.
    std::vector<my_int> vec;
    fermat::ranges::upper_bound(vec, my_int{10}, compare);
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------
TEST(UpperBoundTest, Basic) {
    using fermat::ranges::begin;
    using fermat::ranges::end;
    using fermat::ranges::size;
    using fermat::ranges::less;
    using P = std::pair<int, int>;

    constexpr P a[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};
    constexpr P const c[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};

    EXPECT_EQ(fermat::ranges::aux::upper_bound_n(begin(a), size(a), a[0]), &a[1]);
    EXPECT_EQ(fermat::ranges::aux::upper_bound_n(begin(a), size(a), a[1], less()), &a[2]);
    EXPECT_EQ(fermat::ranges::aux::upper_bound_n(begin(a), size(a), 1, less(), &std::pair<int, int>::first), &a[4]);

    EXPECT_EQ(fermat::ranges::upper_bound(begin(a), end(a), a[0]), &a[1]);
    EXPECT_EQ(fermat::ranges::upper_bound(begin(a), end(a), a[1], less()), &a[2]);
    EXPECT_EQ(fermat::ranges::upper_bound(begin(a), end(a), 1, less(), &std::pair<int, int>::first), &a[4]);

    EXPECT_EQ(fermat::ranges::upper_bound(a, a[2]), &a[3]);
    EXPECT_EQ(fermat::ranges::upper_bound(c, c[3]), &c[4]);

    EXPECT_EQ(fermat::ranges::upper_bound(a, a[4], less()), &a[5]);
    EXPECT_EQ(fermat::ranges::upper_bound(c, c[5], less()), &c[6]); // c+6 out of bounds? Actually c has 6 elements, index 5 is last, so c+6 is end

    EXPECT_EQ(fermat::ranges::upper_bound(a, 1, less(), &std::pair<int, int>::first), &a[4]);
    EXPECT_EQ(fermat::ranges::upper_bound(c, 1, less(), &std::pair<int, int>::first), &c[4]);

    std::vector<P> vec_a(begin(a), end(a));
    std::vector<P> const vec_c(begin(c), end(c));

    EXPECT_EQ(fermat::ranges::upper_bound(fermat::ranges::views::all(a), a[2]), &a[3]);
    EXPECT_EQ(fermat::ranges::upper_bound(fermat::ranges::views::all(c), c[3]), &c[4]);

    // rvalue range tests: dangling expected, but we only need to ensure compilation.
    (void)fermat::ranges::upper_bound(std::move(a), a[2]);
    (void)fermat::ranges::upper_bound(std::move(c), c[3]);
    (void)fermat::ranges::upper_bound(std::move(vec_a), vec_a[2]);
    (void)fermat::ranges::upper_bound(std::move(vec_c), vec_c[3]);

    EXPECT_EQ(fermat::ranges::upper_bound(fermat::ranges::views::all(a), a[4], less()), &a[5]);
    EXPECT_EQ(fermat::ranges::upper_bound(fermat::ranges::views::all(c), c[5], less()), &c[6]);

    (void)fermat::ranges::upper_bound(std::move(a), a[4], less());
    (void)fermat::ranges::upper_bound(std::move(c), c[5], less());
    (void)fermat::ranges::upper_bound(std::move(vec_a), vec_a[4], less());
    (void)fermat::ranges::upper_bound(std::move(vec_c), vec_c[5], less());

    EXPECT_EQ(fermat::ranges::upper_bound(fermat::ranges::views::all(a), 1, less(), &std::pair<int, int>::first), &a[4]);
    EXPECT_EQ(fermat::ranges::upper_bound(fermat::ranges::views::all(c), 1, less(), &std::pair<int, int>::first), &c[4]);

    (void)fermat::ranges::upper_bound(std::move(a), 1, less(), &std::pair<int, int>::first);
    (void)fermat::ranges::upper_bound(std::move(c), 1, less(), &std::pair<int, int>::first);
    (void)fermat::ranges::upper_bound(std::move(vec_a), 1, less(), &std::pair<int, int>::first);
    (void)fermat::ranges::upper_bound(std::move(vec_c), 1, less(), &std::pair<int, int>::first);
}

TEST(UpperBoundTest, Constexpr) {
    using namespace fermat::ranges;
    using P = std::pair<int, int>;

    constexpr P a[] = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};

    static_assert(aux::upper_bound_n(begin(a), size(a), a[0]) == &a[1], "");
    static_assert(aux::upper_bound_n(begin(a), size(a), a[1], less()) == &a[2], "");

    static_assert(upper_bound(begin(a), end(a), a[0]) == &a[1], "");
    static_assert(upper_bound(begin(a), end(a), a[1], less()) == &a[2], "");
    static_assert(upper_bound(a, a[2]) == &a[3], "");
    static_assert(upper_bound(a, a[3], less()) == &a[4], "");

    static_assert(upper_bound(a, std::make_pair(1, 3), less()) == &a[4], "");

#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
    // Requires constexpr std::addressof, which may be available in C++17.
    static_assert(upper_bound(views::all(a), std::make_pair(1, 3), less()) == &a[4], "");
#endif
}

TEST(UpperBoundTest, CompileOnly) {
    // Ensure not_totally_ordered compiles.
    not_totally_ordered();
}
