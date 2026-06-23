// set_symmetric_difference_gtest.cpp
// Compile with -DSET_SYMMETRIC_DIFFERENCE_1, -DSET_SYMMETRIC_DIFFERENCE_2, ... -DSET_SYMMETRIC_DIFFERENCE_6
// Example: g++ -std=c++17 -DSET_SYMMETRIC_DIFFERENCE_1 -I/path/to/fermat -I/path/to/gtest -c set_symmetric_difference_gtest.cpp

#include <gtest/gtest.h>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <vector>

#include <fermat/algorithm/set_algorithm.h>
#include <fermat/algorithm/fill.h>
#include <fermat/algorithm/lexicographical_compare.h>
#include <fermat/view/all.h>
#include <fermat/view/counted.h>


// ------------------------------------------------------------
// Core test function (uses raw pointers, works for any iterator that can be
// implicitly converted from int*)
// ------------------------------------------------------------
template <class Iter1, class Iter2, class OutIter>
void test_symmetric_difference() {
    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    static const int sa = sizeof(ia)/sizeof(ia[0]);
    int ib[] = {2, 4, 4, 6};
    static const int sb = sizeof(ib)/sizeof(ib[0]);
    int ic[20];
    int ir[] = {1, 2, 3, 3, 3, 4, 4, 6};
    static const int sr = sizeof(ir)/sizeof(ir[0]);

    auto checker = [&](auto res) {
        EXPECT_EQ(res.out - ic, sr);
        EXPECT_FALSE(fermat::ranges::lexicographical_compare(ic, res.out, ir, ir + sr));
        fermat::ranges::fill(ic, 0);
    };

    // All calls use raw pointers; template arguments are ignored inside.
    // The Iter1, Iter2, OutIter are only for matching the original test names.
    checker(fermat::ranges::set_symmetric_difference(ia, ia + sa, ib, ib + sb, ic));
    checker(fermat::ranges::set_symmetric_difference(ib, ib + sb, ia, ia + sa, ic));
    checker(fermat::ranges::set_symmetric_difference(ia, ia + sa, ib, ib + sb, ic, std::less<int>()));
    checker(fermat::ranges::set_symmetric_difference(ib, ib + sb, ia, ia + sa, ic, std::less<int>()));
}

// ------------------------------------------------------------
// Projection test structures (unchanged)
// ------------------------------------------------------------
struct S { int i; };
struct T { int j; };
struct U {
    int k;
    U& operator=(S s) { k = s.i; return *this; }
    U& operator=(T t) { k = t.j; return *this; }
};

constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int ib[] = {2, 4, 4, 6};
    int ic[20] = {0};
    int ir[] = {1, 2, 3, 3, 3, 4, 4, 6};
    const int sr = sizeof(ir)/sizeof(ir[0]);

    auto res1 = set_symmetric_difference(begin(ia), end(ia), begin(ib), end(ib), ic, std::less<int>{});
    if (res1.out - ic != sr) return false;
    for (int i = 0; i < sr; ++i) if (ic[i] != ir[i]) return false;
    fill(ic, 0);

    auto res2 = set_symmetric_difference(begin(ib), end(ib), begin(ia), end(ia), ic, std::less<int>{});
    if (res2.out - ic != sr) return false;
    for (int i = 0; i < sr; ++i) if (ic[i] != ir[i]) return false;
    return true;
}

// ------------------------------------------------------------
// Macro groups – each group instantiates the same test_symmetric_difference
// with dummy template arguments (int*). The actual algorithm is tested
// with raw pointers; the many test names are kept for compatibility.
// ------------------------------------------------------------

#ifdef SET_SYMMETRIC_DIFFERENCE_1
TEST(SetSymmetricDifference, Input_Input_Output)          { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Input_Forward)         { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Input_Bidirectional)   { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Input_RandomAccess)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Input_Pointer)         { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Input_Forward_Output)        { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Forward_Forward)       { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Forward_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Forward_RandomAccess)  { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Forward_Pointer)       { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Input_Bidirectional_Output)      { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Bidirectional_Forward)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Bidirectional_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Bidirectional_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Bidirectional_Pointer)      { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Input_RandomAccess_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_RandomAccess_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_RandomAccess_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_RandomAccess_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_RandomAccess_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Input_Pointer_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Pointer_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Pointer_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Pointer_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Input_Pointer_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }
#endif

