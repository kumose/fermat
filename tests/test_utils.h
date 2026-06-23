/// test_utils.hpp
/// Google Test conversion of range-v3 test utilities.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#ifndef RANGES_TEST_UTILS_HPP
#define RANGES_TEST_UTILS_HPP

#include <algorithm>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <ostream>

#include <fermat/meta/meta.h>

#include <fermat/iterator/concepts.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/traits.h>
#include <fermat/range/access.h>
#include <fermat/range/concepts.h>
#include <fermat/range/traits.h>
#include <fermat/view/subrange.h>

#include <gtest/gtest.h>

/// ------------------------------------------------------------
/// Source location support (if available)
/// ------------------------------------------------------------
#if defined(__clang__) || defined(__GNUC__)
#if defined(__has_builtin)
#if __has_builtin(__builtin_FILE) && \
    __has_builtin(__builtin_LINE) && \
    __has_builtin(__builtin_FUNCTION)
#define RANGES_CXX_HAS_SLOC_BUILTINS
#endif
#endif
#else
#define RANGES_CXX_HAS_SLOC_BUILTINS
#endif

#if defined(RANGES_CXX_HAS_SLOC_BUILTINS) && defined(__has_include)
#if __has_include(<source_location>)
#include <source_location>
#ifdef __cpp_lib_source_location
#define RANGES_HAS_SLOC 1
using source_location = std::source_location;
#endif
#elif __has_include(<experimental/source_location>)
#include <experimental/source_location>
#if __cpp_lib_experimental_source_location
#define RANGES_HAS_SLOC 1
using source_location = std::experimental::source_location;
#endif
#endif
#endif

#ifndef RANGES_HAS_SLOC
struct source_location
{
    static source_location current() { return {}; }
};
#define CHECK_SLOC(sloc, ...) \
    do { (void)sloc; EXPECT_TRUE(__VA_ARGS__); } while(false)
#else
#define CHECK_SLOC(sloc, ...) CHECK_LINE(sloc.file_name(), (int)sloc.line(), __VA_ARGS__)
#endif

/// ------------------------------------------------------------
/// check_equal function object
/// ------------------------------------------------------------
struct check_equal_fn
{
    // Single value comparison
    template<typename T, typename U,
             typename = std::enable_if_t<!fermat::ranges::input_range<T> && !fermat::ranges::input_range<U>>>
    constexpr void operator()(T&& actual, U&& expected,
                              source_location sloc = source_location::current()) const
    {
        CHECK_SLOC(sloc, (T&&)actual == (U&&)expected);
    }

    // Two-range comparison (both satisfy input_range)
    template<typename Rng1, typename Rng2,
             typename = std::enable_if_t<fermat::ranges::input_range<Rng1> && fermat::ranges::input_range<Rng2>>>
    constexpr void operator()(Rng1&& actual, Rng2&& expected,
                              source_location sloc = source_location::current()) const
    {
        auto it0 = fermat::ranges::begin(actual);
        auto end0 = fermat::ranges::end(actual);
        auto it1 = fermat::ranges::begin(expected);
        auto end1 = fermat::ranges::end(expected);
        for (; it0 != end0 && it1 != end1; ++it0, ++it1)
            (*this)(*it0, *it1, sloc);
        CHECK_SLOC(sloc, it0 == end0);
        CHECK_SLOC(sloc, it1 == end1);
    }

    // Range vs initializer_list
    template<typename Rng, typename Val,
             typename = std::enable_if_t<fermat::ranges::input_range<Rng>>>
    constexpr void operator()(Rng&& actual, std::initializer_list<Val> expected,
                              source_location sloc = source_location::current()) const
    {
        (*this)(actual, expected, sloc);
    }
};

inline namespace function_objects
{
    RANGES_INLINE_VARIABLE(check_equal_fn, check_equal)
}

/// ------------------------------------------------------------
/// has_type: static assertion that a value has given type
/// ------------------------------------------------------------
template<typename Expected, typename Actual>
constexpr void has_type(Actual&&)
{
    static_assert(std::is_same<Expected, Actual>::value, "Not the same");
}

/// ------------------------------------------------------------
/// has_cardinality: check range cardinality
/// ------------------------------------------------------------
template<fermat::ranges::cardinality Expected,
         typename Rng,
         fermat::ranges::cardinality Actual = fermat::ranges::range_cardinality<Rng>::value>
constexpr void has_cardinality(Rng&&)
{
    static_assert(Actual == Expected, "Unexpected cardinality");
}

/// ------------------------------------------------------------
/// Helper: treat an rvalue as lvalue (to avoid C++23 implicit move)
/// ------------------------------------------------------------
template<typename T>
constexpr T& as_lvalue(T&& t)
{
    return static_cast<T&>(t);
}

/// ------------------------------------------------------------
/// function_ref: type‑erased function reference
/// ------------------------------------------------------------
template<typename Sig>
struct function_ref;

template<typename Ret, typename... Args>
struct function_ref<Ret(Args...)>
{
private:
    void const* data_{nullptr};
    Ret (*pfun_)(void const*, Args...){nullptr};

    template<typename Fun>
    static Ret apply_(void const* data, Args... args)
    {
        return (*static_cast<Fun const*>(data))(args...);
    }

public:
    function_ref() = default;
    template<typename T>
    function_ref(T const& t)
      : data_(&t)
      , pfun_(&apply_<T>)
    {}
    Ret operator()(Args... args) const
    {
        return (*pfun_)(data_, args...);
    }
};

/// ------------------------------------------------------------
/// checker: utility for algorithm testing
/// ------------------------------------------------------------
template<typename T>
struct checker
{
private:
    std::function<void(function_ref<void(T)>)> algo_;

public:
    explicit checker(std::function<void(function_ref<void(T)>)> algo)
      : algo_(std::move(algo))
    {}
    void check(function_ref<void(T)> const& check) const
    {
        algo_(check);
    }
};

