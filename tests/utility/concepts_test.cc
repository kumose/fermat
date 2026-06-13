/// concepts_gtest.cpp
/// Google Test conversion of range-v3 concept test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>
#include <vector>
#include <sstream>

#include <fermat/concepts/concepts.hpp>               /// provides same_as, convertible_to, etc.
#include <fermat/iterator/concepts.h>         /// input_iterator, forward_iterator, etc.
#include <fermat/range/concepts.h>            /// view_, common_range, sized_range
#include <fermat/range/traits.h>              /// range traits
#include <fermat/iterator/traits.h>           /// iterator traits
#include <fermat/view/istream.h>              /// istream_view (if available)
#include <fermat/iterator/common_iterator.h>  /// common_iterator

/// ------------------------------------------------------------
/// Helper types for testing
/// ------------------------------------------------------------

struct moveonly
{
    moveonly(moveonly&&) = default;
    moveonly& operator=(moveonly&&) = default;
};

struct nonmovable
{
    nonmovable(nonmovable const &) = delete;
    nonmovable& operator=(nonmovable const &) = delete;
};

struct nondefaultconstructible
{
    nondefaultconstructible(int) {}
};

struct NotDestructible
{
    ~NotDestructible() = delete;
};

struct IntComparable
{
    operator int() const;

    friend bool operator==(IntComparable, IntComparable);
    friend bool operator!=(IntComparable, IntComparable);

    friend bool operator<(IntComparable, IntComparable);
    friend bool operator>(IntComparable, IntComparable);
    friend bool operator<=(IntComparable, IntComparable);
    friend bool operator>=(IntComparable, IntComparable);

    friend bool operator==(int, IntComparable);
    friend bool operator!=(int, IntComparable);
    friend bool operator==(IntComparable, int);
    friend bool operator!=(IntComparable, int);

    friend bool operator<(int, IntComparable);
    friend bool operator<(IntComparable, int);
    friend bool operator>(int, IntComparable);
    friend bool operator>(IntComparable, int);
    friend bool operator<=(int, IntComparable);
    friend bool operator<=(IntComparable, int);
    friend bool operator>=(int, IntComparable);
    friend bool operator>=(IntComparable, int);
};

struct IntSwappable
{
    operator int() const;

    friend void swap(int &, IntSwappable);
    friend void swap(IntSwappable, int &);
    friend void swap(IntSwappable, IntSwappable);
};

struct XXX
{
    XXX() = default;
    XXX(XXX&&) = delete;
    explicit XXX(int) {}
};

/// A simple view for testing view concept (minimal implementation)
struct myview : ranges::view_base
{
    const char* begin() const { return nullptr; }
    const char* end() const { return nullptr; }
};

/// ------------------------------------------------------------
/// Compile-time concept checks (preserved from original)
/// ------------------------------------------------------------

// same_as
static_assert(ranges::same_as<int, int>, "");
static_assert(ranges::same_as<void, void>, "");
static_assert(ranges::same_as<void const, void const>, "");
static_assert(!ranges::same_as<int&, int>, "");
static_assert(!ranges::same_as<void, void const>, "");
static_assert(!ranges::same_as<void(), void(*)()>, "");

// convertible_to
static_assert(ranges::convertible_to<int, int>, "");
static_assert(ranges::convertible_to<short&, short const&>, "");
static_assert(ranges::convertible_to<int, short>, "");
static_assert(!ranges::convertible_to<int&, short&>, "");
static_assert(!ranges::convertible_to<int, void>, "");
static_assert(!ranges::convertible_to<int, int&>, "");

// unsigned_integral
static_assert(ranges::unsigned_integral<unsigned>, "");
static_assert(!ranges::unsigned_integral<int>, "");

// assignable_from
static_assert(ranges::assignable_from<int&, int>, "");
static_assert(!ranges::assignable_from<int const&, int>, "");
static_assert(!ranges::assignable_from<int, int>, "");

// destructible
static_assert(ranges::destructible<int>, "");
static_assert(ranges::destructible<const int>, "");
static_assert(!ranges::destructible<void>, "");
static_assert(ranges::destructible<int&>, "");
static_assert(!ranges::destructible<void()>, "");
static_assert(ranges::destructible<void(*)()>, "");
static_assert(ranges::destructible<void(&)()>, "");
static_assert(!ranges::destructible<int[]>, "");
static_assert(ranges::destructible<int[2]>, "");
static_assert(ranges::destructible<int(*)[2]>, "");
static_assert(ranges::destructible<int(&)[2]>, "");
static_assert(ranges::destructible<moveonly>, "");
static_assert(ranges::destructible<nonmovable>, "");
static_assert(!ranges::destructible<NotDestructible>, "");

