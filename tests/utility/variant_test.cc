/// variant_gtest.cpp
/// Google Test conversion of range-v3 variant test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <type_traits>

#include <fermat/utility/variant.h>               /// fermat::ranges::variant, fermat::ranges::bad_variant_access
#include <fermat/functional/overload.h>          /// fermat::ranges::overload
#include <fermat/algorithm/equal.h>        /// fermat::ranges::equal
#include <fermat/view/concat.h>            /// views::concat
#include <fermat/view/partial_sum.h>       /// views::partial_sum
#include <fermat/view/transform.h>         /// views::transform
#include <fermat/numeric/accumulate.h>     /// fermat::ranges::accumulate

using namespace fermat::ranges;
/// ------------------------------------------------------------
/// MoveOnlyString helper (as in original)
/// ------------------------------------------------------------
struct MoveOnlyString
{
    std::string s;
    MoveOnlyString() = default;
    MoveOnlyString(const char* c) : s(c) {}
    MoveOnlyString(MoveOnlyString&&) = default;
    MoveOnlyString& operator=(MoveOnlyString&&) = default;
    bool operator==(const MoveOnlyString& other) const { return s == other.s; }
    bool operator!=(const MoveOnlyString& other) const { return !(*this == other); }
};

/// ------------------------------------------------------------
/// Helper: check_equal for ranges (simplified)
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

/// ------------------------------------------------------------
/// Bug 1217 test (compiles only)
/// ------------------------------------------------------------
void bug_1217() {
    std::vector<int> vec;
    if (auto tx = vec | fermat::ranges::views::transform([](int) { return 0; })) {
        auto positions_visited = fermat::ranges::views::concat(tx, tx) | fermat::ranges::views::partial_sum;
        fermat::ranges::accumulate(positions_visited, 0);
    }
}

/// ------------------------------------------------------------
/// Test cases
/// ------------------------------------------------------------

TEST(VariantTest, BasicVariantAndAccess) {
    using fermat::ranges::variant;
    using fermat::ranges::get;
    using fermat::ranges::bad_variant_access;

    variant<int, short> v;
    EXPECT_EQ(v.index(), 0u);
    auto v2 = v;
    EXPECT_EQ(v2.index(), 0u);
    v.emplace<1>(short(2));
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(get<1>(v), short(2));
    EXPECT_THROW(get<0>(v), bad_variant_access);
    v = v2;
    EXPECT_EQ(v.index(), 0u);
}

TEST(VariantTest, VariantOfVoid) {
    using fermat::ranges::variant;
    using fermat::ranges::get;
    using fermat::ranges::bad_variant_access;

    variant<void, void> v;
    EXPECT_EQ(v.index(), 0u);
    v.emplace<0>();
    EXPECT_EQ(v.index(), 0u);
    // get<0> returns void, so it's fine to call
    v.index() == 0 ? void() : get<0>(v);  // no effect, just compile
    v.emplace<1>();
    EXPECT_EQ(v.index(), 1u);
    EXPECT_THROW(get<0>(v), bad_variant_access);
}

TEST(VariantTest, VariantOfReferences) {
    using fermat::ranges::variant;
    using fermat::ranges::get;
    using fermat::ranges::emplaced_index;

    int i = 42;
    std::string s = "hello world";
    variant<int&, std::string&> v{emplaced_index<0>, i};
    // default_constructible trait not checked here; just runtime
    EXPECT_EQ(v.index(), 0u);
    EXPECT_EQ(get<0>(v), 42);
    EXPECT_EQ(&get<0>(v), &i);
    const auto& cv = v;
    get<0>(cv) = 24;
    EXPECT_EQ(i, 24);
    v.emplace<1>(s);
    EXPECT_EQ(v.index(), 1u);
    EXPECT_EQ(get<1>(v), "hello world");
    EXPECT_EQ(&get<1>(v), &s);
    get<1>(cv) = "goodbye";
    EXPECT_EQ(s, "goodbye");
}

