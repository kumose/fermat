/// enumerate_gtest.cpp
/// Google Test conversion of range-v3 enumerate view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <tuple>
#include <iterator>
#include <cstdint>

#include <fermat/range/access.h>            /// ranges::begin, ranges::end
#include <fermat/range/primitives.h>        /// ranges::size, ranges::empty
#include <fermat/view/enumerate.h>          /// views::enumerate
#include <fermat/view/iota.h>               /// views::iota
#include <fermat/view/indices.h>            /// views::indices
#include <fermat/view/transform.h>          /// views::transform

/// ------------------------------------------------------------
/// Helper: test_enumerate_with (runtime check)
/// ------------------------------------------------------------
template<typename RangeT>
void test_enumerate_with(RangeT&& range)
{
    auto enumerated_range = ranges::views::enumerate(range);
    // borrowed_range check is compile-time only; omitted for runtime test.

    std::size_t idx_ref = 0;
    auto it_ref = ranges::begin(range);

    for (auto it = enumerated_range.begin(); it != enumerated_range.end(); ++it)
    {
        const auto idx = std::get<0>(*it);
        const auto value = std::get<1>(*it);
        EXPECT_EQ(idx, idx_ref++);
        EXPECT_EQ(value, *it_ref++);
    }
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(EnumerateTest, Array)
{
    int const es[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    test_enumerate_with(es);
}

TEST(EnumerateTest, VectorOfLists)
{
    std::vector<std::list<int>> range{{1, 2, 3}, {3, 5, 6, 7}, {10, 5, 6, 1}, {1, 2, 3, 4}};
    const auto rcopy = range;

    test_enumerate_with(range);

    // check that range hasn't been accidentally modified
    EXPECT_EQ(rcopy, range);

    // empty range
    range.clear();
    test_enumerate_with(range);
}

TEST(EnumerateTest, List)
{
    std::list<int> range{9, 8, 7, 6, 5, 4, 3, 2, 1};
    test_enumerate_with(range);

    range.clear();
    test_enumerate_with(range);
}

TEST(EnumerateTest, InitializerList)
{
    test_enumerate_with(std::initializer_list<int>{9, 8, 7, 6, 5, 4, 3, 2, 1});
}

TEST(EnumerateTest, IotaZeroLength)
{
    auto range = ranges::views::iota(0, 0);
    test_enumerate_with(range);
}

TEST(EnumerateTest, IotaNegativeToPositive)
{
    auto range = ranges::views::iota(-10000, 10000);
    test_enumerate_with(range);
}

TEST(EnumerateTest, IotaUnsignedZeroLength)
{
    auto range = ranges::views::iota(static_cast<std::uintmax_t>(0),
                                     static_cast<std::uintmax_t>(0));
    test_enumerate_with(range);
}

TEST(EnumerateTest, IotaSignedNegativeToPositive)
{
    auto range2 = ranges::views::iota(static_cast<std::intmax_t>(-10000),
                                      static_cast<std::intmax_t>(10000));
    test_enumerate_with(range2);
}

/// Regression test for issue #1141
TEST(EnumerateTest, IndicesTransformEnumerate)
{
    using namespace ranges;
    auto x = views::indices(std::uintmax_t(100))
           | views::transform([](std::uintmax_t) { return ""; })
           | views::enumerate;
    using X = decltype(x);
    // Check difference type and value type (compile-time)
    static_assert(std::is_same<range_difference_t<X>, detail::diffmax_t>::value,
                  "range_difference_t is not detail::diffmax_t");
    static_assert(std::is_same<range_value_t<X>, std::pair<detail::diffmax_t, char const*>>::value,
                  "range_value_t has wrong type");
}
