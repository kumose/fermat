/// intersperse_gtest.cpp
/// Google Test conversion of range-v3 intersperse view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>          /// fermat::ranges::to
#include <fermat/view/intersperse.h>          /// views::intersperse
#include <fermat/view/delimit.h>              /// views::delimit
#include <fermat/view/reverse.h>              /// views::reverse
#include <fermat/view/istream.h>              /// istream<int>

/// ------------------------------------------------------------
/// Helper functions (as in original)
/// ------------------------------------------------------------
template<std::size_t N>
fermat::ranges::subrange<char const*> c_str(char const (&sz)[N])
{
    return {&sz[0], &sz[N-1]};
}

fermat::ranges::delimit_view<fermat::ranges::subrange<char const*, fermat::ranges::unreachable_sentinel_t>, char>
c_str_(char const* sz)
{
    return fermat::ranges::views::delimit(sz, '\0');
}

/// ------------------------------------------------------------
/// debug_input_view: minimal input view for testing
/// ------------------------------------------------------------
template<typename T>
struct debug_input_view : fermat::ranges::view_interface<debug_input_view<T>>
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

namespace fermat::ranges
{
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
/// ------------------------------------------------------------
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
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

TEST(IntersperseTest, CommonRange) {
    using namespace fermat::ranges;

    // abcde
    {
        auto r0 = views::intersperse(c_str("abcde"), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r0), "a,b,c,d,e");
        EXPECT_EQ(r0.size(), 9u);
    }
    // empty
    {
        auto r1 = views::intersperse(c_str(""), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r1), "");
        EXPECT_EQ(r1.size(), 0u);
    }
    // single char
    {
        auto r2 = views::intersperse(c_str("a"), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r2), "a");
        EXPECT_EQ(r2.size(), 1u);
    }
    // two chars
    {
        auto r3 = views::intersperse(c_str("ab"), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r3), "a,b");
        EXPECT_EQ(r3.size(), 3u);
    }
}

TEST(IntersperseTest, CommonRangeReverse) {
    using namespace fermat::ranges;

    // abcde reversed
    {
        auto r0 = views::intersperse(c_str("abcde"), ',') | views::reverse;
        EXPECT_EQ(fermat::ranges::to<std::string>(r0), "e,d,c,b,a");
    }
    // empty
    {
        auto r1 = views::intersperse(c_str(""), ',') | views::reverse;
        EXPECT_EQ(fermat::ranges::to<std::string>(r1), "");
    }
    // single char
    {
        auto r2 = views::intersperse(c_str("a"), ',') | views::reverse;
        EXPECT_EQ(fermat::ranges::to<std::string>(r2), "a");
    }
    // two chars
    {
        auto r3 = views::intersperse(c_str("ab"), ',') | views::reverse;
        EXPECT_EQ(fermat::ranges::to<std::string>(r3), "b,a");
    }
}

TEST(IntersperseTest, NonCommonRange) {
    using namespace fermat::ranges;

    // Using c_str_ (delimit view, not common)
    {
        auto r0 = views::intersperse(c_str_("abcde"), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r0), "a,b,c,d,e");
    }
    {
        auto r1 = views::intersperse(c_str_(""), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r1), "");
    }
    {
        auto r2 = views::intersperse(c_str_("a"), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r2), "a");
    }
    {
        auto r3 = views::intersperse(c_str_("ab"), ',');
        EXPECT_EQ(fermat::ranges::to<std::string>(r3), "a,b");
    }
}

TEST(IntersperseTest, RandomAccessIteration) {
    using namespace fermat::ranges;

    auto r0 = views::intersperse(c_str("abcde"), ',');
    auto it = r0.begin();
    EXPECT_EQ(*(it + 0), 'a');
    EXPECT_EQ(*(it + 1), ',');
    EXPECT_EQ(*(it + 2), 'b');
    EXPECT_EQ(*(it + 3), ',');
    EXPECT_EQ(*(it + 4), 'c');
    EXPECT_EQ(*(it + 5), ',');
    EXPECT_EQ(*(it + 6), 'd');
    EXPECT_EQ(*(it + 7), ',');
    EXPECT_EQ(*(it + 8), 'e');
    EXPECT_EQ(it + 9, r0.end());

    it = r0.end();
    EXPECT_EQ(*(it - 9), 'a');
    EXPECT_EQ(*(it - 8), ',');
    EXPECT_EQ(*(it - 7), 'b');
    EXPECT_EQ(*(it - 6), ',');
    EXPECT_EQ(*(it - 5), 'c');
    EXPECT_EQ(*(it - 4), ',');
    EXPECT_EQ(*(it - 3), 'd');
    EXPECT_EQ(*(it - 2), ',');
    EXPECT_EQ(*(it - 1), 'e');

    it = r0.begin();
    for (int i = 0; i <= 9; ++i) {
        EXPECT_EQ((it + i) - it, i);
        EXPECT_EQ(it - (it + i), -i);
    }
}

TEST(IntersperseTest, IstreamRange) {
    using namespace fermat::ranges;

    std::stringstream str{"1 2 3 4 5"};
    auto r0 = istream<int>(str) | views::intersperse(42);
    check_equal(r0, {1,42,2,42,3,42,4,42,5});
}

TEST(IntersperseTest, DebugInputView) {
    using namespace fermat::ranges;

    int const some_ints[] = {1,2,3,4,5};
    auto rng = debug_input_view<int const>{some_ints, 5} | views::intersperse(42);
    check_equal(rng, {1,42,2,42,3,42,4,42,5});
}