// constructible_from
static_assert(ranges::constructible_from<int>, "");
static_assert(ranges::constructible_from<int const>, "");
static_assert(!ranges::constructible_from<void>, "");
static_assert(!ranges::constructible_from<int const &>, "");
static_assert(!ranges::constructible_from<int ()>, "");
static_assert(!ranges::constructible_from<int(&)()>, "");
static_assert(!ranges::constructible_from<int[]>, "");
static_assert(ranges::constructible_from<int[5]>, "");
static_assert(!ranges::constructible_from<nondefaultconstructible>, "");
static_assert(ranges::constructible_from<int const(&)[5], int(&)[5]>, "");
static_assert(!ranges::constructible_from<int, int(&)[3]>, "");

static_assert(ranges::constructible_from<int, int>, "");
static_assert(ranges::constructible_from<int, int&>, "");
static_assert(ranges::constructible_from<int, int&&>, "");
static_assert(ranges::constructible_from<int, const int>, "");
static_assert(ranges::constructible_from<int, const int&>, "");
static_assert(ranges::constructible_from<int, const int&&>, "");

static_assert(!ranges::constructible_from<int&, int>, "");
static_assert(ranges::constructible_from<int&, int&>, "");
static_assert(!ranges::constructible_from<int&, int&&>, "");
static_assert(!ranges::constructible_from<int&, const int>, "");
static_assert(!ranges::constructible_from<int&, const int&>, "");
static_assert(!ranges::constructible_from<int&, const int&&>, "");

static_assert(ranges::constructible_from<const int&, int>, "");
static_assert(ranges::constructible_from<const int&, int&>, "");
static_assert(ranges::constructible_from<const int&, int&&>, "");
static_assert(ranges::constructible_from<const int&, const int>, "");
static_assert(ranges::constructible_from<const int&, const int&>, "");
static_assert(ranges::constructible_from<const int&, const int&&>, "");

static_assert(ranges::constructible_from<int&&, int>, "");
static_assert(!ranges::constructible_from<int&&, int&>, "");
static_assert(ranges::constructible_from<int&&, int&&>, "");
static_assert(!ranges::constructible_from<int&&, const int>, "");
static_assert(!ranges::constructible_from<int&&, const int&>, "");
static_assert(!ranges::constructible_from<int&&, const int&&>, "");

static_assert(ranges::constructible_from<const int&&, int>, "");
static_assert(!ranges::constructible_from<const int&&, int&>, "");
static_assert(ranges::constructible_from<const int&&, int&&>, "");
static_assert(ranges::constructible_from<const int&&, const int>, "");
static_assert(!ranges::constructible_from<const int&&, const int&>, "");
static_assert(ranges::constructible_from<const int&&, const int&&>, "");

// XXX specific checks
static_assert(ranges::constructible_from<XXX, int>, "");
static_assert(!ranges::move_constructible<XXX>, "");
static_assert(!ranges::movable<XXX>, "");
static_assert(!ranges::semiregular<XXX>, "");
static_assert(!ranges::regular<XXX>, "");

// default_constructible
static_assert(ranges::default_constructible<int>, "");
static_assert(ranges::default_constructible<int const>, "");
static_assert(!ranges::default_constructible<int const &>, "");
static_assert(!ranges::default_constructible<int ()>, "");
static_assert(!ranges::default_constructible<int(&)()>, "");
static_assert(!ranges::default_constructible<int[]>, "");
static_assert(ranges::default_constructible<int[5]>, "");
static_assert(!ranges::default_constructible<nondefaultconstructible>, "");

// move_constructible
static_assert(ranges::move_constructible<int>, "");
static_assert(ranges::move_constructible<const int>, "");
static_assert(ranges::move_constructible<int &>, "");
static_assert(ranges::move_constructible<int &&>, "");
static_assert(ranges::move_constructible<const int &>, "");
static_assert(ranges::move_constructible<const int &&>, "");
static_assert(ranges::destructible<moveonly>, "");
static_assert(ranges::constructible_from<moveonly, moveonly>, "");
static_assert(ranges::move_constructible<moveonly>, "");
static_assert(!ranges::move_constructible<nonmovable>, "");
static_assert(ranges::move_constructible<nonmovable &>, "");
static_assert(ranges::move_constructible<nonmovable &&>, "");
static_assert(ranges::move_constructible<const nonmovable &>, "");
static_assert(ranges::move_constructible<const nonmovable &&>, "");

// copy_constructible
static_assert(ranges::copy_constructible<int>, "");
static_assert(ranges::copy_constructible<const int>, "");
static_assert(ranges::copy_constructible<int &>, "");
static_assert(!ranges::copy_constructible<int &&>, "");
static_assert(ranges::copy_constructible<const int &>, "");
static_assert(!ranges::copy_constructible<const int &&>, "");
static_assert(!ranges::copy_constructible<moveonly>, "");
static_assert(!ranges::copy_constructible<nonmovable>, "");
static_assert(ranges::copy_constructible<nonmovable &>, "");
static_assert(!ranges::copy_constructible<nonmovable &&>, "");
static_assert(ranges::copy_constructible<const nonmovable &>, "");
static_assert(!ranges::copy_constructible<const nonmovable &&>, "");

