#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <fermat/algorithm/remove_if.h>
#include <fermat/view/subrange.h>

/// predicate for testing remove_if
struct equals_two {
    bool operator()(int i) const { return i == 2; }
};

/// test remove_if with iterator pairs (raw pointers)
void test_remove_if_iter() {
    int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto pred = [](int i) { return i == 2; };
    int* r = fermat::ranges::remove_if(ia, ia + sa, pred);
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 3);
    EXPECT_EQ(ia[3], 4);
    EXPECT_EQ(ia[4], 3);
    EXPECT_EQ(ia[5], 4);
}

/// test remove_if with range (subrange)
void test_remove_if_range() {
    int ia[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto pred = [](int i) { return i == 2; };
    auto r = fermat::ranges::remove_if(fermat::ranges::make_subrange(ia, ia + sa), pred);
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 3);
    EXPECT_EQ(ia[3], 4);
    EXPECT_EQ(ia[4], 3);
    EXPECT_EQ(ia[5], 4);
}

/// test remove_if with move‑only types (unique_ptr)
void test_remove_if_move_only() {
    constexpr std::size_t sa = 9;
    std::unique_ptr<int> ia[sa];
    ia[0] = std::make_unique<int>(0);
    ia[1] = std::make_unique<int>(1);
    ia[2] = std::make_unique<int>(2);
    ia[3] = std::make_unique<int>(3);
    ia[4] = std::make_unique<int>(4);
    ia[5] = std::make_unique<int>(2);
    ia[6] = std::make_unique<int>(3);
    ia[7] = std::make_unique<int>(4);
    ia[8] = std::make_unique<int>(2);

    auto pred = [](const std::unique_ptr<int>& p) { return *p == 2; };
    auto r = fermat::ranges::remove_if(ia, ia + sa, pred);
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

TEST(RemoveIfTest, IteratorPair) {
    test_remove_if_iter();
}

TEST(RemoveIfTest, Range) {
    test_remove_if_range();
}

TEST(RemoveIfTest, MoveOnly) {
    test_remove_if_move_only();
}

TEST(RemoveIfTest, Projection) {
    S arr[] = {S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    constexpr std::size_t sa = sizeof(arr) / sizeof(arr[0]);

    auto pred = [](int i) { return i == 2; };
    S* r = fermat::ranges::remove_if(arr, pred, &S::i);
    EXPECT_EQ(r, arr + sa - 3);
    EXPECT_EQ(arr[0].i, 0);
    EXPECT_EQ(arr[1].i, 1);
    EXPECT_EQ(arr[2].i, 3);
    EXPECT_EQ(arr[3].i, 4);
    EXPECT_EQ(arr[4].i, 3);
    EXPECT_EQ(arr[5].i, 4);
}

TEST(RemoveIfTest, RvalueRange) {
    S arr[] = {S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    auto pred = [](int i) { return i == 2; };
    auto r = fermat::ranges::remove_if(std::move(arr), pred, &S::i);
    // r is a dangling iterator; we only verify that the array content is correct.
    (void)r;
    EXPECT_EQ(arr[0].i, 0);
    EXPECT_EQ(arr[1].i, 1);
    EXPECT_EQ(arr[2].i, 3);
    EXPECT_EQ(arr[3].i, 4);
    EXPECT_EQ(arr[4].i, 3);
    EXPECT_EQ(arr[5].i, 4);

    std::vector<S> vec{S{0}, S{1}, S{2}, S{3}, S{4}, S{2}, S{3}, S{4}, S{2}};
    auto r2 = fermat::ranges::remove_if(std::move(vec), pred, &S::i);
    (void)r2;
    EXPECT_EQ(vec[0].i, 0);
    EXPECT_EQ(vec[1].i, 1);
    EXPECT_EQ(vec[2].i, 3);
    EXPECT_EQ(vec[3].i, 4);
    EXPECT_EQ(vec[4].i, 3);
    EXPECT_EQ(vec[5].i, 4);
}

/// constexpr test (compile‑time)
TEST(RemoveIfTest, Constexpr) {
    constexpr auto test = []() constexpr {
        int arr[] = {0, 1, 2, 3, 4, 2, 3, 4, 2};
        constexpr std::size_t sa = sizeof(arr) / sizeof(arr[0]);
        auto pred = [](int i) { return i == 2; };
        int* r = fermat::ranges::remove_if(arr, pred);
        bool ok = (r == arr + sa - 3) &&
                  (arr[0] == 0) && (arr[1] == 1) &&
                  (arr[2] == 3) && (arr[3] == 4) &&
                  (arr[4] == 3) && (arr[5] == 4);
        return ok;
    };
    static_assert(test(), "");
}
