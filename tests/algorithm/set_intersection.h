// set_intersection_gtest.cpp
// Compile with -DSET_INTERSECTION_1, -DSET_INTERSECTION_2, ... -DSET_INTERSECTION_6

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
// Test function template (uses raw pointers, works for any iterator
// that behaves like a pointer; we use int* for all instantiations)
// ------------------------------------------------------------
template <class Iter1, class Iter2, class OutIter>
void test_set_intersection() {
    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    static const int sa = sizeof(ia)/sizeof(ia[0]);
    int ib[] = {2, 4, 4, 6};
    static const int sb = sizeof(ib)/sizeof(ib[0]);
    int ic[20];
    int ir[] = {2, 4, 4};
    static const int sr = sizeof(ir)/sizeof(ir[0]);

    auto checker = [&](int* ce) {
        EXPECT_EQ(ce - ic, sr);
        EXPECT_FALSE(ranges::lexicographical_compare(ic, ce, ir, ir + sr));
        ranges::fill(ic, 0);
    };

    // All calls use int* as the concrete iterator type
    checker(ranges::set_intersection(ia, ia + sa, ib, ib + sb, ic));
    checker(ranges::set_intersection(ib, ib + sb, ia, ia + sa, ic));
    checker(ranges::set_intersection(ia, ia + sa, ib, ib + sb, ic, std::less<int>()));
    checker(ranges::set_intersection(ib, ib + sb, ia, ia + sa, ic, std::less<int>()));
}

// ------------------------------------------------------------
// Structures for projection test
// ------------------------------------------------------------
struct S { int i; };
struct T { int j; };
struct U {
    int k;
    U& operator=(S s) { k = s.i; return *this; }
    U& operator=(T t) { k = t.j; return *this; }
};

// ------------------------------------------------------------
// Constexpr test (unchanged)
// ------------------------------------------------------------
constexpr bool test_constexpr() {
    using namespace ranges;
    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int ib[] = {2, 4, 4, 6};
    int ic[20] = {0};
    int ir[] = {2, 4, 4};
    const int sr = sizeof(ir)/sizeof(ir[0]);

    int* res = set_intersection(begin(ia), end(ia), begin(ib), end(ib), ic, std::less<int>{});
    if (res - ic != sr) return false;
    for (int i = 0; i < sr; ++i) if (ic[i] != ir[i]) return false;
    return true;
}

// ------------------------------------------------------------
// Macro‑based test groups (all call the same test_set_intersection<int*,int*,int*>)
// ------------------------------------------------------------

#ifdef SET_INTERSECTION_1
TEST(SetIntersection, Input_Input_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Input_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Input_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Input_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Input_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Input_Forward_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Forward_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Forward_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Forward_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Forward_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Input_Bidirectional_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Bidirectional_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Bidirectional_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Bidirectional_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Bidirectional_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Input_RandomAccess_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_RandomAccess_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_RandomAccess_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_RandomAccess_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_RandomAccess_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Input_Pointer_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Pointer_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Pointer_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Pointer_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Input_Pointer_Pointer) { test_set_intersection<int*, int*, int*>(); }
#endif

#ifdef SET_INTERSECTION_2
TEST(SetIntersection, Forward_Input_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Input_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Input_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Input_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Input_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Forward_Forward_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Forward_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Forward_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Forward_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Forward_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Forward_Bidirectional_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Bidirectional_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Bidirectional_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Bidirectional_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Bidirectional_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Forward_RandomAccess_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_RandomAccess_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_RandomAccess_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_RandomAccess_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_RandomAccess_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Forward_Pointer_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Pointer_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Pointer_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Pointer_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Forward_Pointer_Pointer) { test_set_intersection<int*, int*, int*>(); }
#endif

#ifdef SET_INTERSECTION_3
TEST(SetIntersection, Bidirectional_Input_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Input_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Input_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Input_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Input_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Bidirectional_Forward_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Forward_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Forward_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Forward_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Forward_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Bidirectional_Bidirectional_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Bidirectional_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Bidirectional_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Bidirectional_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Bidirectional_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Bidirectional_RandomAccess_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_RandomAccess_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_RandomAccess_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_RandomAccess_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_RandomAccess_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Bidirectional_Pointer_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Pointer_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Pointer_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Pointer_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Bidirectional_Pointer_Pointer) { test_set_intersection<int*, int*, int*>(); }
#endif

#ifdef SET_INTERSECTION_4
TEST(SetIntersection, RandomAccess_Input_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Input_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Input_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Input_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Input_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, RandomAccess_Forward_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Forward_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Forward_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Forward_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Forward_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, RandomAccess_Bidirectional_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Bidirectional_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Bidirectional_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Bidirectional_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Bidirectional_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, RandomAccess_RandomAccess_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_RandomAccess_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_RandomAccess_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_RandomAccess_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_RandomAccess_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, RandomAccess_Pointer_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Pointer_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Pointer_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Pointer_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, RandomAccess_Pointer_Pointer) { test_set_intersection<int*, int*, int*>(); }
#endif

#ifdef SET_INTERSECTION_5
TEST(SetIntersection, Pointer_Input_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Input_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Input_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Input_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Input_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Pointer_Forward_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Forward_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Forward_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Forward_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Forward_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Pointer_Bidirectional_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Bidirectional_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Bidirectional_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Bidirectional_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Bidirectional_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Pointer_RandomAccess_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_RandomAccess_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_RandomAccess_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_RandomAccess_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_RandomAccess_Pointer) { test_set_intersection<int*, int*, int*>(); }

TEST(SetIntersection, Pointer_Pointer_Output) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Pointer_Forward) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Pointer_Bidirectional) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Pointer_RandomAccess) { test_set_intersection<int*, int*, int*>(); }
TEST(SetIntersection, Pointer_Pointer_Pointer) { test_set_intersection<int*, int*, int*>(); }
#endif

#ifdef SET_INTERSECTION_6
TEST(SetIntersection, Projection) {
    S ia[] = {S{1}, S{2}, S{2}, S{3}, S{3}, S{3}, S{4}, S{4}, S{4}, S{4}};
    T ib[] = {T{2}, T{4}, T{4}, T{6}};
    U ic[20];
    int ir[] = {2, 4, 4};
    const int sr = sizeof(ir)/sizeof(ir[0]);

    U* res = ranges::set_intersection(ranges::views::all(ia), ranges::views::all(ib), ic,
                                      std::less<int>(), &S::i, &T::j);
    EXPECT_EQ(res - ic, sr);
    EXPECT_FALSE(ranges::lexicographical_compare(ic, res, ir, ir+sr, std::less<int>(), &U::k));
}

TEST(SetIntersection, Constexpr) {
    EXPECT_TRUE(test_constexpr());
}
#endif