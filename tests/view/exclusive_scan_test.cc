/// exclusive_scan_gtest.cpp
/// Google Test conversion of range-v3 exclusive_scan view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>

#include <fermat/range/access.h>            /// ranges::begin, ranges::end
#include <fermat/range/primitives.h>        /// ranges::empty, ranges::size
#include <fermat/view/exclusive_scan.h>     /// views::exclusive_scan
#include <fermat/utility/copy.h>            /// ranges::copy (if needed)

/// ------------------------------------------------------------
/// Helper: has_type (static assertion on expression type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ExclusiveScanTest, NonEmptyRange) {
    using namespace ranges;

    int rgi[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Basic exclusive_scan
    {
        auto rng = rgi | views::exclusive_scan(0);
        has_type<int&>(*begin(rgi));
        has_type<int>(*begin(rng));
        // Concept checks omitted (view_, sized_range, forward_range, etc.)
        check_equal(rng, {0, 1, 3, 6, 10, 15, 21, 28, 36, 45});
    }

    // Mutable lambda version
    {
        int cnt = 0;
        auto mutable_rng = views::exclusive_scan(rgi, 0, [cnt](int i, int j) mutable {
            return i + j + cnt++;
        });
        check_equal(mutable_rng, {0, 1, 4, 9, 16, 25, 36, 49, 64, 81});
        // view_ const-ness not checked here
    }
}

TEST(ExclusiveScanTest, EmptyRange) {
    using namespace ranges;

    std::vector<int> rgi;
    auto rng = rgi | views::exclusive_scan(0);
    has_type<int>(*begin(rng));
    // Concept checks omitted
    EXPECT_TRUE(empty(rng));
}
