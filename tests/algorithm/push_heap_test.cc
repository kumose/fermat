#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <fermat/algorithm/heap_algorithm.h>
#include <fermat/view/subrange.h>

std::mt19937 gen;

/// test push_heap with default comparator
void test_push_heap_basic(int N) {
    std::vector<int> data(N);
    for (int i = 0; i < N; ++i) data[i] = i;
    std::shuffle(data.begin(), data.end(), gen);

    for (int i = 0; i <= N; ++i) {
        auto res = ranges::push_heap(data.begin(), data.begin() + i);
        EXPECT_EQ(res, data.begin() + i);
        EXPECT_TRUE(std::is_heap(data.begin(), data.begin() + i));
    }
}

/// test push_heap with comparator (std::greater)
void test_push_heap_comp(int N) {
    std::vector<int> data(N);
    for (int i = 0; i < N; ++i) data[i] = i;
    std::shuffle(data.begin(), data.end(), gen);

    for (int i = 0; i <= N; ++i) {
        auto res = ranges::push_heap(data.begin(), data.begin() + i, std::greater<int>());
        EXPECT_EQ(res, data.begin() + i);
        EXPECT_TRUE(std::is_heap(data.begin(), data.begin() + i, std::greater<int>()));
    }
}

/// test push_heap with projection
void test_push_heap_proj(int N) {
    struct S { int i; };
    std::vector<S> data(N);
    for (int i = 0; i < N; ++i) data[i].i = i;
    std::shuffle(data.begin(), data.end(), gen);

    std::vector<int> tmp(N);
    for (int i = 0; i <= N; ++i) {
        auto res = ranges::push_heap(data.begin(), data.begin() + i,
                                     std::greater<int>(), &S::i);
        EXPECT_EQ(res, data.begin() + i);
        std::transform(data.begin(), data.begin() + i, tmp.begin(),
                       [](const S& s) { return s.i; });
        EXPECT_TRUE(std::is_heap(tmp.begin(), tmp.begin() + i, std::greater<int>()));
    }
}

/// test push_heap with move‑only types
void test_push_heap_move_only(int N) {
    // use lambda (no member template in local class)
    auto indirect_less = [](const std::unique_ptr<int>& x, const std::unique_ptr<int>& y) {
        return *x < *y;
    };
    std::vector<std::unique_ptr<int>> data(N);
    for (int i = 0; i < N; ++i) data[i] = std::make_unique<int>(i);
    std::shuffle(data.begin(), data.end(), gen);

    for (int i = 0; i <= N; ++i) {
        auto res = ranges::push_heap(data.begin(), data.begin() + i, indirect_less);
        EXPECT_EQ(res, data.begin() + i);
        EXPECT_TRUE(std::is_heap(data.begin(), data.begin() + i, indirect_less));
    }
}

TEST(PushHeapTest, Basic) {
    test_push_heap_basic(1000);
}

TEST(PushHeapTest, WithComparator) {
    test_push_heap_comp(1000);
}

TEST(PushHeapTest, WithProjection) {
    test_push_heap_proj(1000);
}

TEST(PushHeapTest, MoveOnly) {
    test_push_heap_move_only(1000);
}

TEST(PushHeapTest, RangeVersion) {
    struct S { int i; };
    const int N = 1000;
    std::vector<S> data(N);
    for (int i = 0; i < N; ++i) data[i].i = i;
    std::shuffle(data.begin(), data.end(), gen);

    std::vector<int> tmp(N);
    for (int i = 0; i <= N; ++i) {
        auto rng = ranges::make_subrange(data.begin(), data.begin() + i);
        auto res = ranges::push_heap(rng, std::greater<int>(), &S::i);
        EXPECT_EQ(res, data.begin() + i);
        std::transform(data.begin(), data.begin() + i, tmp.begin(),
                       [](const S& s) { return s.i; });
        EXPECT_TRUE(std::is_heap(tmp.begin(), tmp.begin() + i, std::greater<int>()));
    }
}

/// runtime test for constexpr-like behavior (std::is_heap not constexpr)
TEST(PushHeapTest, ConstexprRuntime) {
    constexpr int N = 100;
    std::array<int, N> arr{};
    for (int i = 0; i < N; ++i) arr[i] = i;
    for (int i = 0; i <= N; ++i) {
        auto res = ranges::push_heap(arr.begin(), arr.begin() + i, std::greater<int>());
        EXPECT_EQ(res, arr.begin() + i);
        EXPECT_TRUE(std::is_heap(arr.begin(), arr.begin() + i, std::greater<int>()));
    }
}