/// ------------------------------------------------------------
/// rvalue_if: conditionally return an rvalue reference
/// ------------------------------------------------------------
template<bool B, typename T>
meta::if_c<B, T, T const&> rvalue_if(T const& t)
{
    return t;
}

/// ------------------------------------------------------------
/// test_range_algo_1: test algorithms with iterator pairs
/// ------------------------------------------------------------
template<typename Algo, bool RvalueOK = false>
struct test_range_algo_1
{
private:
    Algo algo_;

    template<typename I, typename... Rest>
    static auto _impl(Algo algo, I first, I last, Rest&&... rest)
        -> checker<decltype(algo(first, last, rest...))>
    {
        using S = meta::_t<sentinel_type<I>>;
        using R = decltype(algo(first, last, rest...));
        auto check_algo = [algo, first, last, rest...](
                              function_ref<void(R)> const& check) {
            check(algo(first, last, rest...));
            check(algo(first, S{base(last)}, rest...));
            check(algo(::rvalue_if<RvalueOK>(fermat::ranges::make_subrange(first, last)), rest...));
            check(algo(::rvalue_if<RvalueOK>(fermat::ranges::make_subrange(first, S{base(last)})),
                       rest...));
        };
        return checker<R>{check_algo};
    }

public:
    explicit test_range_algo_1(Algo algo) : algo_(algo) {}

    template<typename I>
    auto operator()(I first, I last) const -> checker<decltype(algo_(first, last))>
    {
        return _impl(algo_, first, last);
    }
    template<typename I, typename T>
    auto operator()(I first, I last, T t) const -> checker<decltype(algo_(first, last, t))>
    {
        return _impl(algo_, first, last, t);
    }
    template<typename I, typename T, typename U>
    auto operator()(I first, I last, T t, U u) const
        -> checker<decltype(algo_(first, last, t, u))>
    {
        return _impl(algo_, first, last, t, u);
    }
    template<typename I, typename T, typename U, typename V>
    auto operator()(I first, I last, T t, U u, V v) const
        -> checker<decltype(algo_(first, last, t, u, v))>
    {
        return _impl(algo_, first, last, t, u, v);
    }
};

template<bool RvalueOK = false, typename Algo>
test_range_algo_1<Algo, RvalueOK> make_testable_1(Algo algo)
{
    return test_range_algo_1<Algo, RvalueOK>{algo};
}

/// ------------------------------------------------------------
/// test_range_algo_2: test algorithms with two iterator ranges
/// ------------------------------------------------------------
template<typename Algo, bool RvalueOK1 = false, bool RvalueOK2 = false>
struct test_range_algo_2
{
private:
    Algo algo_;

public:
    explicit test_range_algo_2(Algo algo) : algo_(algo) {}

    template<typename I1, typename I2, typename... Rest>
    auto operator()(I1 begin1, I1 end1, I2 begin2, I2 end2, Rest&&... rest) const
        -> checker<decltype(algo_(begin1, end1, begin2, end2, rest...))>
    {
        using S1 = meta::_t<sentinel_type<I1>>;
        using S2 = meta::_t<sentinel_type<I2>>;
        using R = decltype(algo_(begin1, end1, begin2, end2, rest...));
        return checker<R>{[algo = algo_, begin1, end1, begin2, end2, rest...](
                              function_ref<void(R)> const& check) {
            check(algo(begin1, end1, begin2, end2, rest...));
            check(algo(begin1, S1{base(end1)}, begin2, S2{base(end2)}, rest...));
            check(algo(::rvalue_if<RvalueOK1>(fermat::ranges::make_subrange(begin1, end1)),
                       ::rvalue_if<RvalueOK2>(fermat::ranges::make_subrange(begin2, end2)),
                       rest...));
            check(algo(::rvalue_if<RvalueOK1>(fermat::ranges::make_subrange(begin1, S1{base(end1)})),
                       ::rvalue_if<RvalueOK2>(fermat::ranges::make_subrange(begin2, S2{base(end2)})),
                       rest...));
        }};
    }
};

template<bool RvalueOK1 = false, bool RvalueOK2 = false, typename Algo>
test_range_algo_2<Algo, RvalueOK1, RvalueOK2> make_testable_2(Algo algo)
{
    return test_range_algo_2<Algo, RvalueOK1, RvalueOK2>{algo};
}

/// ------------------------------------------------------------
/// MoveOnlyString: helper for move‑only tests
/// ------------------------------------------------------------
struct MoveOnlyString
{
    char const* sz_;

    MoveOnlyString(char const* sz = "") : sz_(sz) {}
    MoveOnlyString(MoveOnlyString&& that) : sz_(that.sz_)
    {
        that.sz_ = "";
    }
    MoveOnlyString(MoveOnlyString const&) = delete;
    MoveOnlyString& operator=(MoveOnlyString&& that)
    {
        sz_ = that.sz_;
        that.sz_ = "";
        return *this;
    }
    MoveOnlyString& operator=(MoveOnlyString const&) = delete;

    bool operator==(MoveOnlyString const& that) const
    {
        return std::strcmp(sz_, that.sz_) == 0;
    }
    bool operator<(MoveOnlyString const& that) const
    {
        return std::strcmp(sz_, that.sz_) < 0;
    }
    bool operator!=(MoveOnlyString const& that) const
    {
        return !(*this == that);
    }
    friend std::ostream& operator<<(std::ostream& sout, MoveOnlyString const& str)
    {
        return sout << '"' << str.sz_ << '"';
    }
};

#endif // RANGES_TEST_UTILS_HPP
