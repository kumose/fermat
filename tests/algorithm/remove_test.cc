#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <array>
#include <fermat/algorithm/remove_test.h>
#include <fermat/view/subrange.h>

/// test remove with iterator pairs (raw pointers)
void test_remove_iter() {
    int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    int* r = ranges::remove(ia, ia + sa, 2);
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 3);
    EXPECT_EQ(ia[3], 4);
    EXPECT_EQ(ia[4], 3);
    EXPECT_EQ(ia[5], 4);
}

/// test remove with range (using subrange)
void test_remove_range() {
    int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto r = ranges::remove(ranges::make_subrange(ia, ia + sa), 2);
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 3);
    EXPECT_EQ(ia[3], 4);
    EXPECT_EQ(ia[4], 3);
    EXPECT_EQ(ia[5], 4);
}

/// test remove with move‑only types (unique_ptr)
void test_remove_move_only() {
    constexpr std::size_t sa = 9;
    std::unique_ptr<int> ia[sa];
    ia[0] = std::make_unique<int>(0);
    ia[1] = std::make_unique<int>(1);
    ia[3] = std::make_unique<int>(3);
    ia[4] = std::make_unique<int>(4);
    ia[6] = std::make_unique<int>(3);
    ia[7] = std::make_unique<int>(4);

    auto r = ranges::remove(ia, ia + sa, std::unique_ptr<int>());
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(*ia[0], 0);
    EXPECT_EQ(*ia[1], 1);
    EXPECT_EQ(*ia[2], 3);
    EXPECT_EQ(*ia[3], 4);
    EXPECT_EQ(*ia[4], 3);
    EXPECT_EQ(*ia[5], 4);
}

/// projection test
struct S { int i; };

TEST(RemoveTest, IteratorPair) {
    test_remove_iter();
}

TEST(RemoveTest, Range) {
    test_remove_range();
}

TEST(RemoveTest, MoveOnly) {
    test_remove_move_only();
}

TEST(RemoveTest, Projection) {
    S arr[] = {S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    constexpr std::size_t sa = sizeof(arr) / sizeof(arr[0]);

    S* r = ranges::remove(arr, 2, &S::i);
    EXPECT_EQ(r, arr + sa - 3);
    EXPECT_EQ(arr[0].i, 0);
    EXPECT_EQ(arr[1].i, 1);
    EXPECT_EQ(arr[2].i, 3);
    EXPECT_EQ(arr[3].i, 4);
    EXPECT_EQ(arr[4].i, 3);
    EXPECT_EQ(arr[5].i, 4);
}

TEST(RemoveTest, RvalueRange) {
    S arr[] = {S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    auto r = ranges::remove(std::move(arr), 2, &S::i);
    // r is a dangling iterator; we only verify that the array content is correct.
    (void)r;
    EXPECT_EQ(arr[0].i, 0);
    EXPECT_EQ(arr[1].i, 1);
    EXPECT_EQ(arr[2].i, 3);
    EXPECT_EQ(arr[3].i, 4);
    EXPECT_EQ(arr[4].i, 3);
    EXPECT_EQ(arr[5].i, 4);

    std::vector<S> vec{S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    auto r2 = ranges::remove(std::move(vec), 2, &S::i);
    (void)r2;
    EXPECT_EQ(vec[0].i, 0);
    EXPECT_EQ(vec[1].i, 1);
    EXPECT_EQ(vec[2].i, 3);
    EXPECT_EQ(vec[3].i, 4);
    EXPECT_EQ(vec[4].i, 3);
    EXPECT_EQ(vec[5].i, 4);
}

/// constexpr test (compile‑time)
TEST(RemoveTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int arr[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
        constexpr std::size_t sa = sizeof(arr) / sizeof(arr[0]);
        int* r = ranges::remove(arr, 2);
        bool ok = (r == arr + sa - 3) &&
                  (arr[0] == 0) && (arr[1] == 1) &&
                  (arr[2] == 3) && (arr[3] == 4) &&
                  (arr[4] == 3) && (arr[5] == 4);
        return ok;
    };
    static_assert(test(), "");
}