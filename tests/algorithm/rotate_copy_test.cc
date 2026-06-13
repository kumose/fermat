#include <gtest/gtest.h>
#include <vector>
#include <fermat/algorithm/rotate_copy.h>
#include <fermat/view/subrange.h>

/// test rotate_copy with iterator pairs (raw pointers)
void test_rotate_copy_iter() {
    int ia[] = {0, 1, 2, 3};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    // rotate by 0 on empty range
    auto r = ranges::rotate_copy(ia, ia, ia, ib);
    EXPECT_EQ(r.in, ia);
    EXPECT_EQ(r.out, ib);

    // rotate by 0 on [0,1)
    r = ranges::rotate_copy(ia, ia, ia + 1, ib);
    EXPECT_EQ(r.in, ia + 1);
    EXPECT_EQ(r.out, ib + 1);
    EXPECT_EQ(ib[0], 0);

    // rotate by 1 on [0,1) (no effect)
    r = ranges::rotate_copy(ia, ia + 1, ia + 1, ib);
    EXPECT_EQ(r.in, ia + 1);
    EXPECT_EQ(r.out, ib + 1);
    EXPECT_EQ(ib[0], 0);

    // rotate by 0 on [0,2)
    r = ranges::rotate_copy(ia, ia, ia + 2, ib);
    EXPECT_EQ(r.in, ia + 2);
    EXPECT_EQ(r.out, ib + 2);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);

    // rotate by 1 on [0,2)
    r = ranges::rotate_copy(ia, ia + 1, ia + 2, ib);
    EXPECT_EQ(r.in, ia + 2);
    EXPECT_EQ(r.out, ib + 2);
    EXPECT_EQ(ib[0], 1);
    EXPECT_EQ(ib[1], 0);

    // rotate by 2 on [0,2) (full)
    r = ranges::rotate_copy(ia, ia + 2, ia + 2, ib);
    EXPECT_EQ(r.in, ia + 2);
    EXPECT_EQ(r.out, ib + 2);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);

    // rotate by 0 on [0,3)
    r = ranges::rotate_copy(ia, ia, ia + 3, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);

    // rotate by 1 on [0,3)
    r = ranges::rotate_copy(ia, ia + 1, ia + 3, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 1);
    EXPECT_EQ(ib[1], 2);
    EXPECT_EQ(ib[2], 0);

    // rotate by 2 on [0,3)
    r = ranges::rotate_copy(ia, ia + 2, ia + 3, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 2);
    EXPECT_EQ(ib[1], 0);
    EXPECT_EQ(ib[2], 1);

    // rotate by 3 on [0,3)
    r = ranges::rotate_copy(ia, ia + 3, ia + 3, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);

    // rotate by 0 on [0,4)
    r = ranges::rotate_copy(ia, ia, ia + 4, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);
    EXPECT_EQ(ib[3], 3);

    // rotate by 1 on [0,4)
    r = ranges::rotate_copy(ia, ia + 1, ia + 4, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 1);
    EXPECT_EQ(ib[1], 2);
    EXPECT_EQ(ib[2], 3);
    EXPECT_EQ(ib[3], 0);

    // rotate by 2 on [0,4)
    r = ranges::rotate_copy(ia, ia + 2, ia + 4, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 2);
    EXPECT_EQ(ib[1], 3);
    EXPECT_EQ(ib[2], 0);
    EXPECT_EQ(ib[3], 1);

    // rotate by 3 on [0,4)
    r = ranges::rotate_copy(ia, ia + 3, ia + 4, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 3);
    EXPECT_EQ(ib[1], 0);
    EXPECT_EQ(ib[2], 1);
    EXPECT_EQ(ib[3], 2);

    // rotate by 4 on [0,4)
    r = ranges::rotate_copy(ia, ia + 4, ia + 4, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);
    EXPECT_EQ(ib[3], 3);
}

