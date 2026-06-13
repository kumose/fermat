/// all_gtest.cpp
/// Google Test conversion of range-v3 all view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/view/all.h>

/// A minimal debug input view for testing (replaces range-v3's debug_input_view).
/// It inherits from view_base to satisfy view_ concept.
template<typename T>
struct debug_input_view : fermat::ranges::view_interface<debug_input_view<T>> {
    struct data {
        const T* first_;
        std::size_t size_;
    };
    std::shared_ptr<data> data_;

    debug_input_view() = default;
    explicit debug_input_view(const T* first, std::size_t size)
        : data_(std::make_shared<data>(data{first, size})) {}

    const T* begin() const { return data_->first_; }
    const T* end() const { return data_->first_ + data_->size_; }
    std::size_t size() const { return data_->size_; }
};

namespace fermat::ranges {
    /// specialize enable_borrowed_range for debug_input_view
    template<typename T>
    inline constexpr bool enable_borrowed_range<::debug_input_view<T>> = true;
}

/// Helper to get array size (in global namespace, used only when unambiguous).
template<typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) { return N; }

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(AllViewTest, BasicArraysAndContainers) {
    using namespace fermat::ranges;

    int rgi[] = {1, 1, 1, 2, 3, 4, 4};
    std::vector<int> vi(begin(rgi), end(rgi));
    std::list<int> li(begin(rgi), end(rgi));

    ref_view<int[7]> x = views::all(rgi);
    ref_view<std::vector<int>> y = views::all(vi);
    ref_view<std::list<int>> z = views::all(li);

    static_assert(sized_range<decltype(x)> && view_<decltype(x)>, "");
    static_assert(sized_range<decltype(y)> && view_<decltype(y)>, "");
    static_assert(sized_range<decltype(z)> && view_<decltype(z)>, "");

    x = views::all(x);
    y = views::all(y);
    z = views::all(z);

    EXPECT_EQ(x.size(), 7u);
    EXPECT_EQ(y.size(), 7u);
    EXPECT_EQ(z.size(), 7u);
}

TEST(AllViewTest, DebugInputView) {
    using namespace fermat::ranges;

    int rgi[] = {1, 1, 1, 2, 3, 4, 4};
    constexpr std::size_t n = sizeof(rgi) / sizeof(rgi[0]);
    auto v = views::all(debug_input_view<int>{rgi, n});
    EXPECT_EQ(v.size(), n);
    EXPECT_EQ(v.data_->first_, rgi);

    auto v2 = views::all(views::all(views::all(std::move(v))));
    // views::all on a view should return the same view type.
    static_assert(std::is_same<decltype(v), decltype(v2)>::value, "");
    EXPECT_EQ(v2.size(), n);
    EXPECT_EQ(v2.data_->first_, rgi);
}