TEST(VariantTest, MoveTest1) {
    using fermat::ranges::variant;
    using fermat::ranges::get;
    using fermat::ranges::emplaced_index;

    variant<int, MoveOnlyString> v{emplaced_index<1>, "hello world"};
    EXPECT_EQ(get<1>(v), "hello world");
    MoveOnlyString s = get<1>(std::move(v));
    EXPECT_EQ(s, "hello world");
    EXPECT_EQ(get<1>(v), "");
    v.emplace<1>("goodbye");
    EXPECT_EQ(get<1>(v), "goodbye");
    auto v2 = std::move(v);
    EXPECT_EQ(get<1>(v2), "goodbye");
    EXPECT_EQ(get<1>(v), "");
    v = std::move(v2);
    EXPECT_EQ(get<1>(v), "goodbye");
    EXPECT_EQ(get<1>(v2), "");
}

TEST(VariantTest, MoveTest2) {
    using fermat::ranges::variant;
    using fermat::ranges::get;
    using fermat::ranges::emplaced_index;

    MoveOnlyString s = "hello world";
    variant<MoveOnlyString&> v{emplaced_index<0>, s};
    EXPECT_EQ(get<0>(v), "hello world");
    MoveOnlyString& s2 = get<0>(std::move(v));
    EXPECT_EQ(&s2, &s);
}

TEST(VariantTest, ApplyTest1) {
    using fermat::ranges::variant;
    using fermat::ranges::overload;

    std::stringstream sout;
    variant<int, std::string> v{emplaced_index<1>, "hello"};
    auto fun = overload(
        [&sout](int&) { sout << "int"; },
        [&sout](std::string&) -> int { sout << "string"; return 42; }
    );
    variant<void, int> x = v.visit(fun);
    EXPECT_EQ(sout.str(), "string");
    EXPECT_EQ(x.index(), 1u);
    EXPECT_EQ(fermat::ranges::get<1>(x), 42);
}

TEST(VariantTest, ApplyTest2) {
    using fermat::ranges::variant;
    using fermat::ranges::overload;

    std::stringstream sout;
    std::string s = "hello";
    const variant<int, std::string&> v{emplaced_index<1>, s};
    auto fun = overload(
        [&sout](int const&) { sout << "int"; },
        [&sout](std::string&) -> int { sout << "string"; return 42; }
    );
    variant<void, int> x = v.visit(fun);
    EXPECT_EQ(sout.str(), "string");
    EXPECT_EQ(x.index(), 1u);
    EXPECT_EQ(fermat::ranges::get<1>(x), 42);
}

TEST(VariantTest, ConstexprVariant) {
    using fermat::ranges::variant;
    constexpr variant<int, short> v{emplaced_index<1>, short(2)};
    static_assert(v.index() == 1, "");
    static_assert(v.valid(), "");
}

TEST(VariantTest, VariantAndArrays) {
    using fermat::ranges::variant;
    using fermat::ranges::get;

    // Test with array of int[5]
    variant<int[5], std::vector<int>> v{emplaced_index<0>, {1,2,3,4,5}};
    int (&rgi)[5] = get<0>(v);
    check_equal(rgi, {1,2,3,4,5});

    variant<int[5], std::vector<int>> v2{emplaced_index<0>, {}};
    int (&rgi2)[5] = get<0>(v2);
    check_equal(rgi2, {0,0,0,0,0});

    v2 = v;
    check_equal(rgi2, {1,2,3,4,5});

    // Test with non‑default‑constructible element type
    struct T {
        T() = delete;
        T(int) {}
        T(T const&) = default;
        T& operator=(T const&) = default;
    };
    variant<T[5]> vrgt{emplaced_index<0>, {T{42}, T{42}, T{42}, T{42}, T{42}}};
    (void)vrgt;   // just ensure it compiles
}

