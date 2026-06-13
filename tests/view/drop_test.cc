/// drop_gtest.cpp
/// Google Test conversion of range-v3 drop view test.
/// All runtime checks preserved; compile-time concept checks omitted for compatibility.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>           /// fermat::ranges::to
#include <fermat/view/drop.h>                  /// views::drop
#include <fermat/view/iota.h>                  /// views::iota
#include <fermat/view/reverse.h>               /// views::reverse
#include <fermat/view/take.h>                  /// views::take
#include <fermat/view/transform.h>             /// views::transform
#include <fermat/view/chunk.h>                 /// views::chunk
#include <fermat/view/join.h>                  /// views::join
#include <fermat/view/subrange.h>              /// fermat::ranges::subrange
#include <fermat/iterator/operations.h>        /// fermat::ranges::next, fermat::ranges::distance
#include <fermat/algorithm/copy.h>             /// fermat::ranges::copy (if needed)
#include <fermat/utility/copy.h>               /// fermat::ranges::copy (if needed)

/// ------------------------------------------------------------
/// Helper: has_type (static assert on expression type)
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual &&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
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

/// Overload for checking a vector (for regression #413)
template<typename T>
void check_equal(const std::vector<T> &actual, std::initializer_list<T> expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

/// Overload for checking a vector of vectors (elementwise)
template<typename T>
void check_equal(const std::vector<std::vector<T> > &actual,
                 std::initializer_list<std::initializer_list<T> > expected) {
    auto it = actual.begin();
    for (auto const &expected_row: expected) {
        EXPECT_NE(it, actual.end());
        check_equal(*it, expected_row);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(DropTest, RawArrayDrop6Reverse) {
    using namespace fermat::ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto rng0 = rgi | views::drop(6);
    has_type<int &>(*begin(rng0));
    check_equal(rng0, {6, 7, 8, 9, 10});
    EXPECT_EQ(size(rng0), 5u);

    auto rng1 = rng0 | views::reverse;
    has_type<int &>(*begin(rng1));
    check_equal(rng1, {10, 9, 8, 7, 6});
}

TEST(DropTest, VectorDrop6Reverse) {
    using namespace fermat::ranges;

    std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng2 = v | views::drop(6) | views::reverse;
    has_type<int &>(*begin(rng2));
    check_equal(rng2, {10, 9, 8, 7, 6});
}

TEST(DropTest, ListDrop6) {
    using namespace fermat::ranges;

    std::list<int> l{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto rng3 = l | views::drop(6);
    has_type<int &>(*begin(rng3));
    check_equal(rng3, {6, 7, 8, 9, 10});
}

TEST(DropTest, IotaInfiniteDrop10) {
    using namespace fermat::ranges;

    auto rng4 = views::iota(10) | views::drop(10);
    auto b = fermat::ranges::begin(rng4);
    EXPECT_EQ(*b, 20);
    EXPECT_EQ(*(b+1), 21);
}

TEST(DropTest, IotaDrop10Take10Reverse) {
    using namespace fermat::ranges;

    auto rng5 = views::iota(10) | views::drop(10) | views::take(10) | views::reverse;
    check_equal(rng5, {29, 28, 27, 26, 25, 24, 23, 22, 21, 20});
    EXPECT_EQ(size(rng5), 10u);
}

TEST(DropTest, SubrangeDrop) {
    using namespace fermat::ranges;

    int some_ints[] = {0, 1, 2};
    auto rng = subrange{some_ints + 0, some_ints + 1};
    auto rng2 = views::drop(rng, 2);
    EXPECT_EQ(begin(rng2), some_ints + 1);
    EXPECT_EQ(size(rng2), 0u);
}

TEST(DropTest, Regression413) {
    using namespace fermat::ranges;

    auto skips = [](std::vector<int> xs) {
        return views::ints(0, (int) xs.size())
               | views::transform([&](int n) {
                   return xs | views::chunk(n + 1)
                          | views::transform(views::drop(n))
                          | views::join;
               })
               | to<std::vector<std::vector<int> > >();
    };
    auto skipped = skips({1, 2, 3, 4, 5, 6, 7, 8});
    EXPECT_EQ(skipped.size(), 8u);
    if (skipped.size() >= 8u) {
        check_equal(skipped, {
                        {1, 2, 3, 4, 5, 6, 7, 8},
                        {2, 4, 6, 8},
                        {3, 6},
                        {4, 8},
                        {5},
                        {6},
                        {7},
                        {8}
                    });
    }
}

TEST(DropTest, DebugInputView) {
    using namespace fermat::ranges;

    static int const some_ints[] = {0, 1, 2, 3};
    auto rng = debug_input_view<int const>{some_ints, 4} | views::drop(2);
    using R = decltype(rng);
    // runtime check only
    check_equal(rng, {2, 3});
}

TEST(DropTest, Regression728) {
    using namespace fermat::ranges;

    auto rng1 = views::iota(1) | views::chunk(6) | views::take(3);
    int i = 2;
    for (auto o1: rng1) {
        auto rng2 = o1 | views::drop(1);
        check_equal(rng2, {i, i + 1, i + 2, i + 3, i + 4});
        i += 6;
    }
}

TEST(DropTest, Regression813) {
    using namespace fermat::ranges;

    static int const some_ints[] = {0, 1, 2, 3};
    auto rng = some_ints | views::drop(10);
    EXPECT_TRUE(empty(rng));
}
