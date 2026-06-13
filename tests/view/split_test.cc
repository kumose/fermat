/// split_gtest.cpp
/// Google Test conversion of range-v3 split / split_when view tests.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/iterator/operations.h>
#include <fermat/view/split.h>
#include <fermat/view/split_when.h>
#include <fermat/view/c_str.h>
#include <fermat/view/counted.h>
#include <fermat/view/remove_if.h>
#include <fermat/view/empty.h>
#include <fermat/view/single.h>
#include <fermat/utility/copy.h>
#include <fermat/algorithm/equal.h>

/// ------------------------------------------------------------
/// ForwardIterator: a minimal forward iterator for testing
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

/// Overload for checking a range against another range (used for subrange)
template<typename Rng1, typename Rng2>
void check_equal(Rng1&& rng1, Rng2&& rng2) {
    EXPECT_TRUE(ranges::equal(rng1, rng2));
}

/// Helper: c_str returns a subrange from a null‑terminated string literal.
template<std::size_t N>
ranges::subrange<char const*> c_str(char const (&sz)[N]) {
    return {&sz[0], &sz[N-1]};
}

/// Helper: starts_with_g predicate for split_when
struct starts_with_g {
    template<typename I, typename S>
    std::pair<bool, I> operator()(I b, S) const {
        if (*b == 'g')
            return {true, b};
        else
            return {false, b};
    }
};

/// ------------------------------------------------------------
/// Test cases (converted from original main)
/// ------------------------------------------------------------

TEST(SplitTest, SplitStringByChar) {
    using namespace ranges;

    std::string str("Now is the time for all good men to come to the aid of their country.");
    auto rng = views::split(str, ' ');
    EXPECT_EQ(ranges::distance(rng), 16);
    auto it = begin(rng);
    // The original uses c_str("Now") etc., which returns a subrange.
    // We'll compare using ranges::equal.
    EXPECT_TRUE(ranges::equal(*next(it, 0), c_str("Now")));
    EXPECT_TRUE(ranges::equal(*next(it, 1), c_str("is")));
    EXPECT_TRUE(ranges::equal(*next(it, 2), c_str("the")));
    EXPECT_TRUE(ranges::equal(*next(it, 3), c_str("time")));
    EXPECT_TRUE(ranges::equal(*next(it, 4), c_str("for")));
    EXPECT_TRUE(ranges::equal(*next(it, 5), c_str("all")));
    EXPECT_TRUE(ranges::equal(*next(it, 6), c_str("good")));
    EXPECT_TRUE(ranges::equal(*next(it, 7), c_str("men")));
    EXPECT_TRUE(ranges::equal(*next(it, 8), c_str("to")));
    EXPECT_TRUE(ranges::equal(*next(it, 9), c_str("come")));
    EXPECT_TRUE(ranges::equal(*next(it, 10), c_str("to")));
    EXPECT_TRUE(ranges::equal(*next(it, 11), c_str("the")));
    EXPECT_TRUE(ranges::equal(*next(it, 12), c_str("aid")));
    EXPECT_TRUE(ranges::equal(*next(it, 13), c_str("of")));
    EXPECT_TRUE(ranges::equal(*next(it, 14), c_str("their")));
    EXPECT_TRUE(ranges::equal(*next(it, 15), c_str("country.")));
}

TEST(SplitTest, SplitStringByStringPattern) {
    using namespace ranges;

    std::string str("Now is the time for all good men to come to the aid of their country.");
    auto rng = views::split(str, c_str(" "));
    EXPECT_EQ(ranges::distance(rng), 16);
    auto it = begin(rng);
    EXPECT_TRUE(ranges::equal(*next(it, 0), c_str("Now")));
    EXPECT_TRUE(ranges::equal(*next(it, 1), c_str("is")));
    EXPECT_TRUE(ranges::equal(*next(it, 2), c_str("the")));
    EXPECT_TRUE(ranges::equal(*next(it, 3), c_str("time")));
    EXPECT_TRUE(ranges::equal(*next(it, 4), c_str("for")));
    EXPECT_TRUE(ranges::equal(*next(it, 5), c_str("all")));
    EXPECT_TRUE(ranges::equal(*next(it, 6), c_str("good")));
    EXPECT_TRUE(ranges::equal(*next(it, 7), c_str("men")));
    EXPECT_TRUE(ranges::equal(*next(it, 8), c_str("to")));
    EXPECT_TRUE(ranges::equal(*next(it, 9), c_str("come")));
    EXPECT_TRUE(ranges::equal(*next(it, 10), c_str("to")));
    EXPECT_TRUE(ranges::equal(*next(it, 11), c_str("the")));
    EXPECT_TRUE(ranges::equal(*next(it, 12), c_str("aid")));
    EXPECT_TRUE(ranges::equal(*next(it, 13), c_str("of")));
    EXPECT_TRUE(ranges::equal(*next(it, 14), c_str("their")));
    EXPECT_TRUE(ranges::equal(*next(it, 15), c_str("country.")));
}

