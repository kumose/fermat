/// const_gtest.cpp
/// Google Test conversion of range-v3 const view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <functional>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/const.h>
#include <fermat/view/counted.h>
#include <fermat/view/zip.h>
#include <fermat/view/move.h>
#include <fermat/utility/common_tuple.h>
#include <fermat/iterator/operations.h>
#include <fermat/utility/copy.h>

/// ------------------------------------------------------------
/// ForwardIterator: minimal forward iterator (only ++, ==, !=, *)
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
/// has_type: static assertion that a value has given type
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
void has_type(Actual&&) {
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing
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

/// Helper to create debug_input_view from array (deduces size)
template<typename T, std::size_t N>
debug_input_view<T> make_debug_input_view(const T (&arr)[N])
{
    return debug_input_view<T>{arr, static_cast<std::ptrdiff_t>(N)};
}

namespace ranges
{
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list (single elements)
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected)
{
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Overload for ranges of pairs
template<typename Rng, typename U, typename V>
void check_equal(Rng&& rng, std::initializer_list<std::pair<U, V>> expected)
{
    auto it = ranges::begin(rng);
    auto end = ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        // Use (*it).first instead of it->first to avoid proxy issues
        EXPECT_EQ((*it).first, val.first);
        EXPECT_EQ((*it).second, val.second);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ConstViewTest, RawArray)
{
    using namespace ranges;

    int rgi[] = {1, 2, 3, 4};
    auto rng = rgi | views::const_;

    has_type<int&>(*begin(rgi));
    has_type<int const&>(*begin(rng));
    static_assert(same_as<range_rvalue_reference_t<decltype(rng)>, int const&&>, "");
    static_assert(view_<decltype(rng)>, "");
    static_assert(common_range<decltype(rng)>, "");
    static_assert(sized_range<decltype(rng)>, "");
    static_assert(random_access_range<decltype(rng)>, "");

    check_equal(rng, {1,2,3,4});
    EXPECT_EQ(&*begin(rng), &rgi[0]);
    EXPECT_EQ(rng.size(), 4u);
}

TEST(ConstViewTest, CountedForwardIterator)
{
    using namespace ranges;

    int rgi[] = {1, 2, 3, 4};
    // Use ForwardIterator to keep the original test semantics
    auto rng2 = views::counted(ForwardIterator<int*>(rgi), 4) | views::const_;

    has_type<int const&>(*begin(rng2));
    static_assert(same_as<range_rvalue_reference_t<decltype(rng2)>, int const&&>, "");
    static_assert(view_<decltype(rng2)>, "");
    static_assert(forward_range<decltype(rng2)>, "");
    static_assert(!bidirectional_range<decltype(rng2)>, "");
    static_assert(!common_range<decltype(rng2)>, "");
    static_assert(sized_range<decltype(rng2)>, "");

    check_equal(rng2, {1,2,3,4});
    EXPECT_EQ(&*begin(rng2), &rgi[0]);
    EXPECT_EQ(rng2.size(), 4u);
}

TEST(ConstViewTest, ZipOfRawArray)
{
    using namespace ranges;

    int rgi[] = {1, 2, 3, 4};
    auto zip = views::zip(rgi, rgi);
    auto rng3 = zip | views::const_;

    has_type<common_pair<int&, int&>>(*begin(zip));
    has_type<common_pair<int&&, int&&>>(iter_move(begin(zip)));
    has_type<common_pair<int const&, int const&>>(*begin(rng3));
    has_type<common_pair<int const&&, int const&&>>(iter_move(begin(rng3)));

    static_assert(view_<decltype(rng3)>, "");
    static_assert(random_access_range<decltype(rng3)>, "");
    static_assert(common_range<decltype(rng3)>, "");
    static_assert(sized_range<decltype(rng3)>, "");

    using P = std::pair<int,int>;
    check_equal(rng3, {P{1,1}, P{2,2}, P{3,3}, P{4,4}});
    EXPECT_EQ(&(*begin(rng3)).first, &rgi[0]);
    EXPECT_EQ(rng3.size(), 4u);
}

TEST(ConstViewTest, MoveZipThenConst)
{
    using namespace ranges;

    int rgi[] = {1, 2, 3, 4};
    auto zip2 = views::zip(rgi, rgi) | views::move;
    auto rng4 = zip2 | views::const_;

    has_type<common_pair<int&&, int&&>>(*begin(zip2));
    has_type<common_pair<int&&, int&&>>(iter_move(begin(zip2)));
    has_type<common_pair<int const&&, int const&&>>(*begin(rng4));
    has_type<common_pair<int const&&, int const&&>>(iter_move(begin(rng4)));

    static_assert(view_<decltype(rng4)>, "");
    static_assert(random_access_range<decltype(rng4)>, "");
    static_assert(common_range<decltype(rng4)>, "");
    static_assert(sized_range<decltype(rng4)>, "");

    using P = std::pair<int,int>;
    check_equal(rng4, {P{1,1}, P{2,2}, P{3,3}, P{4,4}});
    EXPECT_EQ(&(*begin(rng4)).first, &rgi[0]);
    EXPECT_EQ(rng4.size(), 4u);
}

TEST(ConstViewTest, DebugInputView)
{
    using namespace ranges;

    int rgi[] = {1, 2, 3, 4};
    auto dv = make_debug_input_view(rgi);
    auto rng = dv | views::const_;

    static_assert(std::is_same<range_reference_t<decltype(rng)>, int const&>::value, "");
    check_equal(rng, {1,2,3,4});
}