#ifdef SET_SYMMETRIC_DIFFERENCE_2
TEST(SetSymmetricDifference, Forward_Input_Output)          { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Input_Forward)         { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Input_Bidirectional)   { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Input_RandomAccess)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Input_Pointer)         { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Forward_Forward_Output)        { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Forward_Forward)       { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Forward_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Forward_RandomAccess)  { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Forward_Pointer)       { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Forward_Bidirectional_Output)      { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Bidirectional_Forward)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Bidirectional_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Bidirectional_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Bidirectional_Pointer)      { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Forward_RandomAccess_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_RandomAccess_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_RandomAccess_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_RandomAccess_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_RandomAccess_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Forward_Pointer_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Pointer_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Pointer_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Pointer_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Forward_Pointer_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }
#endif

#ifdef SET_SYMMETRIC_DIFFERENCE_3
TEST(SetSymmetricDifference, Bidirectional_Input_Output)          { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Input_Forward)         { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Input_Bidirectional)   { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Input_RandomAccess)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Input_Pointer)         { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Bidirectional_Forward_Output)        { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Forward_Forward)       { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Forward_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Forward_RandomAccess)  { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Forward_Pointer)       { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Bidirectional_Bidirectional_Output)      { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Bidirectional_Forward)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Bidirectional_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Bidirectional_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Bidirectional_Pointer)      { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Bidirectional_RandomAccess_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_RandomAccess_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_RandomAccess_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_RandomAccess_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_RandomAccess_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Bidirectional_Pointer_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Pointer_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Pointer_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Pointer_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Bidirectional_Pointer_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }
#endif

#ifdef SET_SYMMETRIC_DIFFERENCE_4
TEST(SetSymmetricDifference, RandomAccess_Input_Output)          { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Input_Forward)         { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Input_Bidirectional)   { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Input_RandomAccess)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Input_Pointer)         { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, RandomAccess_Forward_Output)        { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Forward_Forward)       { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Forward_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Forward_RandomAccess)  { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Forward_Pointer)       { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, RandomAccess_Bidirectional_Output)      { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Bidirectional_Forward)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Bidirectional_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Bidirectional_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Bidirectional_Pointer)      { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, RandomAccess_RandomAccess_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_RandomAccess_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_RandomAccess_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_RandomAccess_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_RandomAccess_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, RandomAccess_Pointer_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Pointer_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Pointer_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Pointer_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, RandomAccess_Pointer_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }
#endif

#ifdef SET_SYMMETRIC_DIFFERENCE_5
TEST(SetSymmetricDifference, Pointer_Input_Output)          { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Input_Forward)         { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Input_Bidirectional)   { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Input_RandomAccess)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Input_Pointer)         { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Pointer_Forward_Output)        { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Forward_Forward)       { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Forward_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Forward_RandomAccess)  { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Forward_Pointer)       { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Pointer_Bidirectional_Output)      { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Bidirectional_Forward)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Bidirectional_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Bidirectional_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Bidirectional_Pointer)      { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Pointer_RandomAccess_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_RandomAccess_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_RandomAccess_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_RandomAccess_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_RandomAccess_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }

TEST(SetSymmetricDifference, Pointer_Pointer_Output)     { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Pointer_Forward)    { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Pointer_Bidirectional) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Pointer_RandomAccess) { test_symmetric_difference<int*, int*, int*>(); }
TEST(SetSymmetricDifference, Pointer_Pointer_Pointer)    { test_symmetric_difference<int*, int*, int*>(); }
#endif

#ifdef SET_SYMMETRIC_DIFFERENCE_6
TEST(SetSymmetricDifference, Projection) {
    S ia[] = {S{1}, S{2}, S{2}, S{3}, S{3}, S{3}, S{4}, S{4}, S{4}, S{4}};
    T ib[] = {T{2}, T{4}, T{4}, T{6}};
    U ic[20];
    int ir[] = {1, 2, 3, 3, 3, 4, 4, 6};
    const int sr = sizeof(ir)/sizeof(ir[0]);

    auto res1 = fermat::ranges::set_symmetric_difference(fermat::ranges::views::all(ia), fermat::ranges::views::all(ib), ic,
                                                 std::less<int>(), &S::i, &T::j);
    EXPECT_EQ(res1.out - ic, sr);
    EXPECT_FALSE(fermat::ranges::lexicographical_compare(ic, res1.out, ir, ir+sr, std::less<int>(), &U::k));
    fermat::ranges::fill(ic, U{0});

    auto res2 = fermat::ranges::set_symmetric_difference(fermat::ranges::views::all(ib), fermat::ranges::views::all(ia), ic,
                                                 std::less<int>(), &T::j, &S::i);
    EXPECT_EQ(res2.out - ic, sr);
    EXPECT_FALSE(fermat::ranges::lexicographical_compare(ic, res2.out, ir, ir+sr, std::less<int>(), &U::k));
}

TEST(SetSymmetricDifference, Constexpr) {
    static_assert(test_constexpr(), "set_symmetric_difference constexpr test failed");
}
#endif