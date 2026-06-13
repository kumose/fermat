/// sample_gtest.cpp
/// Google Test conversion of range-v3 sample view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <array>
#include <vector>
#include <numeric>
#include <random>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/view/sample.h>
#include <fermat/algorithm/equal.h>
#include <fermat/algorithm/copy.h>

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T> > {
    struct data {
        const T *first_;
        std::ptrdiff_t size_;
    };

    std::shared_ptr<data> data_;

    debug_input_view() = default;

    explicit debug_input_view(const T *first, std::ptrdiff_t size)
        : data_(std::make_shared<data>(data{first, size})) {
    }

    const T *begin() const { return data_->first_; }
    const T *end() const { return data_->first_ + data_->size_; }
    std::ptrdiff_t size() const { return data_->size_; }
};

namespace ranges {
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T> > = true;
}

/// Helper: check_equal for ranges vs initializer_list (not used in these tests, but kept)
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

TEST(SampleTest, FromVector) {
    using namespace ranges;

    std::mt19937 engine;
    std::vector<int> pop(100);
    std::iota(pop.begin(), pop.end(), 0);

    constexpr int N = 32;
    std::array<int, N> tmp;

    // First sample: copy to tmp
    auto rng = pop | views::sample(N, engine);
    ranges::copy(rng, tmp.begin());

    // Second sample using the same engine (no reset) -> should be different
    rng = pop | views::sample(N, engine);
    EXPECT_FALSE(ranges::equal(rng, tmp));

    // Reset engine -> should be equal to first sample again
    engine = std::mt19937{};
    rng = pop | views::sample(N, engine);
    EXPECT_TRUE(ranges::equal(rng, tmp));
}

TEST(SampleTest, FromDebugInputView) {
    using namespace ranges;

    std::mt19937 engine;
    int const some_ints[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    auto rng = debug_input_view<int const>{some_ints, 9} | views::sample(4, engine);

    EXPECT_EQ(ranges::distance(rng), 4);
}
