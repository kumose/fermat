#include <gtest/gtest.h>
#include <vector>
#include <iterator>
#include <fermat/algorithm/equal_range.h>
#include <fermat/functional/identity.h>
#include <fermat/view/subrange.h>

struct my_int {
    int value;
};

bool compare(my_int lhs, my_int rhs) {
    return lhs.value < rhs.value;
}

// compile-time test for not totally ordered (should compile)
void not_totally_ordered() {
    std::vector<my_int> vec;
    ranges::equal_range(vec, my_int{10}, compare);
}

template<class Iter, class Sent, class T, class Proj = ranges::identity>
void test_it(Iter first, Sent last, const T& value, Proj proj = Proj{}) {
    auto i = ranges::equal_range(first, last, value, ranges::less{}, proj);
    for (Iter j = first; j != i.begin(); ++j)
        EXPECT_LT(ranges::invoke(proj, *j), value);
    for (Iter j = i.begin(); j != last; ++j)
        EXPECT_FALSE(ranges::invoke(proj, *j) < value);
    for (Iter j = first; j != i.end(); ++j)
        EXPECT_FALSE(value < ranges::invoke(proj, *j));
    for (Iter j = i.end(); j != last; ++j)
        EXPECT_LT(value, ranges::invoke(proj, *j));

    auto res = ranges::equal_range(ranges::make_subrange(first, last), value, ranges::less{}, proj);
    for (Iter j = first; j != res.begin(); ++j)
        EXPECT_LT(ranges::invoke(proj, *j), value);
    for (Iter j = res.begin(); j != last; ++j)
        EXPECT_FALSE(ranges::invoke(proj, *j) < value);
    for (Iter j = first; j != res.end(); ++j)
        EXPECT_FALSE(value < ranges::invoke(proj, *j));
    for (Iter j = res.end(); j != last; ++j)
        EXPECT_LT(value, ranges::invoke(proj, *j));
}

TEST(EqualRangeTest, Basic) {
    int d[] = {0, 1, 2, 3};
    for (int* e = d; e <= d + 4; ++e)
        for (int x = -1; x <= 4; ++x)
            test_it(d, e, x);
}

TEST(EqualRangeTest, Projection) {
    struct foo { int i; };
    foo some_foos[] = {{1}, {2}, {4}};
    test_it(some_foos, some_foos + 3, 2, &foo::i);
}

TEST(EqualRangeTest, Constexpr) {
    constexpr auto test = []() constexpr {
        constexpr int d[] = {0, 1, 2, 3};
        auto res = ranges::equal_range(d, 2);
        bool ok = (res.begin() == d + 2) && (res.end() == d + 3);
        return ok;
    };
    static_assert(test(), "");
}