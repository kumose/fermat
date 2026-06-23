#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <array>
#include <fermat/algorithm/remove_copy.h>
#include <fermat/view/subrange.h>


/// test remove_copy with iterator pairs (raw pointers)
void test_remove_copy_iter() {
    int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    auto res = fermat::ranges::remove_copy(ia, ia + sa, ib, 2);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + (sa - 3));
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 3);
    EXPECT_EQ(ib[3], 4);
    EXPECT_EQ(ib[4], 3);
    EXPECT_EQ(ib[5], 4);
}

/// test remove_copy with range (subrange)
void test_remove_copy_range() {
    int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    int ib[sa] = {0};

    auto rng = fermat::ranges::make_subrange(ia, ia + sa);
    auto res = fermat::ranges::remove_copy(rng, ib, 2);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + (sa - 3));
    EXPECT_EQ(ib[0], 0);
    EXPECT_EQ(ib[1], 1);
    EXPECT_EQ(ib[2], 3);
    EXPECT_EQ(ib[3], 4);
    EXPECT_EQ(ib[4], 3);
    EXPECT_EQ(ib[5], 4);
}

/// projection test
struct S { int i; };

TEST(RemoveCopyTest, IteratorPair) {
    test_remove_copy_iter();
}

TEST(RemoveCopyTest, Range) {
    test_remove_copy_range();
}

TEST(RemoveCopyTest, Projection) {
    S ia[] = {S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    S ib[sa] = {{0}};

    auto res = fermat::ranges::remove_copy(ia, ib, 2, &S::i);
    EXPECT_EQ(res.in, ia + sa);
    EXPECT_EQ(res.out, ib + (sa - 3));
    EXPECT_EQ(ib[0].i, 0);
    EXPECT_EQ(ib[1].i, 1);
    EXPECT_EQ(ib[2].i, 3);
    EXPECT_EQ(ib[3].i, 4);
    EXPECT_EQ(ib[4].i, 3);
    EXPECT_EQ(ib[5].i, 4);
}

/// rvalue range test (compile only, we just check that it compiles and outputs are correct)
TEST(RemoveCopyTest, RvalueRange) {
    S ia[] = {S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
    S ib[sa] = {{0}};

    auto res = fermat::ranges::remove_copy(std::move(ia), ib, 2, &S::i);
    // res.in is dangling; we ignore it.
    EXPECT_EQ(res.out, ib + (sa - 3));
    EXPECT_EQ(ib[0].i, 0);
    EXPECT_EQ(ib[1].i, 1);
    EXPECT_EQ(ib[2].i, 3);
    EXPECT_EQ(ib[3].i, 4);
    EXPECT_EQ(ib[4].i, 3);
    EXPECT_EQ(ib[5].i, 4);

    std::fill(std::begin(ib), std::end(ib), S{0});
    std::vector<S> vec(std::begin(ia), std::end(ia));
    auto res2 = fermat::ranges::remove_copy(std::move(vec), ib, 2, &S::i);
    EXPECT_EQ(res2.out, ib + (sa - 3));
    EXPECT_EQ(ib[0].i, 0);
    EXPECT_EQ(ib[1].i, 1);
    EXPECT_EQ(ib[2].i, 3);
    EXPECT_EQ(ib[3].i, 4);
    EXPECT_EQ(ib[4].i, 3);
    EXPECT_EQ(ib[5].i, 4);
}

/// constexpr test (compile‑time)
TEST(RemoveCopyTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
        constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);
        int ib[6] = {0};
        auto r = fermat::ranges::remove_copy(ia, ib, 2);
        bool ok = (r.in == ia + sa) &&
                  (r.out == ib + (sa - 3)) &&
                  (ib[0] == 0) && (ib[1] == 1) &&
                  (ib[2] == 3) && (ib[3] == 4) &&
                  (ib[4] == 3) && (ib[5] == 4);
        return ok;
    };
    static_assert(test(), "");
}
