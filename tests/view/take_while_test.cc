/// take_while_gtest.cpp
/// Google Test conversion of range-v3 take_while view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/take_while.cpp
/// Adapted to use fermat library and Google Test.

#include <gtest/gtest.h>

#include <vector>
#include <list>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/view/iota.h>
#include <fermat/view/generate.h>
#include <fermat/view/take_while.h>
#include <fermat/utility/copy.h>
#include <fermat/iterator/operations.h>

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing (borrowed range)
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : ranges::view_interface<debug_input_view<T>> {
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
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

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

/// Overload for vector of custom type (my_data)
/// Use (*it).i instead of it->i to avoid operator-> issues.
template<typename Rng, typename U>
void check_equal(Rng &&rng, std::vector<U> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ((*it).i, val.i);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Test data type
/// ------------------------------------------------------------
struct my_data {
    int i;
};

bool operator==(my_data left, my_data right) {
    return left.i == right.i;
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

/// Test take_while on infinite iota range
TEST(TakeWhileViewTest, IotaTakeWhile) {
    using namespace ranges;

    auto rng0 = views::iota(10) | views::take_while([](int i) { return i != 25; });
    // CPP_assert(view_<decltype(rng0)>);
    // CPP_assert(!common_range<decltype(rng0)>);
    // CPP_assert(random_access_iterator<decltype(rng0.begin())>);
    check_equal(rng0, {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
}

/// Test take_while on vector (finite range)
TEST(TakeWhileViewTest, VectorTakeWhile) {
    using namespace ranges;

    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng1 = vi | views::take_while([](int i) { return i != 50; });
    // CPP_assert(view_<decltype(rng1)>);
    // CPP_assert(random_access_range<decltype(rng1)>);
    check_equal(rng1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

/// Test with mutable predicate (non-const view)
TEST(TakeWhileViewTest, MutablePredicate) {
    using namespace ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int cnt = 0;
    auto mutable_only = views::take_while(rgi, [cnt](int) mutable { return ++cnt <= 5; });
    check_equal(mutable_only, {0, 1, 2, 3, 4});
    // CPP_assert(view_<decltype(mutable_only)>);
    // CPP_assert(!view_<decltype(mutable_only) const>);
}

/// Test take_while on generate view with const predicate
TEST(TakeWhileViewTest, GenerateTakeWhileConstPredicate) {
    using namespace ranges;

    auto ns = views::generate([]() mutable {
        static int N;
        return ++N;
    });
    auto rng = ns | views::take_while([](int i) { return i < 5; });
    check_equal(rng, {1, 2, 3, 4});
}

/// Test take_while on generate view with mutable predicate
TEST(TakeWhileViewTest, GenerateTakeWhileMutablePredicate) {
    using namespace ranges;

    auto ns = views::generate([]() mutable {
        static int N;
        return ++N;
    });
    auto rng = ns | views::take_while([](int i) mutable { return i < 5; });
    check_equal(rng, {1, 2, 3, 4});
}

/// Test take_while on debug_input_view (input range)
TEST(TakeWhileViewTest, DebugInputViewTakeWhile) {
    using namespace ranges;

    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto rng = debug_input_view<int const>{rgi, 10} | views::take_while([](int i) {
        return i != 5;
    });
    check_equal(rng, {0, 1, 2, 3, 4});
}

/// Test take_while with projection (using member pointer)
TEST(TakeWhileViewTest, GenerateTakeWhileWithProjection) {
    using namespace ranges;

    auto ns = views::generate([]() {
        static int N;
        return my_data{++N};
    });
    auto rng = ns | views::take_while([](int i) { return i < 5; },
                                      &my_data::i);
    check_equal(rng, std::vector<my_data>{{1}, {2}, {3}, {4}});
}
