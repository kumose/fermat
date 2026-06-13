/// move_gtest.cpp
/// Google Test conversion of range-v3 move view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <list>
#include <memory>
#include <utility>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/move.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/utility/copy.h>
#include <fermat/view/move.h>

/// ------------------------------------------------------------
/// MoveOnlyString: a move‑only string type for testing
/// ------------------------------------------------------------
struct MoveOnlyString {
    std::string s;

    MoveOnlyString() = default;

    MoveOnlyString(const char *cstr) : s(cstr) {
    }

    MoveOnlyString(MoveOnlyString &&) = default;

    MoveOnlyString &operator=(MoveOnlyString &&) = default;

    MoveOnlyString(const MoveOnlyString &) = delete;

    MoveOnlyString &operator=(const MoveOnlyString &) = delete;

    bool operator==(const MoveOnlyString &other) const { return s == other.s; }
    bool operator!=(const MoveOnlyString &other) const { return s != other.s; }
};

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing (borrowed range)
/// Note: uses non-const pointer to allow moving elements.
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T> > {
    struct data {
        T *first_;
        std::ptrdiff_t size_;
    };

    std::shared_ptr<data> data_;

    debug_input_view() = default;

    explicit debug_input_view(T *first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size})) {
    }

    T *begin() const { return data_->first_; }
    T *end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace ranges {
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T> > = true;
}

/// Helper: check_equal for ranges vs initializer_list
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

/// Helper for MoveOnlyString vector
template<typename Rng>
void check_equal(Rng &&rng, std::vector<MoveOnlyString> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(it->s, val.s);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(MoveTest, MoveViewOnDebugInputView) {
    using namespace ranges;

    // Create a modifiable array of MoveOnlyString
    MoveOnlyString arr[] = {"a", "b", "c", "d", "e"};
    debug_input_view<MoveOnlyString> di{arr, 5};

    // Apply move view
    auto rng = di | views::move;
    static_assert(view_<decltype(rng)>);
    static_assert(input_range<decltype(rng)>);

    // Target array for move
    MoveOnlyString target[5];

    // Use ranges::move algorithm to move elements from the view to target
    ranges::move(rng, target);

    // Verify target contains the original values
    const char *expected_original[] = {"a", "b", "c", "d", "e"};
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(target[i].s, expected_original[i]);
    }

    // The original array `arr` is left in a valid but unspecified state
    // (typically empty strings). We do not check it because the standard
    // allows moved-from objects to be in any valid state.
}
