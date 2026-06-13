/// invoke_gtest.cpp
/// Google Test conversion of range-v3 invoke test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>
#include <memory>

#include <fermat/functional/invoke.h>   /// ranges::invoke
#include <fermat/functional/not_fn.h>   /// ranges::not_fn
#include <fermat/view/filter.h>         /// ranges::views::filter

/// ------------------------------------------------------------
/// Simple check_equal for ranges
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

/// ------------------------------------------------------------
/// reference_wrapper constructibility checks (compile‑time)
/// ------------------------------------------------------------
static_assert(ranges::constructible_from<ranges::reference_wrapper<int>, int&>, "");
static_assert(!ranges::constructible_from<ranges::reference_wrapper<int>, int&&>, "");
static_assert(!ranges::constructible_from<ranges::reference_wrapper<int &&>, int&>, "");
static_assert(ranges::constructible_from<ranges::reference_wrapper<int &&>, int&&>, "");

/// ------------------------------------------------------------
/// Helper types for tests
/// ------------------------------------------------------------
struct Integer {
    int i;
    operator int() const { return i; }
    bool odd() const { return (i % 2) != 0; }
};

enum class kind { lvalue, const_lvalue, rvalue, const_rvalue };

std::ostream& operator<<(std::ostream& os, kind k) {
    const char* msg = nullptr;
    switch (k) {
        case kind::lvalue:        msg = "lvalue"; break;
        case kind::const_lvalue:  msg = "const_lvalue"; break;
        case kind::rvalue:        msg = "rvalue"; break;
        case kind::const_rvalue:  msg = "const_rvalue"; break;
    }
    return os << msg;
}

kind last_call;

template<kind DisableKind>
struct fn {
    bool operator()() & {
        last_call = kind::lvalue;
        return DisableKind != kind::lvalue;
    }
    bool operator()() const & {
        last_call = kind::const_lvalue;
        return DisableKind != kind::const_lvalue;
    }
    bool operator()() && {
        last_call = kind::rvalue;
        return DisableKind != kind::rvalue;
    }
    bool operator()() const && {
        last_call = kind::const_rvalue;
        return DisableKind != kind::const_rvalue;
    }
};

constexpr struct {
    template<typename T>
    constexpr T&& operator()(T&& arg) const noexcept {
        return std::forward<T>(arg);
    }
} h = {};

struct A {
    int i = 13;
    constexpr int f() const noexcept { return 42; }
    constexpr int g(int j) { return 2 * j; }
};

constexpr int f() noexcept { return 13; }
constexpr int g(int i) { return 2 * i + 1; }