// movable
static_assert(ranges::movable<int>, "");
static_assert(!ranges::movable<int const>, "");
static_assert(ranges::movable<moveonly>, "");
static_assert(!ranges::movable<nonmovable>, "");

// copyable
static_assert(ranges::copyable<int>, "");
static_assert(!ranges::copyable<int const>, "");
static_assert(!ranges::copyable<moveonly>, "");
static_assert(!ranges::copyable<nonmovable>, "");

// input_iterator, etc.
static_assert(ranges::input_iterator<int*>, "");
static_assert(!ranges::input_iterator<int>, "");
static_assert(ranges::forward_iterator<int*>, "");
static_assert(!ranges::forward_iterator<int>, "");
static_assert(ranges::bidirectional_iterator<int*>, "");
static_assert(!ranges::bidirectional_iterator<int>, "");
static_assert(ranges::random_access_iterator<int*>, "");
static_assert(!ranges::random_access_iterator<int>, "");
static_assert(ranges::contiguous_iterator<int*>, "");
static_assert(!ranges::contiguous_iterator<int>, "");

// view concept (using our minimal view)
static_assert(ranges::view_<myview>, "");
static_assert(!ranges::view_<int>, "");

// range concepts
static_assert(ranges::common_range<std::vector<int>>, "");
static_assert(ranges::common_range<std::vector<int> &>, "");
static_assert(!ranges::view_<std::vector<int>>, "");
static_assert(!ranges::view_<std::vector<int> &>, "");
static_assert(ranges::random_access_iterator<ranges::iterator_t<std::vector<int> const &>>, "");
// Note: istream_view may not be available; comment out if Fermat lacks it.
// static_assert(!ranges::common_range<ranges::istream_view<int>>, "");

// predicate
static_assert(ranges::predicate<std::less<int>, int, int>, "");
static_assert(!ranges::predicate<std::less<int>, char*, int>, "");

// output_iterator
static_assert(ranges::output_iterator<int *, int>, "");
static_assert(!ranges::output_iterator<int const *, int>, "");

// swappable
static_assert(ranges::swappable<int &>, "");
static_assert(ranges::swappable<int>, "");
static_assert(!ranges::swappable<int const &>, "");
static_assert(ranges::swappable<IntSwappable>, "");
static_assert(ranges::swappable_with<IntSwappable, int &>, "");
static_assert(!ranges::swappable_with<IntSwappable, int const &>, "");

// totally_ordered etc.
static_assert(ranges::totally_ordered<int>, "");
static_assert(ranges::common_with<int, IntComparable>, "");
static_assert(ranges::common_reference_with<int &, IntComparable &>, "");
static_assert(ranges::totally_ordered_with<int, IntComparable>, "");
static_assert(ranges::totally_ordered_with<IntComparable, int>, "");
static_assert(ranges::detail::weakly_equality_comparable_with_<int, int>, "");
static_assert(ranges::equality_comparable<int>, "");
static_assert(ranges::equality_comparable_with<int, int>, "");
static_assert(ranges::equality_comparable_with<int, IntComparable>, "");
static_assert(ranges::equality_comparable_with<int &, IntComparable &>, "");

#if __cplusplus > 201703L && __has_include(<compare>) && \
    defined(__cpp_concepts) && defined(__cpp_impl_three_way_comparison)
#include <compare>

static_assert(ranges::three_way_comparable<int>);
static_assert(ranges::three_way_comparable<int, std::partial_ordering>);
static_assert(ranges::three_way_comparable<int, std::weak_ordering>);
static_assert(ranges::three_way_comparable<int, std::strong_ordering>);

static_assert(ranges::three_way_comparable_with<int, IntComparable>);
static_assert(ranges::three_way_comparable_with<int, IntComparable, std::partial_ordering>);
static_assert(ranges::three_way_comparable_with<int, IntComparable, std::weak_ordering>);
static_assert(ranges::three_way_comparable_with<int, IntComparable, std::strong_ordering>);
static_assert(ranges::three_way_comparable_with<IntComparable, int>);
static_assert(ranges::three_way_comparable_with<IntComparable, int, std::partial_ordering>);
static_assert(ranges::three_way_comparable_with<IntComparable, int, std::weak_ordering>);
static_assert(ranges::three_way_comparable_with<IntComparable, int, std::strong_ordering>);
#endif // supports spaceship

// sized_range test (using vector)
static_assert(ranges::sized_range<std::vector<int>>, "");

/// ------------------------------------------------------------
/// Google Test dummy test (all checks are compile-time)
/// ------------------------------------------------------------
TEST(ConceptsTest, AllChecksAreStatic) {
    SUCCEED();
}
