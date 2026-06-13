/// addressof_gtest.cpp
/// Google Test conversion of range-v3 addressof view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include <fermat/range/access.h>                       /// ranges::begin, ranges::end
#include <fermat/view/addressof.h>               /// views::addressof
#include <fermat/view/facade.h>                  /// view_facade
#include <fermat/view/iota.h>                    /// views::iota
#include <fermat/view/take.h>                    /// views::take

/// Helper to compare a range with an initializer list
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
// simple_test: vector of ints -> addresses
// ------------------------------------------------------------------
void simple_test() {
    std::vector<int> list = {1, 2, 3};
    auto out = list | ranges::views::addressof;
    check_equal(out, {&list[0], &list[1], &list[2]});
}

// ------------------------------------------------------------------
// Custom input range (simulates an input range)
// ------------------------------------------------------------------
struct test_istream_range
  : ranges::view_facade<test_istream_range, ranges::unknown>
{
private:
    friend ranges::range_access;

    std::vector<int>* list;

    struct cursor {
        std::size_t i = 0;
        std::vector<int>* list = nullptr;

        cursor() = default;
        explicit cursor(std::vector<int>& list_) : list(&list_) {}

        void next() { ++i; }
        int& read() const noexcept { return (*list)[i]; }
        bool equal(ranges::default_sentinel_t) const {
            return i == list->size();
        }
    };

    cursor begin_cursor() { return cursor{*list}; }

public:
    test_istream_range() = default;
    explicit test_istream_range(std::vector<int>& list_) : list(&list_) {}
};

void test_input_range() {
    std::vector<int> list{1, 2, 3};
    auto rng = test_istream_range(list);
    // Check that rng is an input_range
    static_assert(ranges::input_range<decltype(rng)>, "rng must be input_range");

    auto out = rng | ranges::views::addressof;
    check_equal(out, {&list[0], &list[1], &list[2]});
}

// ------------------------------------------------------------------
// xvalue range (declared but not defined)
// ------------------------------------------------------------------
struct test_xvalue_range
  : ranges::view_facade<test_xvalue_range, ranges::unknown>
{
private:
    friend ranges::range_access;

    struct cursor {
        cursor() = default;
        void next();
        int&& read() const noexcept;
        bool equal(ranges::default_sentinel_t) const;
    };

    cursor begin_cursor();
};

// Helper to detect if views::addressof can be called on a range
template<typename, typename = void>
constexpr bool can_view = false;

template<typename R>
constexpr bool can_view<R,
    meta::void_<decltype(ranges::views::addressof(std::declval<R>()))>> = true;

// prvalue ranges cannot be passed to views::addressof
static_assert(!can_view<decltype(ranges::views::iota(0, 3))>, "prvalue range should not be addressable");
// xvalue ranges cannot be passed to views::addressof
static_assert(!can_view<test_xvalue_range>, "xvalue range should not be addressable");

// ------------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------------
TEST(AddressofTest, Simple) {
    simple_test();
}

TEST(AddressofTest, InputRange) {
    test_input_range();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
