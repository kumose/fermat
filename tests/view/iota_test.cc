/// iota_gtest.cpp
/// Google Test conversion of range-v3 iota view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <climits>
#include <cstdint>
#include <limits>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/iterator/operations.h>
#include <fermat/view/iota.h>
#include <fermat/view/indices.h>
#include <fermat/view/c_str.h>
#include <fermat/view/indirect.h>
#include <fermat/view/take.h>
#include <fermat/view/drop_exactly.h>
#include <fermat/algorithm/copy.h>

/// ------------------------------------------------------------
/// ForwardIterator: minimal forward iterator for testing
/// ------------------------------------------------------------
template<typename It>
class ForwardIterator {
    It it_;
public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept   = std::forward_iterator_tag;
    using value_type        = typename std::iterator_traits<It>::value_type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = typename std::iterator_traits<It>::pointer;
    using reference         = typename std::iterator_traits<It>::reference;

    ForwardIterator() = default;
    explicit ForwardIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer   operator->() const { return &*it_; }
    ForwardIterator& operator++() { ++it_; return *this; }
    ForwardIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    friend bool operator==(const ForwardIterator& a, const ForwardIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const ForwardIterator& a, const ForwardIterator& b) { return !(a == b); }
    It base() const { return it_; }
};

/// ------------------------------------------------------------
/// Helper types (as in original)
/// ------------------------------------------------------------
struct Int
{
    using difference_type = int;
    int i = 0;
    Int() = default;
    explicit Int(int j) : i(j) {}
    Int& operator++() { ++i; EXPECT_LE(i, 10); return *this; }
    Int operator++(int) { auto tmp = *this; ++*this; return tmp; }
    bool operator==(Int j) const { return i == j.i; }
    bool operator!=(Int j) const { return i != j.i; }
};

struct NonDefaultInt
{
    using difference_type = int;
    int i = 0;
    explicit NonDefaultInt(int j) : i(j) {}
    NonDefaultInt& operator++() { ++i; EXPECT_LE(i, 10); return *this; }
    NonDefaultInt operator++(int) { auto tmp = *this; ++*this; return tmp; }
    bool operator==(NonDefaultInt j) const { return i == j.i; }
    bool operator!=(NonDefaultInt j) const { return i != j.i; }
};

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

