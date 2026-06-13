#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <initializer_list>
#include <fermat/algorithm/partition_copy.h>
#include <fermat/view/subrange.h>

/// predicate for testing
struct is_odd {
    constexpr bool operator()(const int& i) const { return i & 1; }
};

/// test partition_copy with iterator pairs (raw pointers)
void test_iter() {
    const int ia[] = {1, 2, 3, 4, 6, 8, 5, 7};
    int r1[10] = {0};
    int r2[10] = {0};

    auto p = ranges::partition_copy(std::begin(ia), std::end(ia), r1, r2, is_odd{});
    EXPECT_EQ(p.in, std::end(ia));
    EXPECT_EQ(p.out1, r1 + 4);
    EXPECT_EQ(r1[0], 1);
    EXPECT_EQ(r1[1], 3);
    EXPECT_EQ(r1[2], 5);
    EXPECT_EQ(r1[3], 7);
    EXPECT_EQ(p.out2, r2 + 4);
    EXPECT_EQ(r2[0], 2);
    EXPECT_EQ(r2[1], 4);
    EXPECT_EQ(r2[2], 6);
    EXPECT_EQ(r2[3], 8);
}

/// test partition_copy with range (using subrange)
void test_range() {
    const int ia[] = {1, 2, 3, 4, 6, 8, 5, 7};
    int r1[10] = {0};
    int r2[10] = {0};

    auto rng = ranges::make_subrange(std::begin(ia), std::end(ia));
    auto p = ranges::partition_copy(rng, r1, r2, is_odd{});
    EXPECT_EQ(p.in, std::end(ia));
    EXPECT_EQ(p.out1, r1 + 4);
    EXPECT_EQ(r1[0], 1);
    EXPECT_EQ(r1[1], 3);
    EXPECT_EQ(r1[2], 5);
    EXPECT_EQ(r1[3], 7);
    EXPECT_EQ(p.out2, r2 + 4);
    EXPECT_EQ(r2[0], 2);
    EXPECT_EQ(r2[1], 4);
    EXPECT_EQ(r2[2], 6);
    EXPECT_EQ(r2[3], 8);
}

/// projection test
struct S { int i; };

TEST(PartitionCopyTest, IteratorPair) {
    test_iter();
}

TEST(PartitionCopyTest, Range) {
    test_range();
}

TEST(PartitionCopyTest, Projection) {
    const S ia[] = {S{1}, S{2}, S{3}, S{4}, S{6}, S{8}, S{5}, S{7}};
    S r1[10] = {{0}};
    S r2[10] = {{0}};

    auto p = ranges::partition_copy(ia, r1, r2, is_odd{}, &S::i);
    EXPECT_EQ(p.in, std::end(ia));
    EXPECT_EQ(p.out1, r1 + 4);
    EXPECT_EQ(r1[0].i, 1);
    EXPECT_EQ(r1[1].i, 3);
    EXPECT_EQ(r1[2].i, 5);
    EXPECT_EQ(r1[3].i, 7);
    EXPECT_EQ(p.out2, r2 + 4);
    EXPECT_EQ(r2[0].i, 2);
    EXPECT_EQ(r2[1].i, 4);
    EXPECT_EQ(r2[2].i, 6);
    EXPECT_EQ(r2[3].i, 8);
}

/// rvalue range test (compile only, we just check dangling is not required)
TEST(PartitionCopyTest, RvalueRange) {
    const S ia[] = {S{1}, S{2}, S{3}, S{4}, S{6}, S{8}, S{5}, S{7}};
    S r1[10] = {{0}};
    S r2[10] = {{0}};

    auto p = ranges::partition_copy(std::move(ia), r1, r2, is_odd{}, &S::i);
    // p.in is dangling; we only verify that it compiles and outputs are correct.
    EXPECT_EQ(p.out1, r1 + 4);
    EXPECT_EQ(r1[0].i, 1);
    EXPECT_EQ(r1[1].i, 3);
    EXPECT_EQ(r1[2].i, 5);
    EXPECT_EQ(r1[3].i, 7);
    EXPECT_EQ(p.out2, r2 + 4);
    EXPECT_EQ(r2[0].i, 2);
    EXPECT_EQ(r2[1].i, 4);
    EXPECT_EQ(r2[2].i, 6);
    EXPECT_EQ(r2[3].i, 8);

    std::fill(r1, r1 + 10, S{0});
    std::fill(r2, r2 + 10, S{0});
    std::vector<S> vec(std::begin(ia), std::end(ia));
    auto q = ranges::partition_copy(std::move(vec), r1, r2, is_odd{}, &S::i);
    EXPECT_EQ(q.out1, r1 + 4);
    EXPECT_EQ(r1[0].i, 1);
    EXPECT_EQ(r1[1].i, 3);
    EXPECT_EQ(r1[2].i, 5);
    EXPECT_EQ(r1[3].i, 7);
    EXPECT_EQ(q.out2, r2 + 4);
    EXPECT_EQ(r2[0].i, 2);
    EXPECT_EQ(r2[1].i, 4);
    EXPECT_EQ(r2[2].i, 6);
    EXPECT_EQ(r2[3].i, 8);
}

/// constexpr test (static_assert at compile time)
TEST(PartitionCopyTest, Constexpr) {
    constexpr auto test = []() constexpr {
        const int ia[] = {1, 2, 3, 4, 6, 8, 5, 7};
        int r1[10] = {0};
        int r2[10] = {0};
        const auto p = ranges::partition_copy(ia, r1, r2, is_odd{});
        bool ok = (p.in == std::end(ia)) &&
                  (p.out1 == r1 + 4) &&
                  (r1[0] == 1) && (r1[1] == 3) && (r1[2] == 5) && (r1[3] == 7) &&
                  (p.out2 == r2 + 4) &&
                  (r2[0] == 2) && (r2[1] == 4) && (r2[2] == 6) && (r2[3] == 8);
        return ok;
    };
    static_assert(test(), "");
}
