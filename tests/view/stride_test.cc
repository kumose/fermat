/// stride_gtest.cpp
/// Google Test conversion of range-v3 stride view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <sstream>
#include <memory>
#include <iterator>                     // for ostream_iterator, back_inserter

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/iterator/stream_iterators.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/equal.h>     // for ranges::equal
#include <fermat/view/stride.h>
#include <fermat/view/reverse.h>
#include <fermat/view/iota.h>
#include <fermat/view/move.h>
#include <fermat/view/istream.h>
#include <fermat/view/partial_sum.h>    // for views::partial_sum
#include <fermat/numeric/accumulate.h>
#include <fermat/numeric/iota.h>
#include <fermat/utility/copy.h>

/// ------------------------------------------------------------
/// ForwardIterator (minimal, as in test_iterators.hpp)
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

/// Overload for comparing two ranges (used for reverse)
template<typename Rng1, typename Rng2>
void check_equal(Rng1&& rng1, Rng2&& rng2) {
    EXPECT_TRUE(ranges::equal(rng1, rng2));
}

/// ------------------------------------------------------------
/// Bug 1291: ensure compilation (no runtime check)
/// ------------------------------------------------------------
void bug_1291() {
    std::vector<int> vec;
    auto tx = vec | ranges::views::stride(2) | ranges::views::partial_sum;
    ranges::accumulate(tx, 0);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(StrideTest, VectorStride3) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    auto rng = v | views::stride(3);
    // Runtime checks only
    auto rev = rng | views::reverse;
    check_equal(rev, {48, 45, 42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12, 9, 6, 3, 0});
}

TEST(StrideTest, IstreamStride3) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    std::stringstream str;
    // Use std::copy to avoid any fermat copy concept issues with ostream_iterator
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(str, " "));

    auto rng = istream<int>(str) | views::stride(3);
    check_equal(rng, {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48});
}

TEST(StrideTest, ListStride3) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    std::list<int> li;
    copy(v, back_inserter(li));

    auto rng = li | views::stride(3);
    check_equal(rng, {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48});

    auto rev = rng | views::reverse;
    check_equal(rev, {48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3,0});
}

TEST(StrideTest, DistanceAndIteratorDifference) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    auto x2 = v | views::stride(3);
    EXPECT_EQ(ranges::distance(x2), 17);

    auto it0 = x2.begin();
    auto it1 = ranges::next(it0, 10);
    EXPECT_EQ(it1 - it0, 10);
    EXPECT_EQ(it0 - it1, -10);
    EXPECT_EQ(it0 - it0, 0);
    EXPECT_EQ(it1 - it1, 0);
}

TEST(StrideTest, MoveAndStride) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    const auto n = 4;
    auto rng = v | views::move | views::stride(2);
    EXPECT_EQ(ranges::next(begin(rng), n) - begin(rng), n);
}

TEST(StrideTest, StrideWithIntegerLiteral) {
    // Regression test #368 – just ensure it compiles.
    int n = 42;
    (void)ranges::views::stride(n);
    SUCCEED();
}

TEST(StrideTest, DebugInputViewStride2) {
    using namespace ranges;

    int const some_ints[] = {0,1,2,3,4,5,6,7};
    auto rng = debug_input_view<int const>{some_ints, 8} | views::stride(2);
    check_equal(rng, {0,2,4,6});
}

TEST(StrideTest, ConstSubrangeStride3) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    std::list<int> li;
    copy(v, back_inserter(li));

    subrange<std::list<int>::const_iterator> tmp{li.begin(), li.end()};
    auto rng = tmp | views::stride(3);
    check_equal(rng, {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48});
    check_equal(rng | views::reverse, {48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3,0});
}

TEST(StrideTest, SizedSubrangeStride3) {
    using namespace ranges;

    std::vector<int> v(50);
    iota(v, 0);

    std::list<int> li;
    copy(v, back_inserter(li));

    using CLI = std::list<int>::const_iterator;
    subrange<CLI, CLI, subrange_kind::sized> tmp{li};
    auto rng = tmp | views::stride(3);
    EXPECT_EQ(*--rng.end(), 48);
    check_equal(rng, {0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48});
    check_equal(rng | views::reverse, {48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3,0});
}

TEST(StrideTest, IotaStrideEvenlyDivisible) {
    using namespace ranges;

    auto r = views::iota(0, 12);
    auto strided = r | views::stride(3);
    check_equal(strided, {0,3,6,9});
    EXPECT_EQ(strided.size(), 4u);
    EXPECT_EQ(strided.front(), 0);
    EXPECT_EQ(strided[0], 0);
    EXPECT_EQ(strided.back(), 9);
    EXPECT_EQ(strided[3], 9);
    EXPECT_EQ(strided[strided.size() - 1], 9);
}

TEST(StrideTest, IotaStrideNotEvenlyDivisible) {
    using namespace ranges;

    auto r = views::iota(0, 12);
    auto strided = r | views::stride(5);
    check_equal(strided, {0,5,10});
    EXPECT_EQ(strided.size(), 3u);
    EXPECT_EQ(strided.front(), 0);
    EXPECT_EQ(strided[0], 0);
    EXPECT_EQ(strided.back(), 10);
    EXPECT_EQ(strided[2], 10);
    EXPECT_EQ(strided[strided.size() - 1], 10);
}