/// Overload for char (for indirect view test)
template<typename Rng>
void check_equal(Rng&& rng, const std::string& expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (char c : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, c);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for Int (using (*it).i to avoid operator-> issues)
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<Int> expected) {
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ((*it).i, val.i);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// ------------------------------------------------------------
/// Helper: test_iota_distance (runtime) – fixed type conversions
/// ------------------------------------------------------------
template<typename I>
void test_iota_distance() {
    using namespace ranges;
    using D = iter_difference_t<I>;
    I max = std::numeric_limits<I>::max();

    // Ensure both arguments have the same type I
    EXPECT_EQ(detail::iota_distance_(I(0), I(0)), D(0));
    EXPECT_EQ(detail::iota_distance_(I(1), I(0)), D(-1));
    EXPECT_EQ(detail::iota_distance_(I(0), I(1)), D(1));
    EXPECT_EQ(detail::iota_distance_(I(1), I(1)), D(0));

    // Use explicit conversion to I for expressions that may promote to int
    EXPECT_EQ(detail::iota_distance_(max, I(max - I(1))), D(-1));
    EXPECT_EQ(detail::iota_distance_(I(max - I(1)), max), D(1));
    EXPECT_EQ(detail::iota_distance_(max, max), D(0));
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(IotaTest, BasicConcepts) {
    using namespace ranges;

    static_assert(random_access_range<decltype(views::iota((unsigned short)0))>, "");
    static_assert(random_access_range<decltype(views::iota(0))>, "");
    // is_infinite trait not present in Fermat; skip
    static_assert(!sized_range<decltype(views::iota(0))>, "");
}

TEST(IotaTest, BasicOperations) {
    using namespace ranges;

    // With ForwardIterator
    {
        char const* sz = "hello world";
        auto r = views::iota(ForwardIterator<char const*>(sz))
               | views::take(10)
               | views::indirect;
        check_equal(r, std::string("hello worl"));
    }

    // views::ints
    {
        check_equal(views::ints | views::take(10), {0,1,2,3,4,5,6,7,8,9});
        check_equal(views::ints(0, unreachable) | views::take(10), {0,1,2,3,4,5,6,7,8,9});
        check_equal(views::ints(0,9), {0,1,2,3,4,5,6,7,8});
        check_equal(views::closed_indices(0,9), {0,1,2,3,4,5,6,7,8,9});
        check_equal(views::ints(1,10), {1,2,3,4,5,6,7,8,9});
        check_equal(views::closed_indices(1,10), {1,2,3,4,5,6,7,8,9,10});
    }
}

TEST(IotaTest, SignedCharRange) {
    using namespace ranges;
    auto chars = views::ints(std::numeric_limits<signed char>::min(),
                            std::numeric_limits<signed char>::max());
    static_assert(random_access_range<decltype(chars)>, "");
    static_assert(std::is_same<int, range_difference_t<decltype(chars)>>::value, "");
    static_assert(view_<decltype(chars)>, "");
    static_assert(common_range<decltype(chars)>, "");
    EXPECT_EQ(ranges::distance(chars.begin(), chars.end()),
              static_cast<long>(CHAR_MAX) - static_cast<long>(CHAR_MIN));
    EXPECT_EQ(chars.size(),
              static_cast<unsigned>(static_cast<long>(CHAR_MAX) - static_cast<long>(CHAR_MIN)));
}

TEST(IotaTest, UnsignedShortRange) {
    using namespace ranges;
    auto ushorts = views::ints(std::numeric_limits<unsigned short>::min(),
                               std::numeric_limits<unsigned short>::max());
    static_assert(view_<decltype(ushorts)>, "");
    static_assert(common_range<decltype(ushorts)>, "");
    static_assert(std::is_same<int, range_difference_t<decltype(ushorts)>>::value, "");
    static_assert(std::is_same<unsigned int, range_size_t<decltype(ushorts)>>::value, "");
    EXPECT_EQ(ranges::distance(ushorts.begin(), ushorts.end()), static_cast<int>(USHRT_MAX));
    EXPECT_EQ(ushorts.size(), static_cast<unsigned>(USHRT_MAX));
}

TEST(IotaTest, Uint32Range) {
    using namespace ranges;
    auto uints = views::closed_indices(
        std::numeric_limits<std::uint_least32_t>::min(),
        std::numeric_limits<std::uint_least32_t>::max() - 1);
    static_assert(view_<decltype(uints)>, "");
    static_assert(common_range<decltype(uints)>, "");
    static_assert(std::is_same<std::int_fast64_t, range_difference_t<decltype(uints)>>::value, "");
    static_assert(std::is_same<std::uint_fast64_t, range_size_t<decltype(uints)>>::value, "");
    EXPECT_EQ(uints.size(), static_cast<std::uint_fast64_t>(std::numeric_limits<std::uint32_t>::max()));
}

TEST(IotaTest, Int32Range) {
    using namespace ranges;
    auto is = views::closed_indices(
        std::numeric_limits<std::int_least32_t>::min(),
        std::numeric_limits<std::int_least32_t>::max() - 1);
    static_assert(std::is_same<std::int_fast64_t, range_difference_t<decltype(is)>>::value, "");
    static_assert(std::is_same<std::uint_fast64_t, range_size_t<decltype(is)>>::value, "");
    EXPECT_EQ(is.size(), static_cast<std::uint_fast64_t>(std::numeric_limits<std::uint32_t>::max()));
}

TEST(IotaTest, IntRange) {
    using namespace ranges;
    auto sints = views::ints(std::numeric_limits<int>::min(),
                            std::numeric_limits<int>::max());
    static_assert(random_access_range<decltype(sints)>, "");
    static_assert(std::is_same<std::int_fast64_t, range_difference_t<decltype(sints)>>::value, "");
    static_assert(view_<decltype(sints)>, "");
    static_assert(common_range<decltype(sints)>, "");
    std::int_fast64_t diff = static_cast<std::int_fast64_t>(INT_MAX) - static_cast<std::int_fast64_t>(INT_MIN);
    EXPECT_EQ(ranges::distance(sints.begin(), sints.end()), diff);
    EXPECT_EQ(sints.size(), static_cast<std::uint_fast64_t>(diff));
}

TEST(IotaTest, CustomIntType) {
    using namespace ranges;
    auto is = views::closed_iota(Int{0}, Int{10});
    check_equal(is, {Int{0},Int{1},Int{2},Int{3},Int{4},Int{5},Int{6},Int{7},Int{8},Int{9},Int{10}});
    static_assert(view_<decltype(is)>, "");
    static_assert(common_range<decltype(is)>, "");
    static_assert(!sized_range<decltype(is)>, "");
    static_assert(forward_range<decltype(is)>, "");
    static_assert(!bidirectional_range<decltype(is)>, "");
}

TEST(IotaTest, NonDefaultIntType) {
    using namespace ranges;
    auto is = views::iota(NonDefaultInt{0});
    static_assert(input_range<decltype(is)>, "");
    static_assert(!forward_range<decltype(is)>, "");
}

TEST(IotaTest, ClosedIotaInt) {
    using namespace ranges;
    auto is = views::closed_iota(0, 10);
    check_equal(is, {0,1,2,3,4,5,6,7,8,9,10});
    static_assert(view_<decltype(is)>, "");
    static_assert(common_range<decltype(is)>, "");
    static_assert(sized_range<decltype(is)>, "");
    static_assert(random_access_range<decltype(is)>, "");
    EXPECT_EQ(size(is), 11u);

    auto it = is.begin();
    auto e = is.end();
    auto be = e;
    --be;
    using D = range_difference_t<decltype(is)>;
    for (D i = 0; ; ++i) {
        EXPECT_EQ((e - it), (11 - i));
        EXPECT_EQ((it - e), -(11 - i));
        EXPECT_EQ((be - it), (10 - i));
        EXPECT_EQ((it - be), -(10 - i));
        if (i == 11) break;
        ++it;
    }
    for (D i = 11; ; --i) {
        EXPECT_EQ((e - it), (11 - i));
        EXPECT_EQ((it - e), -(11 - i));
        EXPECT_EQ((be - it), (10 - i));
        EXPECT_EQ((it - be), -(10 - i));
        if (i == 0) break;
        --it;
    }
    for (D i = 0; ; ++i) {
        it = next(is.begin(), i);
        EXPECT_EQ((e - it), (11 - i));
        EXPECT_EQ((it - e), -(11 - i));
        EXPECT_EQ((be - it), (10 - i));
        EXPECT_EQ((it - be), -(10 - i));
        if (i == 11) break;
    }
    EXPECT_EQ((e - 0), e);
    EXPECT_EQ((be - 0), be);
    EXPECT_EQ((e - 1), be);
    EXPECT_EQ((be - 1), is.begin() + 9);
}

TEST(IotaTest, DistanceFunctions) {
    test_iota_distance<int8_t>();
    test_iota_distance<int16_t>();
    test_iota_distance<int32_t>();
    test_iota_distance<int64_t>();

    test_iota_distance<uint8_t>();
    test_iota_distance<uint16_t>();
    test_iota_distance<uint32_t>();
    test_iota_distance<uint64_t>();
}

TEST(IotaTest, CStringWithIndirect) {
    using namespace ranges;
    // https://github.com/ericniebler/range-v3/issues/506
    auto cstr = views::c_str((const char*)"hello world");
    auto cstr2 = views::iota(cstr.begin(), cstr.end()) | views::indirect;
    check_equal(cstr2, std::string("hello world"));
    auto i = cstr2.begin();
    i += 4;
    EXPECT_EQ(*i, 'o');
    EXPECT_EQ((i - cstr2.begin()), 4);
}

TEST(IotaTest, Indices) {
    using namespace ranges;
    check_equal(views::indices | views::take(10),
                std::initializer_list<std::size_t>{0,1,2,3,4,5,6,7,8,9});

    check_equal(views::indices(0, 10), {0,1,2,3,4,5,6,7,8,9});
    check_equal(views::closed_indices(0, 10), {0,1,2,3,4,5,6,7,8,9,10});

    check_equal(views::indices(10), {0,1,2,3,4,5,6,7,8,9});
    check_equal(views::closed_indices(10), {0,1,2,3,4,5,6,7,8,9,10});
}