TEST(SplitTest, SplitWhenWithPredicate) {
    using namespace ranges;

    std::string str("Now is the time for all ggood men to come to the aid of their country.");
    auto rng = views::split_when(str, starts_with_g{});
    EXPECT_EQ(ranges::distance(rng), 3);
    auto it = begin(rng);
    EXPECT_TRUE(ranges::equal(*it, c_str("Now is the time for all ")));
    EXPECT_TRUE(ranges::equal(*next(it), c_str("g")));
    EXPECT_TRUE(ranges::equal(*next(it, 2), c_str("good men to come to the aid of their country.")));
}

TEST(SplitTest, SplitWhenWithCountedForwardIterator) {
    using namespace ranges;

    std::string str("Now is the time for all ggood men to come to the aid of their country.");
    ForwardIterator<std::string::iterator> i{str.begin()};
    auto rng = views::counted(i, str.size()) | views::split_when(starts_with_g{});
    EXPECT_EQ(ranges::distance(rng), 3);
    auto it = begin(rng);
    EXPECT_TRUE(ranges::equal(*it, c_str("Now is the time for all ")));
    EXPECT_TRUE(ranges::equal(*next(it), c_str("g")));
    EXPECT_TRUE(ranges::equal(*next(it, 2), c_str("good men to come to the aid of their country.")));
}

TEST(SplitTest, SplitByEmptyPattern) {
    using namespace ranges;

    std::string str("meow");
    auto rng = views::split(str, views::empty<char>);
    EXPECT_EQ(ranges::distance(rng), 4);
    auto it = begin(rng);
    EXPECT_TRUE(ranges::equal(*next(it, 0), c_str("m")));
    EXPECT_TRUE(ranges::equal(*next(it, 1), c_str("e")));
    EXPECT_TRUE(ranges::equal(*next(it, 2), c_str("o")));
    EXPECT_TRUE(ranges::equal(*next(it, 3), c_str("w")));
}

TEST(SplitTest, SplitAfterRemoveIf) {
    using namespace ranges;

    int a[] = {0, 2, 3, 1, 4, 5, 1, 6, 7};
    auto rng = a | views::remove_if([](int i) { return i % 2 == 0; });
    auto srng = views::split(rng, 1);
    EXPECT_EQ(ranges::distance(srng), 3);
    auto it = begin(srng);
    check_equal(*it, {3});
    check_equal(*next(it), {5});
    check_equal(*next(it, 2), {7});
}

TEST(SplitTest, SplitWhenWithIsspace) {
    using namespace ranges;

    std::string str("now  is \t the\ttime");
    auto rng = views::split_when(str, static_cast<int(*)(int)>(&std::isspace));
    EXPECT_EQ(ranges::distance(rng), 4);
    auto it = begin(rng);
    EXPECT_TRUE(ranges::equal(*it, c_str("now")));
    EXPECT_TRUE(ranges::equal(*next(it), c_str("is")));
    EXPECT_TRUE(ranges::equal(*next(it, 2), c_str("the")));
    EXPECT_TRUE(ranges::equal(*next(it, 3), c_str("time")));
}

// Regression test for https://stackoverflow.com/questions/49015671
TEST(SplitTest, CStrSplitByChar) {
    using namespace ranges;
    auto const str = "quick brown fox";
    auto rng = views::c_str(str) | views::split(' ');
    static_assert(forward_range<decltype(rng)>, "");
    (void)rng;
    SUCCEED();
}

