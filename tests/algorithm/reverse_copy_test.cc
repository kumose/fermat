#include <gtest/gtest.h>
#include <cstring>
#include <fermat/algorithm/reverse_copy.h>
#include <fermat/view/subrange.h>

/// test reverse_copy with iterator pairs (raw pointers)
void test_reverse_copy_iter() {
    // empty range
    const int ia[] = {0};
    int ja[1] = {-1};
    auto p0 = ranges::reverse_copy(ia, ia, ja);
    EXPECT_EQ(p0.in, ia);
    EXPECT_EQ(p0.out, ja);
    EXPECT_EQ(ja[0], -1);

    // single element
    auto p1 = ranges::reverse_copy(ia, ia + 1, ja);
    EXPECT_EQ(p1.in, ia + 1);
    EXPECT_EQ(p1.out, ja + 1);
    EXPECT_EQ(ja[0], 0);

    // two elements
    const int ib[] = {0, 1};
    int jb[2] = {-1, -1};
    auto p2 = ranges::reverse_copy(ib, ib + 2, jb);
    EXPECT_EQ(p2.in, ib + 2);
    EXPECT_EQ(p2.out, jb + 2);
    EXPECT_EQ(jb[0], 1);
    EXPECT_EQ(jb[1], 0);

    // three elements
    const int ic[] = {0, 1, 2};
    int jc[3] = {-1, -1, -1};
    auto p3 = ranges::reverse_copy(ic, ic + 3, jc);
    EXPECT_EQ(p3.in, ic + 3);
    EXPECT_EQ(p3.out, jc + 3);
    EXPECT_EQ(jc[0], 2);
    EXPECT_EQ(jc[1], 1);
    EXPECT_EQ(jc[2], 0);

    // four elements
    const int id[] = {0, 1, 2, 3};
    int jd[4] = {-1, -1, -1, -1};
    auto p4 = ranges::reverse_copy(id, id + 4, jd);
    EXPECT_EQ(p4.in, id + 4);
    EXPECT_EQ(p4.out, jd + 4);
    EXPECT_EQ(jd[0], 3);
    EXPECT_EQ(jd[1], 2);
    EXPECT_EQ(jd[2], 1);
    EXPECT_EQ(jd[3], 0);
}

/// test reverse_copy with range (subrange)
void test_reverse_copy_range() {
    const int ia[] = {0};
    int ja[1] = {-1};
    auto r0 = ranges::reverse_copy(ranges::make_subrange(ia, ia), ja);
    EXPECT_EQ(r0.in, ia);
    EXPECT_EQ(r0.out, ja);
    EXPECT_EQ(ja[0], -1);

    auto r1 = ranges::reverse_copy(ranges::make_subrange(ia, ia + 1), ja);
    EXPECT_EQ(r1.in, ia + 1);
    EXPECT_EQ(r1.out, ja + 1);
    EXPECT_EQ(ja[0], 0);

    const int ib[] = {0, 1};
    int jb[2] = {-1, -1};
    auto r2 = ranges::reverse_copy(ranges::make_subrange(ib, ib + 2), jb);
    EXPECT_EQ(r2.in, ib + 2);
    EXPECT_EQ(r2.out, jb + 2);
    EXPECT_EQ(jb[0], 1);
    EXPECT_EQ(jb[1], 0);

    const int ic[] = {0, 1, 2};
    int jc[3] = {-1, -1, -1};
    auto r3 = ranges::reverse_copy(ranges::make_subrange(ic, ic + 3), jc);
    EXPECT_EQ(r3.in, ic + 3);
    EXPECT_EQ(r3.out, jc + 3);
    EXPECT_EQ(jc[0], 2);
    EXPECT_EQ(jc[1], 1);
    EXPECT_EQ(jc[2], 0);

    const int id[] = {0, 1, 2, 3};
    int jd[4] = {-1, -1, -1, -1};
    auto r4 = ranges::reverse_copy(ranges::make_subrange(id, id + 4), jd);
    EXPECT_EQ(r4.in, id + 4);
    EXPECT_EQ(r4.out, jd + 4);
    EXPECT_EQ(jd[0], 3);
    EXPECT_EQ(jd[1], 2);
    EXPECT_EQ(jd[2], 1);
    EXPECT_EQ(jd[3], 0);
}

TEST(ReverseCopyTest, IteratorPair) {
    test_reverse_copy_iter();
}

TEST(ReverseCopyTest, Range) {
    test_reverse_copy_range();
}

/// constexpr test (compile‑time)
TEST(ReverseCopyTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4};
        int ib[5] = {0};
        constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
        auto r = ranges::reverse_copy(ia, ib);
        bool ok = (r.in == ia + sa) &&
                  (r.out == ib + sa) &&
                  (ib[0] == 4) && (ib[1] == 3) &&
                  (ib[2] == 2) && (ib[3] == 1) && (ib[4] == 0);
        return ok;
    };
    static_assert(test(), "");
}
