/// view_facade_gtest.cpp
/// Google Test conversion of range-v3 view_facade test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <memory>

#include <fermat/range/access.h>            /// ranges::begin, ranges::end
#include <fermat/range/primitives.h>        /// ranges::size, ranges::data, ranges::front, ranges::back
#include <fermat/view/facade.h>             /// ranges::view_facade, ranges::range_access

/// ------------------------------------------------------------
/// Custom view using view_facade (as in original)
/// ------------------------------------------------------------
struct MyRange
        : ranges::view_facade<MyRange> {
private:
    friend ranges::range_access;
    std::vector<int> ints_;

    struct cursor {
    private:
        std::vector<int>::const_iterator iter;

    public:
        using contiguous = std::true_type;

        cursor() = default;

        explicit cursor(std::vector<int>::const_iterator it) : iter(it) {
        }

        int const &read() const { return *iter; }
        bool equal(cursor const &that) const { return iter == that.iter; }
        void next() { ++iter; }
        void prev() { --iter; }
        std::ptrdiff_t distance_to(cursor const &that) const { return that.iter - iter; }
        void advance(std::ptrdiff_t n) { iter += n; }
    };

    cursor begin_cursor() const { return cursor{ints_.begin()}; }
    cursor end_cursor() const { return cursor{ints_.end()}; }

public:
    MyRange() : ints_{1, 2, 3, 4, 5, 6, 7} {
    }
};

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ViewFacadeTest, MutableRange) {
    using namespace ranges;

    auto r = MyRange{};

    // Concept checks (compile-time)
    static_assert(view_<decltype(r)>, "");
    static_assert(common_range<decltype(r)>, "");
    static_assert(sized_range<decltype(r)>, "");
    static_assert(contiguous_range<decltype(r)>, "");

    check_equal(r, {1, 2, 3, 4, 5, 6, 7});
    EXPECT_EQ(r.size(), 7u);
    EXPECT_EQ(r.front(), 1);
    EXPECT_EQ(r.back(), 7);
    EXPECT_EQ(&*r.begin(), r.data());
    EXPECT_EQ(r[1], 2);
    EXPECT_EQ(r[5], 6);
}

TEST(ViewFacadeTest, ConstRange) {
    using namespace ranges;

    const auto r = MyRange{};

    static_assert(common_range<decltype(r)>, "");
    static_assert(sized_range<decltype(r)>, "");
    static_assert(random_access_range<decltype(r)>, "");
    static_assert(contiguous_range<decltype(r)>, "");

    check_equal(r, {1, 2, 3, 4, 5, 6, 7});
    EXPECT_EQ(r.size(), 7u);
    EXPECT_EQ(r.front(), 1);
    EXPECT_EQ(r.back(), 7);
    EXPECT_EQ(&*r.begin(), r.data());
    EXPECT_EQ(r[1], 2);
    EXPECT_EQ(r[5], 6);
}
