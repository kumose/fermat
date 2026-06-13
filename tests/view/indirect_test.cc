/// indirect_gtest.cpp
/// Google Test conversion of range-v3 indirect view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <memory>
#include <utility>

#include <fermat/range/access.h>            /// ranges::begin, ranges::end
#include <fermat/range/primitives.h>        /// ranges::size
#include <fermat/view/indirect.h>           /// views::indirect
#include <fermat/view/transform.h>          /// views::transform

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

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(IndirectTest, SharedPtrVector) {
    using namespace ranges;

    std::vector<std::shared_ptr<int>> vp;
    for (int i = 0; i < 10; ++i)
        vp.push_back(std::make_shared<int>(i));

    auto rng = vp | views::indirect;
    EXPECT_EQ(&*ranges::begin(rng), vp[0].get());
    check_equal(rng, {0,1,2,3,4,5,6,7,8,9});
}

TEST(IndirectTest, PointerArray) {
    using namespace ranges;

    int const some_ints[] = {0,1,2,3};
    int const* some_int_pointers[] = {
        some_ints + 0, some_ints + 1, some_ints + 2, some_ints + 3
    };
    auto make_range = [&]{
        return debug_input_view<int const*>{some_int_pointers, 4} | views::indirect;
    };
    auto rng = make_range();
    check_equal(rng, {0,1,2,3});
    rng = make_range();
    EXPECT_EQ(&*ranges::begin(rng), some_ints + 0);
}

#if RANGES_CXX_RETURN_TYPE_DEDUCTION >= RANGES_CXX_RETURN_TYPE_DEDUCTION_14
/// Regression test for #946
TEST(IndirectTest, MemberFunctionReturnTypeDeduction) {
    class Data;

    struct Test {
        std::vector<Data*> m_list;
        auto list() {
            return m_list | ranges::views::indirect;
        }
    };

    class Data {};

    Test test{std::vector<Data*>(42)};
    EXPECT_EQ(test.list().size(), 42u);
}
#endif // RANGES_CXX_RETURN_TYPE_DEDUCTION

/// Regression test for #952
TEST(IndirectTest, TransformThenIndirect) {
    using namespace ranges;

    int some_ints[42]{};
    auto a = some_ints | views::transform([](int& i) { return &i; })
                      | views::indirect;
    // Just ensure it compiles and can be iterated.
    auto it = a.begin();
    (void)it;
    SUCCEED();
}
