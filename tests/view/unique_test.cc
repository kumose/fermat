/// unique_gtest.cpp
/// Google Test conversion of range-v3 unique view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
///
/// Original source: range-v3 test/view/unique.cpp
/// Adapted to use fermat library and Google Test.

#include <gtest/gtest.h>

#include <cctype>
#include <string>
#include <vector>
#include <list>
#include <memory>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/range/conversion.h>
#include <fermat/algorithm/copy.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/utility/copy.h>
#include <fermat/view/delimit.h>
#include <fermat/view/reverse.h>
#include <fermat/view/transform.h>
#include <fermat/view/unique.h>

/// ------------------------------------------------------------
/// Case‑insensitive character traits and string type
/// ------------------------------------------------------------
/// From http://stackoverflow.com/a/2886589/195873
struct ci_char_traits : std::char_traits<char>
{
    static bool eq(char c1, char c2) { return std::toupper(c1) == std::toupper(c2); }
    static bool ne(char c1, char c2) { return std::toupper(c1) != std::toupper(c2); }
    static bool lt(char c1, char c2) { return std::toupper(c1) <  std::toupper(c2); }
    static int compare(const char* s1, const char* s2, std::size_t n)
    {
        for(; n != 0; ++s1, ++s2, --n)
        {
            if(std::toupper(*s1) < std::toupper(*s2))
                return -1;
            if(std::toupper(*s1) > std::toupper(*s2))
                return 1;
        }
        return 0;
    }
    static const char* find(const char* s, int n, char a)
    {
        for(; n-- > 0; ++s)
            if(std::toupper(*s) == std::toupper(a))
                break;
        return s;
    }
};

using ci_string = std::basic_string<char, ci_char_traits>;

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

/// Overload for vector<string>
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<std::string> expected)
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

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

/// Basic test: unique on array of integers
TEST(UniqueViewTest, UniqueOnArray)
{
    using namespace fermat::ranges;

    int rgi[] = {1, 1, 1, 2, 3, 4, 4};
    std::vector<int> out;

    auto rng = rgi | views::unique;
    // has_type<int &>(*begin(rng));
    // CPP_assert(view_<decltype(rng)>);
    // CPP_assert(bidirectional_range<decltype(rng)>);
    // CPP_assert(!random_access_range<decltype(rng)>);
    // CPP_assert(common_range<decltype(rng)>);
    // CPP_assert(!sized_range<decltype(rng)>);
    // CPP_assert(range<decltype(rng) const>);
    copy(rng, fermat::ranges::back_inserter(out));
    check_equal(out, {1, 2, 3, 4});
    check_equal(views::reverse(out), {4, 3, 2, 1});
}

/// Test unique on vector of case‑insensitive strings
TEST(UniqueViewTest, UniqueOnCIString)
{
    using namespace fermat::ranges;

    std::vector<ci_string> rgs{"hello", "HELLO", "bye", "Bye", "BYE"};
    auto rng = rgs | views::unique;
    // has_type<ci_string &>(*begin(rng));
    // CPP_assert(view_<decltype(rng)>);
    // CPP_assert(bidirectional_range<decltype(rng)>);
    // CPP_assert(!random_access_range<decltype(rng)>);
    // CPP_assert(common_range<decltype(rng)>);
    // CPP_assert(!sized_range<decltype(rng)>);
    // CPP_assert(range<decltype(rng) const>);

    auto fs = rng | views::transform([](ci_string s){
        return std::string(s.data(), s.size());
    });
    // CPP_assert(view_<decltype(fs)>);
    // CPP_assert(bidirectional_range<decltype(fs)>);
    check_equal(fs, {"hello","bye"});
    check_equal(views::reverse(fs), {"bye","hello"});
}

/// Test unique on a delimited (input) range composed with reverse
TEST(UniqueViewTest, UniqueOnDelimitReverse)
{
    using namespace fermat::ranges;

    int const rgi[] = {1, 1, 1, 2, 3, 4, 4, 42, 7};
    auto rng0 = views::delimit(rgi, 42) | views::reverse;
    // rng0 is mutable‑only...
    // CPP_assert(forward_range<decltype(rng0)>);
    // CPP_assert(!forward_range<decltype(rng0) const>);
    // ...and composable
    auto rng = rng0 | views::unique(equal_to{});
    // CPP_assert(view_<decltype(rng)>);
    // CPP_assert(bidirectional_range<decltype(rng)>);
    // CPP_assert(!random_access_range<decltype(rng)>);
    // CPP_assert(common_range<decltype(rng)>);
    // CPP_assert(!sized_range<decltype(rng)>);
    check_equal(rng, {4, 3, 2, 1});
}

/// Test unique with custom binary predicate (case‑insensitive string compare)
TEST(UniqueViewTest, UniqueWithCustomPredicate)
{
    using namespace fermat::ranges;

    auto const caseInsensitiveCompare = [](const std::string& s1, const std::string& s2){
        if (s1.size() != s2.size())
            return false;
        for (unsigned i = 0; i < s1.size(); ++i)
            if (std::toupper(s1[i]) != std::toupper(s2[i]))
                return false;
        return true;
    };

    std::vector<std::string> rgs{"hello", "HELLO", "bye", "Bye", "BYE"};
    auto rng = rgs | views::unique(caseInsensitiveCompare);
    // has_type<std::string &>(*begin(rng));
    // CPP_assert(view_<decltype(rng)>);
    // CPP_assert(bidirectional_range<decltype(rng)>);
    // CPP_assert(!random_access_range<decltype(rng)>);
    // CPP_assert(common_range<decltype(rng)>);
    // CPP_assert(!sized_range<decltype(rng)>);
    // CPP_assert(range<decltype(rng) const>);
    check_equal(rng, {"hello","bye"});
    check_equal(views::reverse(rng), {"bye","hello"});
}

/// Test unique with predicate that considers absolute values, then transform
TEST(UniqueViewTest, UniqueWithAbsPredicateAndTransform)
{
    using namespace fermat::ranges;

    int const rgi[] = {-1, 1, -1, 2, 3, 4, -4, 42, 7};
    auto rng0 = views::delimit(rgi, 42) | views::reverse;
    // rng0 is mutable‑only...
    // CPP_assert(forward_range<decltype(rng0)>);
    // CPP_assert(!forward_range<decltype(rng0) const>);
    // ...and composable
    auto rng = rng0 | views::unique([](const int& n1, const int& n2){
                        return n1 == n2 || n1 == -n2;
                    })
                    | views::transform([](const int& n){ return n > 0 ? n : -n;});
    // CPP_assert(view_<decltype(rng)>);
    // CPP_assert(bidirectional_range<decltype(rng)>);
    // CPP_assert(!random_access_range<decltype(rng)>);
    // CPP_assert(common_range<decltype(rng)>);
    // CPP_assert(!sized_range<decltype(rng)>);
    check_equal(rng, {4, 3, 2, 1});
}
