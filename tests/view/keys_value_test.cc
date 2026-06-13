/// map_gtest.cpp
/// Google Test conversion of range-v3 map view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <utility>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/map.h>
#include <fermat/view/iota.h>           /// views::iota
#include <fermat/view/zip.h>            /// views::zip
#include <fermat/algorithm/find.h>      /// ranges::find

/// ------------------------------------------------------------
/// Helper: has_type (static assertion on expression type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// debug_input_view (minimal input view for testing)
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T>>
{
    struct data
    {
        const T* first_;
        std::ptrdiff_t size_;
    };
    std::shared_ptr<data> data_;

    debug_input_view() = default;
    explicit debug_input_view(const T* first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size}))
    {}

    const T* begin() const { return data_->first_; }
    const T* end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace ranges
{
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
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

/// Overload for pair-like elements (keys/values)
template<typename Rng, typename U, typename V>
void check_equal(Rng&& rng, std::initializer_list<U> expected) {
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

TEST(MapViewTest, KeysAndValuesOnStdMap) {
    using namespace ranges;

    std::map<std::string, int> m = {
        {"this", 0},
        {"that", 1},
        {"other", 2}};

    // keys
    auto keys = m | views::keys;
    has_type<std::string const&>(*begin(keys));
    EXPECT_EQ(&*begin(keys), &m.begin()->first);
    check_equal(keys, {"other", "that", "this"});

    // values
    auto values = m | views::values;
    has_type<int&>(*begin(values));
    EXPECT_EQ(&*begin(values), &m.begin()->second);
    check_equal(values, {2, 1, 0});
}

// Regression test for #526 – removed detail::get_first because it is an internal
// implementation detail not guaranteed to exist in Fermat. The essential test
// for views::keys on a zip view is kept below.
TEST(MapViewTest, ZipWithIota) {
    using namespace ranges;

    std::vector<int> xs = {42, 100, -1234};
    auto exs = views::zip(views::ints, xs);
    check_equal(views::keys(exs), {0, 1, 2});
}

TEST(MapViewTest, KeysAndValuesOnDebugInputView) {
    using namespace ranges;

    std::pair<int, int> const rgp[] = {{0, 2}, {1, 1}, {2, 0}};
    auto key_range = debug_input_view<std::pair<int, int> const>{rgp, 3} | views::keys;
    check_equal(key_range, {0, 1, 2});

    auto value_range = debug_input_view<std::pair<int, int> const>{rgp, 3} | views::values;
    check_equal(value_range, {2, 1, 0});
}

TEST(MapViewTest, FindOnKeysAndValues) {
    using namespace ranges;

    std::map<std::string, int> m = {
        {"this", 0},
        {"that", 1},
        {"other", 2}};

    auto it = find(m | views::keys, "other");
    EXPECT_EQ(it.base()->second, 2);

    auto it2 = find(m | views::values, 1);
    EXPECT_EQ(it2.base()->first, "that");
}
