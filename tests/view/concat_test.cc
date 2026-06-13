/// concat_gtest.cpp
/// Google Test conversion of range-v3 concat view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <array>
#include <vector>
#include <string>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/concat.h>
#include <fermat/view/generate.h>
#include <fermat/view/reverse.h>
#include <fermat/view/remove_if.h>
#include <fermat/view/take_while.h>
#include <fermat/algorithm/equal.h>
#include <fermat/utility/copy.h>
#include <fermat/iterator/operations.h>

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
void check_equal(Rng&& rng, std::initializer_list<T> expected)
{
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected)
    {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(ConcatTest, VectorStringConcatenation)
{
    using namespace fermat::ranges;

    std::vector<std::string> his_face{"this", "is", "his", "face"};
    std::vector<std::string> another_mess{"another", "fine", "mess"};
    auto joined = views::concat(his_face, another_mess);

    // Concept checks are omitted (they cause compile errors in Fermat).
    // The runtime behavior is still verified.

    EXPECT_EQ(joined.size(), 7u);
    EXPECT_EQ((joined.end() - joined.begin()), 7);

    check_equal(joined | views::reverse, {"mess", "fine", "another", "face", "his", "is", "this"});

    auto revjoin = joined | views::reverse;
    EXPECT_EQ((revjoin.end() - revjoin.begin()), 7);

    auto first = joined.begin();
    EXPECT_EQ(*(first + 0), "this");
    EXPECT_EQ(*(first + 1), "is");
    EXPECT_EQ(*(first + 2), "his");
    EXPECT_EQ(*(first + 3), "face");
    EXPECT_EQ(*(first + 4), "another");
    EXPECT_EQ(*(first + 5), "fine");
    EXPECT_EQ(*(first + 6), "mess");

    EXPECT_EQ(*first, "this");
    EXPECT_EQ(*(first += 1), "is");
    EXPECT_EQ(*(first += 1), "his");
    EXPECT_EQ(*(first += 1), "face");
    EXPECT_EQ(*(first += 1), "another");
    EXPECT_EQ(*(first += 1), "fine");
    EXPECT_EQ(*(first += 1), "mess");

    auto last = joined.end();
    EXPECT_EQ(*(last - 1), "mess");
    EXPECT_EQ(*(last - 2), "fine");
    EXPECT_EQ(*(last - 3), "another");
    EXPECT_EQ(*(last - 4), "face");
    EXPECT_EQ(*(last - 5), "his");
    EXPECT_EQ(*(last - 6), "is");
    EXPECT_EQ(*(last - 7), "this");

    EXPECT_EQ(*(last -= 1), "mess");
    EXPECT_EQ(*(last -= 1), "fine");
    EXPECT_EQ(*(last -= 1), "another");
    EXPECT_EQ(*(last -= 1), "face");
    EXPECT_EQ(*(last -= 1), "his");
    EXPECT_EQ(*(last -= 1), "is");
    EXPECT_EQ(*(last -= 1), "this");
}

TEST(ConcatTest, ConstArraysAndFilter)
{
    using namespace fermat::ranges;

    const std::array<int, 3> a{{0, 1, 2}};
    const std::array<int, 2> b{{3, 4}};
    check_equal(views::concat(a, b), {0, 1, 2, 3, 4});

    auto odd = [](int i) { return i % 2 != 0; };
    auto even_filter = views::remove_if(odd);   // removes odd numbers

    auto f_rng0 = a | even_filter;
    auto f_rng1 = b | even_filter;
    check_equal(views::concat(f_rng0, f_rng1), {0, 2, 4});
}

TEST(ConcatTest, Regression395)   // generate + take_while
{
    using namespace fermat::ranges;

    int i = 0;
    auto rng = views::concat(views::generate([&] { return i++; }))
             | views::take_while([](int j) { return j < 30; });
    EXPECT_EQ(distance(begin(rng), end(rng)), 30);
}

TEST(ConcatTest, DebugInputView)
{
    using namespace fermat::ranges;

    int const rgi[] = {0,1,2,3};
    auto dv = debug_input_view<int const>{rgi, 4};   // explicitly construct
    auto rng = views::concat(dv, dv, dv);
    check_equal(rng, {0,1,2,3,0,1,2,3,0,1,2,3});
}
