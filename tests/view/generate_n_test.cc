/// generate_n_gtest.cpp
/// Google Test conversion of range-v3 generate_n view test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <utility>
#include <memory>
#include <string>
#include <cstring>

#include <fermat/range/access.h>
#include <fermat/range/primitives.h>
#include <fermat/view/generate_n.h>
#include <fermat/view/drop_exactly.h>

/// ------------------------------------------------------------
/// MoveOnlyString (as in original test)
/// ------------------------------------------------------------
struct MoveOnlyString {
    std::string s_;
    MoveOnlyString() = default;
    MoveOnlyString(const char* sz) : s_(sz) {}
    MoveOnlyString(MoveOnlyString&&) = default;
    MoveOnlyString& operator=(MoveOnlyString&&) = default;
    MoveOnlyString(const MoveOnlyString&) = delete;
    MoveOnlyString& operator=(const MoveOnlyString&) = delete;

    bool operator==(const MoveOnlyString& other) const { return s_ == other.s_; }
    bool operator!=(const MoveOnlyString& other) const { return !(*this == other); }
    bool operator==(const char* sz) const { return s_ == sz; }
    bool operator!=(const char* sz) const { return !(*this == sz); }
    friend std::ostream& operator<<(std::ostream& os, const MoveOnlyString& s) {
        return os << s.s_;
    }
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

/// Overload for MoveOnlyString
template<typename Rng>
void check_equal(Rng&& rng, std::initializer_list<MoveOnlyString> expected) {
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

TEST(GenerateNTest, ConstantGenerator) {
    using namespace ranges;

    int i = 0, j = 1;
    auto fib = views::generate_n([&]() -> int {
        int tmp = i;
        i += j;
        std::swap(i, j);
        return tmp;
    }, 10);

    check_equal(fib, {0,1,1,2,3,5,8,13,21,34});
}

TEST(GenerateNTest, MutableOnlyGenerator) {
    using namespace ranges;

    int i = 0, j = 1;
    auto fib = views::generate_n([=]() mutable -> int {
        int tmp = i;
        i += j;
        std::swap(i, j);
        return tmp;
    }, 10);

    check_equal(fib, {0,1,1,2,3,5,8,13,21,34});
    // const version does not model view (concept check omitted)
}

TEST(GenerateNTest, MoveOnlyTypes) {
    using namespace ranges;

    char str[] = "gi";
    auto rng = views::generate_n([&] {
        str[0]++;
        return MoveOnlyString{str};
    }, 2);

    auto it = rng.begin();
    EXPECT_TRUE(*it == MoveOnlyString{"hi"});
    EXPECT_TRUE(*it == MoveOnlyString{"hi"});
    EXPECT_TRUE(*rng.begin() == MoveOnlyString{"hi"});
    EXPECT_TRUE(*rng.begin() == MoveOnlyString{"hi"});
    check_equal(rng, {MoveOnlyString{"hi"}, MoveOnlyString{"ii"}});
    static_assert(std::is_same<ranges::range_reference_t<decltype(rng)>, MoveOnlyString&&>::value,
                  "reference type mismatch");
}

TEST(GenerateNTest, InternalReference) {
    using namespace ranges;

    int i = 42;
    auto rng = views::generate_n([i] { return &i; }, 2);
    auto rng2 = std::move(rng);
    auto it = rng2.begin();
    auto p = *it;
    auto p2 = *++it;
    EXPECT_EQ(p, p2);
}

TEST(GenerateNTest, CallCount) {
    using namespace ranges;

    int i = 0;
    auto rng = views::generate_n([&i] { return ++i; }, 2);
    auto rng2 = std::move(rng);
    auto it = rng2.begin();
    EXPECT_EQ(i, 0);
    EXPECT_EQ(*it, 1);
    EXPECT_EQ(i, 1);
    ++it;
    EXPECT_EQ(i, 1);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(i, 2);
}

TEST(GenerateNTest, EmptyMethod) {
    using namespace ranges;

    int i = 0;
    auto rng = views::generate_n([&i] { return ++i; }, 2);
    EXPECT_FALSE(rng.empty());
}

TEST(GenerateNTest, SkipPastPositions) {
    using namespace ranges;

    auto fib = [p = std::make_pair(0, 1)]() mutable -> int {
        auto a = p.first;
        p = {p.second, p.first + p.second};
        return a;
    };

    auto rng = views::generate_n(fib, 8)
             | views::drop_exactly(3);

    check_equal(rng, {2,3,5,8,13});
}
