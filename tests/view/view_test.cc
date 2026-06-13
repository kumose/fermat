/// my_drop_gtest.cpp
/// Google Test conversion of range-v3 custom drop view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/my_drop.cpp (issue #1169)
/// Adapted to use fermat library and Google Test.

#include <gtest/gtest.h>

#include <vector>

#include <fermat/view/drop.h>          // for drop_view
#include <fermat/view/view.h>           // for make_view_closure, view_closure
#include <fermat/range/access.h>        // for begin/end
#include <fermat/iterator/operations.h> // for equality comparison

/// ------------------------------------------------------------
/// Custom drop view (adapted from original test)
/// ------------------------------------------------------------

namespace my_drop_ns
{
    using namespace ranges;

    struct my_drop_base_fn
    {
        template<typename Rng>
        auto operator()(Rng && rng, range_difference_t<Rng> n) const
        {
            return drop_view<views::all_t<Rng>>(views::all(static_cast<Rng &&>(rng)), n);
        }
    };

    struct my_drop_fn : my_drop_base_fn
    {
        using my_drop_base_fn::operator();

        template<typename Int>
        constexpr auto operator()(Int n) const
        {
            return make_view_closure([=](auto && rng) {
                return my_drop_base_fn{}(std::forward<decltype(rng)>(rng), n);
            });
        }
    };

    inline constexpr my_drop_fn my_drop{};
} // namespace my_drop_ns

using my_drop_ns::my_drop;

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected)
{
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

/// Test constexpr composition of custom drop views (issue #1169)
TEST(MyDropViewTest, ConstexprComposition)
{
    using namespace ranges;

    // The original test used a constexpr lambda, which requires C++17.
    // We keep the same conditional compilation for portability.
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603L
    constexpr auto drop1 = my_drop(1);
    constexpr auto drop3 = drop1 | my_drop(2);

    std::vector<int> vec = {1, 2, 3, 4};
    auto rng = vec | drop3;
    check_equal(rng, {4});
#else
    // Fallback: runtime test only
    auto drop1 = my_drop(1);
    auto drop3 = drop1 | my_drop(2);
    std::vector<int> vec = {1, 2, 3, 4};
    auto rng = vec | drop3;
    check_equal(rng, {4});
#endif
}
