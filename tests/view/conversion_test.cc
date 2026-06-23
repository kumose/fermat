/// conversion_gtest.cpp
/// Google Test conversion of range-v3 conversion test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <map>
#include <set>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include <fermat/range/conversion.h>
#include <fermat/view/iota.h>
#include <fermat/view/take.h>
#include <fermat/view/reverse.h>
#include <fermat/view/repeat_n.h>
#include <fermat/view/transform.h>
#include <fermat/view/for_each.h>
#include <fermat/view/zip.h>
#include <fermat/view/drop.h>
#include <fermat/view/concat.h>
#include <fermat/view/any_view.h>

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list (single elements)
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for checking vector of vectors (direct equality)
template<typename T>
void check_equal(const std::vector<std::vector<T> > &actual,
                 const std::vector<std::vector<T> > &expected) {
    EXPECT_EQ(actual, expected);
}

/// Overload for checking vector of strings (direct equality)
inline void check_equal(const std::vector<std::string> &actual,
                        const std::vector<std::string> &expected) {
    EXPECT_EQ(actual, expected);
}

/// Overload for checking map (pairs)
template<typename K, typename V>
void check_equal(const std::map<K, V> &actual,
                 std::initializer_list<std::pair<const K, V> > expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(it->first, val.first);
        EXPECT_EQ(it->second, val.second);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ConversionTest, VectorFromIotaTake) {
    using namespace fermat::ranges;
    auto v = views::ints | views::take(10) | to<std::vector<int> >();
    check_equal(v, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

TEST(ConversionTest, VectorFromIotaReverse) {
    using namespace fermat::ranges;
    auto v = views::iota(10) | views::take(10) | views::reverse | to<std::vector<int> >();
    check_equal(v, {19, 18, 17, 16, 15, 14, 13, 12, 11, 10});
}

TEST(ConversionTest, ListFromIotaTake) {
    using namespace fermat::ranges;
    auto l = views::ints | views::take(10) | to<std::list<int> >();
    check_equal(l, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

TEST(ConversionTest, ListFromIotaReverse) {
    using namespace fermat::ranges;
    auto l = views::iota(10) | views::take(10) | views::reverse | to<std::list<int> >();
    check_equal(l, {19, 18, 17, 16, 15, 14, 13, 12, 11, 10});
}

TEST(ConversionTest, VectorOfVectors) {
    using namespace fermat::ranges;
    auto vv = views::repeat_n(views::ints(0, 8), 10) | to<std::vector<std::vector<int> > >();
    std::vector<std::vector<int> > expected(10, {0, 1, 2, 3, 4, 5, 6, 7});
    check_equal(vv, expected);
}

TEST(ConversionTest, Issue556) {
    using namespace fermat::ranges;
    std::string s{"abc"};
    any_view<any_view<char, category::random_access>, category::random_access> v1 =
            views::single(s | views::drop(1));
    any_view<any_view<char, category::random_access>, category::random_access> v2 =
            views::single(s | views::drop(2));
    auto v3 = views::concat(v1, v2);

    auto owner1 = v3 | to<std::vector<std::vector<char> > >();
    auto owner2 = v3 | to<std::vector<std::string> >();

    std::vector<std::vector<char> > expected1{{'b', 'c'}, {'c'}};
    std::vector<std::string> expected2{{"bc"}, {"c"}};
    check_equal(owner1, expected1);
    EXPECT_EQ(owner2, expected2);
}

TEST(ConversionTest, MapFromZip) {
    using namespace fermat::ranges;
    auto to_string = [](int i) {
        std::stringstream str;
        str << i;
        return str.str();
    };
    auto m = views::zip(views::ints, views::ints | views::transform(to_string)) |
             views::take(5) | to<std::map<int, std::string> >();
    using P = std::pair<const int, std::string>;
    check_equal(m, {P{0, "0"}, P{1, "1"}, P{2, "2"}, P{3, "3"}, P{4, "4"}});
}

TEST(ConversionTest, MapFromForEach) {
    using namespace fermat::ranges;
    auto to_string = [](int i) {
        std::stringstream str;
        str << i;
        return str.str();
    };
    auto m = views::for_each(views::ints(0, 5), [&](int i) {
        return yield(std::make_pair(i, to_string(i)));
    }) | to<std::map<int, std::string> >();
    using P = std::pair<const int, std::string>;
    check_equal(m, {P{0, "0"}, P{1, "1"}, P{2, "2"}, P{3, "3"}, P{4, "4"}});
}

TEST(ConversionTest, SetFromIotaTake) {
    using namespace fermat::ranges;
    auto s = views::ints | views::take(10) | to<std::set<int> >();
    check_equal(s, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    static_assert(!view_<std::initializer_list<int> >, "");
}
