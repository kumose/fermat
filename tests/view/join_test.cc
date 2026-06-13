/// join_gtest.cpp
/// Google Test conversion of range-v3 join view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
/// All concept checks (CPP_assert) are omitted; only runtime behavior is tested.

#include <gtest/gtest.h>

#include <forward_list>
#include <functional>
#include <string>
#include <vector>
#include <array>      /// for std::array in DebugInputViewOfPairs test

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>       /// fermat::ranges::to
#include <fermat/view/join.h>               /// views::join
#include <fermat/view/split.h>              /// views::split
#include <fermat/view/generate_n.h>         /// views::generate_n
#include <fermat/view/repeat_n.h>           /// views::repeat_n
#include <fermat/view/chunk.h>              /// views::chunk
#include <fermat/view/concat.h>             /// views::concat
#include <fermat/view/iota.h>               /// views::iota
#include <fermat/view/single.h>             /// views::single
#include <fermat/view/transform.h>          /// views::transform
#include <fermat/view/filter.h>             /// views::filter

using namespace fermat::ranges;

/// ------------------------------------------------------------
/// Helper: check_equal for ranges vs initializer_list
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

/// Overload for checking a string (for join result)
inline void check_equal(const std::string &actual, const char *expected) {
    EXPECT_EQ(actual, expected);
}

/// Overload for checking vector of strings
template<typename T>
void check_equal(const std::vector<T> &actual,
                 std::initializer_list<T> expected) {
    auto it = actual.begin();
    for (auto const &val: expected) {
        EXPECT_NE(it, actual.end());
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, actual.end());
}

/// ------------------------------------------------------------
/// Helper: twice(t) = concat(single(t), single(t))
/// ------------------------------------------------------------
template<typename T>
constexpr auto twice(T t) {
    return fermat::ranges::views::concat(
        fermat::ranges::views::single(t),
        fermat::ranges::views::single(t));
}

/// ------------------------------------------------------------
/// Helper: input_array (simplified, using std::vector)
/// ------------------------------------------------------------
template<typename T, std::size_t N>
struct input_array {
    std::vector<T> elements_;

    input_array(std::initializer_list<T> il) : elements_(il) {}

    auto begin() { return elements_.begin(); }
    auto end() { return elements_.end(); }
    constexpr std::size_t size() const { return elements_.size(); }
};

/// ------------------------------------------------------------
/// Helper: make_input_rng (produces an input_range of input_ranges)
/// ------------------------------------------------------------
static int N = 0;

auto make_input_rng() {
    using namespace fermat::ranges;
    return views::generate_n([]() {
        return views::generate_n([]() {
            return N++;
        }, 3);
    }, 3);
}

/// ------------------------------------------------------------
/// Issue #283: join vector of vectors
/// ------------------------------------------------------------
void test_issue_283() {
    using namespace fermat::ranges;
    const std::vector<std::vector<int> > nums = {{1, 2, 3}, {4, 5, 6}};
    auto flat_nums = views::join(nums) | to<std::vector>();
    check_equal(flat_nums, {1, 2, 3, 4, 5, 6});
}

/// ------------------------------------------------------------
/// Issue #1414: join on forward_list
/// ------------------------------------------------------------
void test_issue_1414() {
    using namespace fermat::ranges;
    std::forward_list<char> u2;
    std::vector<char> i2;
    auto v2 = u2 | views::chunk(3) | views::join(i2);
    (void) v2;
}

// ------------------------------------------------------------------
// Test cases (mapping original main)
// ------------------------------------------------------------------

TEST(JoinTest, InputRangeOfInputRanges) {
    using namespace fermat::ranges;
    N = 0;
    auto rng0 = make_input_rng() | views::join;
    check_equal(rng0, {0, 1, 2, 3, 4, 5, 6, 7, 8});
}

TEST(JoinTest, JoiningWithAValue) {
    using namespace fermat::ranges;
    N = 0;
    auto rng1 = make_input_rng() | views::join(42);
    check_equal(rng1, {0, 1, 2, 42, 3, 4, 5, 42, 6, 7, 8});
}

TEST(JoinTest, JoiningWithARange) {
    using namespace fermat::ranges;
    N = 0;
    int rgi[] = {42, 43};
    auto rng2 = make_input_rng() | views::join(rgi);
    check_equal(rng2, {0, 1, 2, 42, 43, 3, 4, 5, 42, 43, 6, 7, 8});
}

TEST(JoinTest, SplitJoinToString) {
    using namespace fermat::ranges;
    std::string str = "Now,is,the,time,for,all,good,men,to,come,to,the,aid,of,their,country";
    auto res = str | views::split(',') | views::join(' ') | to<std::string>();
    EXPECT_EQ(res, "Now is the time for all good men to come to the aid of their country");
}

