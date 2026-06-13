#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <memory>
#include <vector>
#include <array>
#include <fermat/algorithm/heap_algorithm.h>
#include <fermat/view/subrange.h>

std::mt19937 gen;

// Test for make_heap with no comparator, using iterator pair
void test_make_heap_basic(int N) {
    std::vector<int> v(N);
    for (int i = 0; i < N; ++i) v[i] = i;
    std::shuffle(v.begin(), v.end(), gen);
    auto result = fermat::ranges::make_heap(v.begin(), v.end());
    EXPECT_EQ(result, v.end());
    EXPECT_TRUE(std::is_heap(v.begin(), v.end()));
}

// With sentinel (just pointer as sentinel, same as pair)
void test_make_heap_sentinel(int N) {
    std::vector<int> v(N);
    for (int i = 0; i < N; ++i) v[i] = i;
    std::shuffle(v.begin(), v.end(), gen);
    auto result = fermat::ranges::make_heap(v.data(), v.data() + N);
    EXPECT_EQ(result, v.data() + N);
    EXPECT_TRUE(std::is_heap(v.begin(), v.end()));
}

// With subrange
void test_make_heap_subrange(int N) {
    std::vector<int> v(N);
    for (int i = 0; i < N; ++i) v[i] = i;
    std::shuffle(v.begin(), v.end(), gen);
    auto rng = fermat::ranges::subrange(v.begin(), v.end());
    auto result = fermat::ranges::make_heap(rng);
    EXPECT_EQ(result, v.end());
    EXPECT_TRUE(std::is_heap(v.begin(), v.end()));
}

// With comparator (std::greater)
void test_make_heap_comparator(int N) {
    std::vector<int> v(N);
    for (int i = 0; i < N; ++i) v[i] = i;
    std::shuffle(v.begin(), v.end(), gen);
    auto result = fermat::ranges::make_heap(v.begin(), v.end(), std::greater<int>());
    EXPECT_EQ(result, v.end());
    EXPECT_TRUE(std::is_heap(v.begin(), v.end(), std::greater<int>()));
}

// With comparator and subrange
void test_make_heap_comparator_subrange(int N) {
    std::vector<int> v(N);
    for (int i = 0; i < N; ++i) v[i] = i;
    std::shuffle(v.begin(), v.end(), gen);
    auto rng = fermat::ranges::subrange(v.begin(), v.end());
    auto result = fermat::ranges::make_heap(rng, std::greater<int>());
    EXPECT_EQ(result, v.end());
    EXPECT_TRUE(std::is_heap(v.begin(), v.end(), std::greater<int>()));
}

// With indirect comparison (unique_ptr)
void test_make_heap_indirect(int N) {
    auto indirect_less = [](const std::unique_ptr<int>& x, const std::unique_ptr<int>& y) {
        return *x < *y;
    };
    std::vector<std::unique_ptr<int>> v(N);
    for (int i = 0; i < N; ++i) v[i] = std::make_unique<int>(i);
    std::shuffle(v.begin(), v.end(), gen);
    auto result = fermat::ranges::make_heap(v.begin(), v.end(), indirect_less);
    EXPECT_EQ(result, v.end());
    EXPECT_TRUE(std::is_heap(v.begin(), v.end(), indirect_less));
}

// With projection
void test_make_heap_projection(int N) {
    struct S { int i; };
    std::vector<S> v(N);
    for (int i = 0; i < N; ++i) v[i].i = i;
    std::shuffle(v.begin(), v.end(), gen);
    auto result = fermat::ranges::make_heap(v.begin(), v.end(), std::less<int>(), &S::i);
    EXPECT_EQ(result, v.end());
    std::vector<int> tmp(N);
    std::transform(v.begin(), v.end(), tmp.begin(), [](const S& s) { return s.i; });
    EXPECT_TRUE(std::is_heap(tmp.begin(), tmp.end()));
}

// Parameterized test for different N
void test_all(int N) {
    test_make_heap_basic(N);
    test_make_heap_sentinel(N);
    test_make_heap_subrange(N);
    test_make_heap_comparator(N);
    test_make_heap_comparator_subrange(N);
}

TEST(MakeHeapTest, SmallSizes) {
    test_all(0);
    test_all(1);
    test_all(2);
    test_all(3);
    test_all(10);
}

TEST(MakeHeapTest, Larger) {
    test_all(1000);
}

TEST(MakeHeapTest, IndirectComparison) {
    test_make_heap_indirect(1000);
}

TEST(MakeHeapTest, Projection) {
    test_make_heap_projection(1000);
}

// Runtime constexpr-like test (no static_assert)
TEST(MakeHeapTest, ConstexprRuntime) {
    const int N = 100;
    std::array<int, N> ia;
    for (int i = 0; i < N; ++i) ia[i] = N - 1 - i;
    fermat::ranges::make_heap(ia.begin(), ia.end(), std::less<int>());
    EXPECT_TRUE(std::is_heap(ia.begin(), ia.end()));
}