// Regression test for #986
TEST(SplitTest, EmptyStringSplitWhen) {
    using namespace ranges;
    std::string s;
    auto rng = s | views::split_when([](char) { return true; });
    (void)rng;
    SUCCEED();
}

TEST(SplitTest, MoarTests) {
    using namespace ranges;

    std::string greeting = "now is the time";
    std::string pattern = " ";

    // split with pattern string
    {
        auto sv = views::split(greeting, pattern);
        auto i = sv.begin();
        check_equal(*i, {'n','o','w'});
        ++i;
        EXPECT_NE(i, sv.end());
        check_equal(*i, {'i','s'});
        ++i;
        EXPECT_NE(i, sv.end());
        check_equal(*i, {'t','h','e'});
        ++i;
        EXPECT_NE(i, sv.end());
        check_equal(*i, {'t','i','m','e'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }

    // split with character
    {
        auto sv = views::split(greeting, ' ');
        auto i = sv.begin();
        check_equal(*i, {'n','o','w'});
        ++i;
        EXPECT_NE(i, sv.end());
        check_equal(*i, {'i','s'});
        ++i;
        EXPECT_NE(i, sv.end());
        check_equal(*i, {'t','h','e'});
        ++i;
        EXPECT_NE(i, sv.end());
        check_equal(*i, {'t','i','m','e'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }

    // split on input stream (input range)
    {
        std::stringstream sin{greeting};
        auto rng = subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
        auto sv = views::split(rng, ' ');
        auto i = sv.begin();
        check_equal(*i, {'n','o','w'});
        ++i;
        check_equal(*i, {'i','s'});
        ++i;
        check_equal(*i, {'t','h','e'});
        ++i;
        check_equal(*i, {'t','i','m','e'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }

    // split with empty string handling
    {
        std::string list{"eggs,milk,,butter"};
        auto sv = views::split(list, ',');
        auto i = sv.begin();
        check_equal(*i, {'e','g','g','s'});
        ++i;
        check_equal(*i, {'m','i','l','k'});
        ++i;
        check_equal(*i, views::empty<char>);
        ++i;
        check_equal(*i, {'b','u','t','t','e','r'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }

    // split on input stream with empty field
    {
        std::string list{"eggs,milk,,butter"};
        std::stringstream sin{list};
        auto rng = subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
        auto sv = rng | views::split(',');
        auto i = sv.begin();
        check_equal(*i, {'e','g','g','s'});
        ++i;
        check_equal(*i, {'m','i','l','k'});
        ++i;
        check_equal(*i, views::empty<char>);
        ++i;
        check_equal(*i, {'b','u','t','t','e','r'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }

    // split by empty pattern (on string)
    {
        std::string hello("hello");
        auto sv = views::split(hello, views::empty<char>);
        auto i = sv.begin();
        check_equal(*i, {'h'});
        ++i;
        check_equal(*i, {'e'});
        ++i;
        check_equal(*i, {'l'});
        ++i;
        check_equal(*i, {'l'});
        ++i;
        check_equal(*i, {'o'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }

    // split by empty pattern on input stream
    {
        std::string hello("hello");
        std::stringstream sin{hello};
        auto rng = subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
        auto sv = views::split(rng, views::empty<char>);
        auto i = sv.begin();
        check_equal(*i, {'h'});
        ++i;
        check_equal(*i, {'e'});
        ++i;
        check_equal(*i, {'l'});
        ++i;
        check_equal(*i, {'l'});
        ++i;
        check_equal(*i, {'o'});
        ++i;
        EXPECT_EQ(i, sv.end());
    }
}

// Regression test for #1041
TEST(SplitTest, SplitWhenEscapeSequence) {
    using namespace ranges;

    auto is_escape = [](auto first, auto last) {
        return std::make_pair(ranges::next(first) != last, first);
    };

    auto escapes = views::split_when(views::c_str(R"(\t)"), is_escape);
    static_assert(forward_range<decltype(escapes)>, "");
    auto const first = begin(escapes);
    EXPECT_NE(first, end(escapes));
    EXPECT_NE(first, next(first));
}
