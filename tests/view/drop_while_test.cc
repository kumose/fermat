/// drop_while_gtest.cpp
/// Google Test conversion of range-v3 drop_while view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/drop_while.h>
#include <fermat/view/iota.h>
#include <fermat/view/take.h>
#include <fermat/utility/copy.h>

/// ------------------------------------------------------------
/// my_data structure with projection test
/// ------------------------------------------------------------
struct my_data {
    int i;
};

bool operator==(my_data left, my_data right) {
    return left.i == right.i;
}

/// ------------------------------------------------------------
/// debug_input_view (minimal input view for testing)
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : fermat::ranges::view_interface<debug_input_view<T> > {
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

namespace fermat::ranges {
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T> > = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
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

/// Overload for std::list (for projection test)
template<typename T>
void check_equal(const std::list<T> &actual, std::initializer_list<T> expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(DropWhileTest, IotaDropWhile) {
    using namespace fermat::ranges;

    auto rng0 = views::iota(10) | views::drop_while([](int i) { return i < 25; });
    auto b = rng0.begin();
    EXPECT_EQ(*b, 25);
    EXPECT_EQ(*(b + 1), 26);
    check_equal(rng0 | views::take(10), {25, 26, 27, 28, 29, 30, 31, 32, 33, 34});
}

TEST(DropWhileTest, ListDropWhileNoMatch) {
    using namespace fermat::ranges;

    std::list<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng1 = vi | views::drop_while([](int i) { return i != 50; });
    EXPECT_EQ(rng1.begin(), rng1.end());
}

TEST(DropWhileTest, MutablePredicate) {
    using namespace fermat::ranges;

    static int const rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int cnt = 0;
    auto mutable_only = views::drop_while(rgi, [cnt](int) mutable { return ++cnt <= 5; });
    check_equal(mutable_only, {5, 6, 7, 8, 9});
}

TEST(DropWhileTest, DebugInputView) {
    using namespace fermat::ranges;

    static int const rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng = debug_input_view<int const>{rgi, 10} | views::drop_while([](int i) { return i < 4; });
    check_equal(rng, {4, 5, 6, 7, 8, 9});
}

TEST(DropWhileTest, Projection) {
    using namespace fermat::ranges;

    const std::list<my_data> data_list{{1}, {2}, {3}, {1}};
    auto rng = data_list | views::drop_while([](int i) { return i <= 2; }, &my_data::i);
    check_equal(rng, {my_data{3}, my_data{1}});
}

TEST(DropWhileTest, ConstArrayAllDropped) {
    using namespace fermat::ranges;

    static int const rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng2 = rgi | views::drop_while([](int i) { return i != 50; });
    EXPECT_EQ(fermat::ranges::size(rng2), 0u);
}