/// test rotate_copy with range (subrange)
void test_rotate_copy_range() {
    int ia[] = {0, 1, 2, 3};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    // empty range
    auto rng = ranges::make_subrange(ia, ia);
    auto r = ranges::rotate_copy(rng, ia, ib);
    EXPECT_EQ(r.in, ia);
    EXPECT_EQ(r.out, ib);

    // range [0,1), rotate by 0
    rng = ranges::make_subrange(ia, ia + 1);
    r = ranges::rotate_copy(rng, ia, ib);
    EXPECT_EQ(r.in, ia + 1);
    EXPECT_EQ(r.out, ib + 1);
    EXPECT_EQ(ib[0], 0);

    // range [0,1), rotate by 1
    r = ranges::rotate_copy(rng, ia + 1, ib);
    EXPECT_EQ(r.in, ia + 1);
    EXPECT_EQ(r.out, ib + 1);
    EXPECT_EQ(ib[0], 0);

    // range [0,2), rotate by 0
    rng = ranges::make_subrange(ia, ia + 2);
    r = ranges::rotate_copy(rng, ia, ib);
    EXPECT_EQ(r.in, ia + 2);
    EXPECT_EQ(r.out, ib + 2);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);

    // rotate by 1
    r = ranges::rotate_copy(rng, ia + 1, ib);
    EXPECT_EQ(r.in, ia + 2);
    EXPECT_EQ(r.out, ib + 2);
    EXPECT_EQ(ib[0], 1);
    EXPECT_EQ(ib[1], 0);

    // rotate by 2
    r = ranges::rotate_copy(rng, ia + 2, ib);
    EXPECT_EQ(r.in, ia + 2);
    EXPECT_EQ(r.out, ib + 2);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);

    // range [0,3), rotate by 0
    rng = ranges::make_subrange(ia, ia + 3);
    r = ranges::rotate_copy(rng, ia, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);

    // rotate by 1
    r = ranges::rotate_copy(rng, ia + 1, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 1);
    EXPECT_EQ(ib[1], 2);
    EXPECT_EQ(ib[2], 0);

    // rotate by 2
    r = ranges::rotate_copy(rng, ia + 2, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 2);
    EXPECT_EQ(ib[1], 0);
    EXPECT_EQ(ib[2], 1);

    // rotate by 3
    r = ranges::rotate_copy(rng, ia + 3, ib);
    EXPECT_EQ(r.in, ia + 3);
    EXPECT_EQ(r.out, ib + 3);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);

    // range [0,4), rotate by 0
    rng = ranges::make_subrange(ia, ia + 4);
    r = ranges::rotate_copy(rng, ia, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);
    EXPECT_EQ(ib[3], 3);

    // rotate by 1
    r = ranges::rotate_copy(rng, ia + 1, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 1);
    EXPECT_EQ(ib[1], 2);
    EXPECT_EQ(ib[2], 3);
    EXPECT_EQ(ib[3], 0);

    // rotate by 2
    r = ranges::rotate_copy(rng, ia + 2, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 2);
    EXPECT_EQ(ib[1], 3);
    EXPECT_EQ(ib[2], 0);
    EXPECT_EQ(ib[3], 1);

    // rotate by 3
    r = ranges::rotate_copy(rng, ia + 3, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 3);
    EXPECT_EQ(ib[1], 0);
    EXPECT_EQ(ib[2], 1);
    EXPECT_EQ(ib[3], 2);

    // rotate by 4
    r = ranges::rotate_copy(rng, ia + 4, ib);
    EXPECT_EQ(r.in, ia + 4);
    EXPECT_EQ(r.out, ib + 4);
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 2);
    EXPECT_EQ(ib[3], 3);
}

TEST(RotateCopyTest, IteratorPair) {
    test_rotate_copy_iter();
}

TEST(RotateCopyTest, Range) {
    test_rotate_copy_range();
}

/// test rvalue ranges (only verify output array content)
TEST(RotateCopyTest, RvalueRange) {
    int rgi[] = {0, 1, 2, 3, 4, 5};
    int rgo[6] = {0};
    auto r = ranges::rotate_copy(std::move(rgi), rgi + 2, rgo);
    // r.in is dangling; we ignore it
    EXPECT_EQ(r.out, rgo + 6);
    EXPECT_EQ(rgo[0], 2);
    EXPECT_EQ(rgo[1], 3);
    EXPECT_EQ(rgo[2], 4);
    EXPECT_EQ(rgo[3], 5);
    EXPECT_EQ(rgo[4], 0);
    EXPECT_EQ(rgo[5], 1);

    std::vector<int> vec{0, 1, 2, 3, 4, 5};
    int rgo2[6] = {0};
    auto r2 = ranges::rotate_copy(std::move(vec), vec.begin() + 2, rgo2);
    EXPECT_EQ(r2.out, rgo2 + 6);
    EXPECT_EQ(rgo2[0], 2);
    EXPECT_EQ(rgo2[1], 3);
    EXPECT_EQ(rgo2[2], 4);
    EXPECT_EQ(rgo2[3], 5);
    EXPECT_EQ(rgo2[4], 0);
    EXPECT_EQ(rgo2[5], 1);
}

/// constexpr test (compile‑time)
TEST(RotateCopyTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int rgi[] = {0, 1, 2, 3, 4, 5};
        int rgo[6] = {0};
        auto r = ranges::rotate_copy(rgi, rgi + 2, rgo);
        bool ok = (r.in == rgi + 6) &&
                  (r.out == rgo + 6) &&
                  (rgo[0] == 2) && (rgo[1] == 3) &&
                  (rgo[2] == 4) && (rgo[3] == 5) &&
                  (rgo[4] == 0) && (rgo[5] == 1);
        return ok;
    };
    static_assert(test(), "");
}
