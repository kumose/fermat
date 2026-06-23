/// common_type_gtest.cpp
/// Google Test conversion of range-v3 common_type / common_reference tests.
/// All comments in English, using /// Doxygen style.

#include <gtest/gtest.h>
#include <utility>
#include <type_traits>

#include <fermat/utility/common_type.h>   /// Fermat provides fermat::ranges::common_type, common_reference
#include <fermat/utility/common_tuple.h>  /// For common_pair

struct B {};
struct D : B {};

struct noncopyable
{
    noncopyable() = default;
    noncopyable(noncopyable const &) = delete;
    noncopyable(noncopyable &&) = default;
    noncopyable &operator=(noncopyable const &) = delete;
    noncopyable &operator=(noncopyable &&) = default;
};

struct noncopyable2 : noncopyable {};

struct X {};
struct Y {};
struct Z {};

/// Custom common_type specialization for X and Y must be placed in `concepts` namespace
/// because Fermat's common_type is an alias to concepts::common_type.
namespace concepts {
    template<>
    struct common_type<X, Y> { using type = Z; };
    template<>
    struct common_type<Y, X> { using type = Z; };
}

/// Also need to specialize basic_common_reference for X& and Y const & to get Z.
/// This is required for common_reference_t<X&, Y const &> to work.
namespace concepts {
    template<template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<X, Y, Qual1, Qual2>
    {
        using type = Z;
    };
    template<template<typename> class Qual1, template<typename> class Qual2>
    struct basic_common_reference<Y, X, Qual1, Qual2>
    {
        using type = Z;
    };
}

/// Helper to test conversion to int
template<typename T>
struct ConvTo { operator T(); };

/// Validate that common_type works with ConvTo<int> and int
static_assert(std::is_same<
    fermat::ranges::common_type_t<ConvTo<int>, int>,
    int
>::value, "common_type with ConvTo should yield int");

/// Test suite for common_reference and common_type
TEST(CommonTypeTest, CommonReferenceWithInheritance) {
    using namespace fermat::ranges;

    static_assert(std::is_same<common_reference_t<B &, D &>, B &>::value, "");
    static_assert(std::is_same<common_reference_t<B &, D const &>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B &, D const &, D &>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B const &, D &>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B &, D &, B &, D &>, B &>::value, "");

    static_assert(std::is_same<common_reference_t<B &&, D &&>, B &&>::value, "");
    static_assert(std::is_same<common_reference_t<B const &&, D &&>, B const &&>::value, "");
    static_assert(std::is_same<common_reference_t<B &&, D const &&>, B const &&>::value, "");

    static_assert(std::is_same<common_reference_t<B &, D &&>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B &, D const &&>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B const &, D &&>, B const &>::value, "");

    static_assert(std::is_same<common_reference_t<B &&, D &>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B &&, D const &>, B const &>::value, "");
    static_assert(std::is_same<common_reference_t<B const &&, D &>, B const &>::value, "");
}

TEST(CommonTypeTest, FundamentalTypes) {
    using namespace fermat::ranges;

    static_assert(std::is_same<common_reference_t<int, short>, int>::value, "");
    static_assert(std::is_same<common_reference_t<int, short &>, int>::value, "");
    static_assert(std::is_same<common_reference_t<int &, short &>, int>::value, "");
    static_assert(std::is_same<common_reference_t<int &, short>, int>::value, "");
}

TEST(CommonTypeTest, VolatileReferences) {
    using namespace fermat::ranges;

    static_assert(std::is_same<common_reference_t<int &&, int volatile &>, int>::value, "");
    static_assert(std::is_same<common_reference_t<int volatile &, int &&>, int>::value, "");
    static_assert(std::is_same<common_reference_t<int const volatile &&, int volatile &&>, int const volatile &&>::value, "");
    static_assert(std::is_same<common_reference_t<int &&, int const &, int volatile &>, int const volatile &>::value, "");
}

TEST(CommonTypeTest, Arrays) {
    using namespace fermat::ranges;

    static_assert(std::is_same<common_reference_t<int (&)[10], int (&&)[10]>, int const(&)[10]>::value, "");
    static_assert(std::is_same<common_reference_t<int const (&)[10], int volatile (&)[10]>, int const volatile(&)[10]>::value, "");
    static_assert(std::is_same<common_reference_t<int (&)[10], int (&)[11]>, int *>::value, "");
}

TEST(CommonTypeTest, CommonPairWithCommonReference) {
    using namespace fermat::ranges;

    /// Test common_reference between std::pair and common_pair
    static_assert(std::is_same<
        common_reference_t<std::pair<int &, int &>, common_pair<int,int> const &>,
        common_pair<int const &, int const &>
    >::value, "");

#if !defined(__GNUC__) || __GNUC__ != 4 || __GNUC_MINOR__ > 8
    static_assert(std::is_same<
        common_reference_t<common_pair<int const &, int const &>, std::pair<int, int>>,
        common_pair<int, int>
    >::value, "");

    using builtin_common = common_reference_t<common_pair<int, int> const &, std::pair<int, int> &>;
    static_assert(std::is_same<builtin_common, std::pair<int, int> const &>::value, "");
#endif

    static_assert(std::is_same<
        common_reference_t<common_pair<int, int> const &, std::pair<int, int> &>,
        std::pair<int, int> const &
    >::value, "");
}

TEST(CommonTypeTest, Noncopyable) {
    using namespace fermat::ranges;

    using t1 = common_reference_t<noncopyable const &, noncopyable>;
    static_assert(std::is_same<t1, noncopyable>::value, "");

    using t2 = common_reference_t<noncopyable2 const &, noncopyable>;
    static_assert(std::is_same<t2, noncopyable>::value, "");

    using t3 = common_reference_t<noncopyable const &, noncopyable2>;
    static_assert(std::is_same<t3, noncopyable>::value, "");
}

TEST(CommonTypeTest, CustomCommonType) {
    using namespace fermat::ranges;

    static_assert(std::is_same<common_reference_t<X &, Y const &>, Z>::value, "");
}

TEST(CommonTypeTest, CommonTypeForCommonPair) {
    using namespace fermat::ranges;

    using CP = common_pair<int, int>;
    static_assert(std::is_same<common_type_t<CP, CP>, CP>::value, "");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}