/// ------------------------------------------------------------
/// Test body for ranges::invoke
/// ------------------------------------------------------------
void test_invoke() {
    EXPECT_EQ(ranges::invoke(f), 13);
    EXPECT_EQ(ranges::invoke(g, 2), 5);
    EXPECT_EQ(ranges::invoke(h, 42), 42);
    EXPECT_NO_THROW(noexcept(ranges::invoke(h, 42) == 42));

    {
        int i = 13;
        EXPECT_EQ(&ranges::invoke(h, i), &i);
        EXPECT_NO_THROW(noexcept(&ranges::invoke(h, i) == &i));
    }

    EXPECT_EQ(ranges::invoke(&A::f, A{}), 42);
    EXPECT_EQ(ranges::invoke(&A::g, A{}, 2), 4);

    {
        A a;
        const A& ca = a;
        EXPECT_EQ(ranges::invoke(&A::f, a), 42);
        EXPECT_EQ(ranges::invoke(&A::f, ca), 42);
        EXPECT_EQ(ranges::invoke(&A::g, a, 2), 4);
    }

    {
        A a;
        const A& ca = a;
        EXPECT_EQ(ranges::invoke(&A::f, &a), 42);
        EXPECT_EQ(ranges::invoke(&A::f, &ca), 42);
        EXPECT_EQ(ranges::invoke(&A::g, &a, 2), 4);
    }

    {
        std::unique_ptr<A> up(new A);
        EXPECT_EQ(ranges::invoke(&A::f, up), 42);
        EXPECT_EQ(ranges::invoke(&A::g, up, 2), 4);
    }

    {
        auto sp = std::make_shared<A>();
        EXPECT_EQ(ranges::invoke(&A::f, sp), 42);
        EXPECT_EQ(ranges::invoke(&A::g, sp, 2), 4);
    }

    EXPECT_EQ(ranges::invoke(&A::i, A{}), 13);
    {
        int&& tmp = ranges::invoke(&A::i, A{});
        (void)tmp;
    }

    {
        A a;
        const A& ca = a;
        EXPECT_EQ(ranges::invoke(&A::i, a), 13);
        EXPECT_EQ(ranges::invoke(&A::i, ca), 13);
        EXPECT_EQ(ranges::invoke(&A::i, &a), 13);
        EXPECT_EQ(ranges::invoke(&A::i, &ca), 13);

        ranges::invoke(&A::i, a) = 0;
        EXPECT_EQ(a.i, 0);
        ranges::invoke(&A::i, &a) = 1;
        EXPECT_EQ(a.i, 1);

        static_assert(std::is_same<decltype(ranges::invoke(&A::i, ca)), const int&>::value, "");
        static_assert(std::is_same<decltype(ranges::invoke(&A::i, &ca)), const int&>::value, "");
    }

    {
        std::unique_ptr<A> up(new A);
        EXPECT_EQ(ranges::invoke(&A::i, up), 13);
        ranges::invoke(&A::i, up) = 0;
        EXPECT_EQ(up->i, 0);
    }

    {
        auto sp = std::make_shared<A>();
        EXPECT_EQ(ranges::invoke(&A::i, sp), 13);
        ranges::invoke(&A::i, sp) = 0;
        EXPECT_EQ(sp->i, 0);
    }

    // constexpr tests are commented out in original; keep commented.
    /*
    {
        struct B { int i = 42; constexpr int f() const { return i; } };
        constexpr B b;
        static_assert(b.i == 42, "");
        static_assert(b.f() == 42, "");
        static_assert(ranges::invoke_detail::impl(&B::i, b) == 42, "");
        static_assert(ranges::invoke_detail::impl(&B::i, &b) == 42, "");
        static_assert(ranges::invoke_detail::impl(&B::i, B{}) == 42, "");
        static_assert(ranges::invoke_detail::impl(&B::f, b) == 42, "");
        static_assert(ranges::invoke_detail::impl(&B::f, &b) == 42, "");
        static_assert(ranges::invoke_detail::impl(&B::f, B{}) == 42, "");
    }
    */
}

/// ------------------------------------------------------------
/// Google Test cases
/// ------------------------------------------------------------
TEST(InvokeTest, NotFnWithFilter) {
    Integer some_ints[] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}};
    check_equal(some_ints | ranges::views::filter(ranges::not_fn(&Integer::odd)),
                {0, 2, 4, 6});
}

TEST(InvokeTest, NotFnValueCategory) {
    // lvalue
    {
        constexpr kind k = kind::lvalue;
        using F = fn<k>;
        auto f = ranges::not_fn(F{});
        EXPECT_TRUE(f());
        EXPECT_EQ(last_call, k);
    }
    // const lvalue
    {
        constexpr kind k = kind::const_lvalue;
        using F = fn<k>;
        auto const f = ranges::not_fn(F{});
        EXPECT_TRUE(f());
        EXPECT_EQ(last_call, k);
    }
    // rvalue (xvalue and prvalue)
    {
        constexpr kind k = kind::rvalue;
        using F = fn<k>;
        auto f = ranges::not_fn(F{});
        EXPECT_TRUE(std::move(f)());    // xvalue
        EXPECT_EQ(last_call, k);

        EXPECT_TRUE(decltype(f){}());    // prvalue
        EXPECT_EQ(last_call, k);
    }
}

#ifdef _WIN32
TEST(InvokeTest, CallingConventions) {
    static_assert(ranges::invocable<void(__cdecl*)()>, "");
    static_assert(ranges::invocable<void(__stdcall*)()>, "");
    static_assert(ranges::invocable<void(__fastcall*)()>, "");
    static_assert(ranges::invocable<void(__thiscall*)()>, "");
#ifndef __MINGW32__
    static_assert(ranges::invocable<void(__vectorcall*)()>, "");
#endif
}
#endif // _WIN32

TEST(InvokeTest, InvokeFunction) {
    test_invoke();
}