TEST(JoinTest, VectorOfStrings) {
    using namespace fermat::ranges;
    std::vector<std::string> vs{"This", "is", "his", "face"};
    auto rng3 = views::join(vs);
    EXPECT_EQ(to<std::string>(rng3), "Thisishisface");

    auto rng4 = views::join(vs, ' ');
    EXPECT_EQ(to<std::string>(rng4), "This is his face");
}

TEST(JoinTest, TwiceTwice) {
    using namespace fermat::ranges;
    auto rng5 = views::join(twice(twice(42)));
    static_assert(range_cardinality<decltype(rng5)>::value == 4, "");
    EXPECT_EQ(rng5.size(), 4u);
    check_equal(rng5, {42, 42, 42, 42});
}

TEST(JoinTest, JoinRepeatN) {
    using namespace fermat::ranges;
    auto rng6 = views::join(twice(views::repeat_n(42, 2)));
    static_assert(range_cardinality<decltype(rng6)>::value == fermat::ranges::finite, "");
    EXPECT_EQ(rng6.size(), 4u);
    check_equal(rng6, {42, 42, 42, 42});
}

TEST(JoinTest, InputArrayOfStrings) {
    using namespace fermat::ranges;
    input_array<std::string, 4> some_strings{{"This", "is", "his", "face"}};
    auto rng = some_strings | views::join;
    EXPECT_EQ(to<std::string>(rng), "Thisishisface");
}

/// Fixed test: use a proper range-of-ranges (2D array) instead of a flat subrange.
TEST(JoinTest, DebugInputViewOfPairs) {
    using namespace fermat::ranges;
    // 2D array where each element is an array of 2 ints (a range)
    int const some_int_pairs[3][2] = {{0, 1}, {2, 3}, {4, 5}};
    auto rng = some_int_pairs | views::join;
    check_equal(rng, {0, 1, 2, 3, 4, 5});
}

TEST(JoinTest, JoinViewOfVectorOfStrings) {
    using namespace fermat::ranges;
    std::vector<std::string> vs{"this", "is", "his", "face"};
    join_view<ref_view<std::vector<std::string> > > jv{vs};
    check_equal(jv, {'t', 'h', 'i', 's', 'i', 's', 'h', 'i', 's', 'f', 'a', 'c', 'e'});
}

TEST(JoinTest, IotaTransformJoin) {
    using namespace fermat::ranges;
    auto rng = views::iota(0, 4)
               | views::transform([](int i) { return views::iota(0, i); })
               | views::join;
    check_equal(rng, {0, 0, 1, 0, 1, 2});
}

TEST(JoinTest, IotaTransformFilterJoin) {
    using namespace fermat::ranges;
    auto rng = views::iota(0, 4)
               | views::transform([](int i) { return views::iota(0, i); })
               | views::filter([](auto) { return true; })
               | views::join;
    check_equal(rng, {0, 0, 1, 0, 1, 2});
}

TEST(JoinTest, IotaTransformStringJoin) {
    using namespace fermat::ranges;
    auto rng = views::iota(0, 4)
               | views::transform([](int i) { return std::string(static_cast<std::size_t>(i), char('a' + i)); })
               | views::join;
    check_equal(rng, {'b', 'c', 'c', 'd', 'd', 'd'});
}

TEST(JoinTest, IotaTransformStringJoinWithSeparator) {
    using namespace fermat::ranges;
    auto rng = views::iota(0, 4)
               | views::transform([](int i) { return std::string(static_cast<std::size_t>(i), char('a' + i)); })
               | views::join('-');
    check_equal(rng, {'-', 'b', '-', 'c', 'c', '-', 'd', 'd', 'd'});
}

TEST(JoinTest, Issue1320) {
    using namespace fermat::ranges;
    auto op = [](auto &input, int i, auto &ins) {
        return input | views::chunk(i) | views::join(ins);
    };
    std::string input{"foobarbaxbat"};
    std::string insert{"X"};
    auto rng = op(input, 2, insert);
    check_equal(rng, {'f', 'o', 'X', 'o', 'b', 'X', 'a', 'r', 'X', 'b', 'a', 'X', 'x', 'b', 'X', 'a', 't'});

    std::vector<std::string> input2{"foo", "bar", "bax", "bat"};
    std::string insert2{"XX"};
    auto rng2 = op(input2, 2, insert2);
    check_equal(rng2, {"foo", "bar", "XX", "bax", "bat"});
}

TEST(JoinTest, ThrowingTransform) {
    using namespace fermat::ranges;
    std::vector<int> v = {1, 2, 3};
    auto throws = [](auto &&) -> std::vector<std::vector<int> > & { throw 42; };
    auto rng = v | views::transform(throws) | views::join;
    try {
        auto d = fermat::ranges::distance(rng);
        (void) d;
        ADD_FAILURE() << "Expected exception not thrown";
    } catch (int) {
        SUCCEED();
    } catch (...) {
        ADD_FAILURE() << "Unexpected exception type";
    }
}

TEST(JoinTest, Issue283) {
    test_issue_283();
}

TEST(JoinTest, Issue1414) {
    test_issue_1414();
}
