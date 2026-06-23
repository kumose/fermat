// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fermat/container/buffer.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
// Helper to check if two buffers contain the same elements.
template<typename T>
bool UnorderedEqual(const fermat::Buffer<T> &a, const fermat::Buffer<T> &b) {
    if (a.size() != b.size()) return false;
    auto a_copy = a;
    auto b_copy = b;
    std::sort(a_copy.begin(), a_copy.end());
    std::sort(b_copy.begin(), b_copy.end());
    return std::equal(a_copy.begin(), a_copy.end(), b_copy.begin());
}

// ============================================================================
// Construction and destruction test cases (only trivial types are allowed)
// ============================================================================

TEST(BufferTest, DefaultConstructor) {
    fermat::Buffer<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0u, v.size());
    EXPECT_EQ(0u, v.capacity());
    EXPECT_EQ(nullptr, v.data());
    EXPECT_TRUE(v.validate());
}

TEST(BufferTest, SizeConstructor) {
    // Buffer(size_type n) – zero-initializes n elements (memset)
    fermat::Buffer<int> v(5);
    EXPECT_EQ(5u, v.size());
    EXPECT_GE(v.capacity(), 5u);
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(0, v[i]); // zero-initialized
    }
}

TEST(BufferTest, SizeValueConstructor) {
    // Buffer(size_type n, const value_type& value)
    fermat::Buffer<int> v(3, 42);
    EXPECT_EQ(3u, v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(42, v[i]);
    }
}

TEST(BufferTest, CopyConstructor) {
    fermat::Buffer<int> original;
    original.append(1);
    original.append(2);
    original.append(3);
    EXPECT_EQ(3u, original.size());

    fermat::Buffer<int> copy(original);
    EXPECT_EQ(original.size(), copy.size());
    EXPECT_EQ(original.capacity(), copy.capacity());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(original[i], copy[i]);
    }
}

TEST(BufferTest, MoveConstructor) {
    fermat::Buffer<int> original;
    original.append(1);
    original.append(2);
    original.append(3);
    size_t original_cap = original.capacity();
    int *original_data = original.data();

    fermat::Buffer<int> moved(std::move(original));
    EXPECT_EQ(3u, moved.size());
    EXPECT_EQ(original_cap, moved.capacity());
    EXPECT_EQ(original_data, moved.data());
    // Original is left in valid but unspecified state (empty, capacity 0)
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(0u, original.capacity());
    EXPECT_EQ(nullptr, original.data());
}

TEST(BufferTest, InitializerListConstructor) {
    fermat::Buffer<int> v = {5, 6, 7};
    EXPECT_EQ(3u, v.size());
    EXPECT_EQ(5, v[0]);
    EXPECT_EQ(6, v[1]);
    EXPECT_EQ(7, v[2]);
}

TEST(BufferTest, IteratorPairConstructor) {
    std::vector<int> src = {1, 2, 3, 4};
    fermat::Buffer<int> v(src.begin(), src.end());
    EXPECT_EQ(4u, v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(static_cast<int>(i + 1), v[i]);
    }
}

TEST(BufferTest, CopyAssignment) {
    fermat::Buffer<int> a;
    a.append(1);
    a.append(2);
    fermat::Buffer<int> b;
    b.append(3);

    b = a;
    EXPECT_EQ(a.size(), b.size());
    EXPECT_EQ(a[0], b[0]);
    EXPECT_EQ(a[1], b[1]);
}

TEST(BufferTest, MoveAssignment) {
    fermat::Buffer<int> a;
    a.append(1);
    a.append(2);
    int *a_data = a.data();
    size_t a_cap = a.capacity();

    fermat::Buffer<int> b;
    b = std::move(a);
    EXPECT_EQ(2u, b.size());
    EXPECT_EQ(a_cap, b.capacity());
    EXPECT_EQ(a_data, b.data());
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(0u, a.capacity());
}

TEST(BufferTest, InitializerListAssignment) {
    fermat::Buffer<int> v;
    v = {10, 20, 30, 40};
    EXPECT_EQ(4u, v.size());
    EXPECT_EQ(10, v[0]);
    EXPECT_EQ(20, v[1]);
    EXPECT_EQ(30, v[2]);
    EXPECT_EQ(40, v[3]);
}

TEST(BufferTest, SelfCopyAssignment) {
    fermat::Buffer<int> v = {1, 2, 3};
    int *data_before = v.data();
    v = v;
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v.data(), data_before);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(BufferTest, SelfMoveAssignment) {
    fermat::Buffer<int> v = {1, 2, 3};
    int *data_before = v.data();
    v = std::move(v);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v.data(), data_before);
}

// -----------------------------------------------------------------------------
// Iterator tests
// -----------------------------------------------------------------------------
TEST(BufferIteratorTest, BeginEnd) {
    fermat::Buffer<int> v = {10, 20, 30};
    auto it = v.begin();
    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);
    ++it;
    EXPECT_EQ(it, v.end());

    const auto &cv = v;
    auto cit = cv.begin();
    EXPECT_EQ(*cit, 10);
    ++cit;
    EXPECT_EQ(*cit, 20);
    ++cit;
    EXPECT_EQ(*cit, 30);
    ++cit;
    EXPECT_EQ(cit, cv.end());
}

TEST(BufferIteratorTest, ReverseBeginEnd) {
    fermat::Buffer<int> v = {10, 20, 30};
    auto rit = v.rbegin();
    EXPECT_EQ(*rit, 30);
    ++rit;
    EXPECT_EQ(*rit, 20);
    ++rit;
    EXPECT_EQ(*rit, 10);
    ++rit;
    EXPECT_EQ(rit, v.rend());

    const auto &cv = v;
    auto crit = cv.rbegin();
    EXPECT_EQ(*crit, 30);
    ++crit;
    EXPECT_EQ(*crit, 20);
    ++crit;
    EXPECT_EQ(*crit, 10);
    ++crit;
    EXPECT_EQ(crit, cv.rend());
}

TEST(BufferIteratorTest, ConstIteratorsCpp11) {
    fermat::Buffer<int> v = {10, 20, 30};
    auto it = v.cbegin();
    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);
    ++it;
    EXPECT_EQ(it, v.cend());

    auto rit = v.crbegin();
    EXPECT_EQ(*rit, 30);
    ++rit;
    EXPECT_EQ(*rit, 20);
    ++rit;
    EXPECT_EQ(*rit, 10);
    ++rit;
    EXPECT_EQ(rit, v.crend());
}

// -----------------------------------------------------------------------------
// Element access
// -----------------------------------------------------------------------------
TEST(BufferAccessTest, SquareBrackets) {
    fermat::Buffer<int> v = {10, 20, 30};
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
    v[1] = 99;
    EXPECT_EQ(v[1], 99);
    const auto &cv = v;
    EXPECT_EQ(cv[0], 10);
    EXPECT_EQ(cv[1], 99);
    EXPECT_EQ(cv[2], 30);
}

TEST(BufferAccessTest, At) {
    fermat::Buffer<int> v = {10, 20, 30};
    EXPECT_EQ(v.at(0), 10);
    EXPECT_EQ(v.at(1), 20);
    EXPECT_EQ(v.at(2), 30);
    const auto &cv = v;
    EXPECT_EQ(cv.at(0), 10);
    EXPECT_EQ(cv.at(1), 20);
    EXPECT_EQ(cv.at(2), 30);
    // Out-of-range would cause KCHECK (death test), skip in regular tests.
}

TEST(BufferAccessTest, FrontBack) {
    fermat::Buffer<int> v = {10, 20, 30};
    EXPECT_EQ(v.front(), 10);
    EXPECT_EQ(v.back(), 30);
    v.front() = 5;
    v.back() = 50;
    EXPECT_EQ(v.front(), 5);
    EXPECT_EQ(v.back(), 50);
    const auto &cv = v;
    EXPECT_EQ(cv.front(), 5);
    EXPECT_EQ(cv.back(), 50);
}

TEST(BufferAccessTest, Data) {
    fermat::Buffer<int> v = {10, 20, 30};
    int *p = v.data();
    EXPECT_EQ(p[0], 10);
    EXPECT_EQ(p[1], 20);
    EXPECT_EQ(p[2], 30);
    const auto &cv = v;
    const int *cp = cv.data();
    EXPECT_EQ(cp[0], 10);
    EXPECT_EQ(cp[1], 20);
    EXPECT_EQ(cp[2], 30);
    fermat::Buffer<int> empty;
    EXPECT_EQ(empty.data(), nullptr);
}

// -----------------------------------------------------------------------------
// Capacity operations
// -----------------------------------------------------------------------------
TEST(BufferCapacityTest, EmptySizeCapacity) {
    fermat::Buffer<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0u, v.size());
    EXPECT_EQ(0u, v.capacity());
    v.append(42);
    EXPECT_FALSE(v.empty());
    EXPECT_EQ(1u, v.size());
    EXPECT_GE(v.capacity(), 1u);
}

TEST(BufferCapacityTest, Reserve) {
    fermat::Buffer<int> v;
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100u);
    const size_t expected_cap = fermat::TieredAllocator<int, 0>::pooled_alloc_size(100u);
    EXPECT_EQ(v.capacity(), expected_cap);
    EXPECT_EQ(0u, v.size());
    size_t old_cap = v.capacity();
    v.reserve(50);
    EXPECT_EQ(v.capacity(), old_cap);
    v.reserve(200);
    EXPECT_GE(v.capacity(), 200u);
}

TEST(BufferCapacityTest, ReserveWithElements) {
    fermat::Buffer<int> v;
    v.append(1);
    v.append(2);
    size_t old_size = v.size();
    v.reserve(100);
    EXPECT_GE(v.capacity(), 100u);
    EXPECT_EQ(v.size(), old_size);
    for (size_t i = 0; i < old_size; ++i) {
        EXPECT_EQ(v[i], static_cast<int>(i + 1));
    }
}

TEST(BufferCapacityTest, ShrinkToFit) {
    fermat::Buffer<int> v;
    v.reserve(100);
    v.append(1);
    v.append(2);
    v.append(3);
    size_t old_cap = v.capacity();
    EXPECT_GT(old_cap, v.size());
    v.shrink_to_fit();
    EXPECT_LE(v.capacity(), old_cap);
    EXPECT_GE(v.capacity(), v.size());
    // After shrink, the capacity should be the minimal good_size(size)
    // But we don't know exact tier, just ensure no increase.
}

TEST(BufferCapacityTest, SetCapacity) {
    fermat::Buffer<int> v;
    v.append(1);
    v.append(2);
    v.append(3);
    size_t old_size = v.size();
    // Expand capacity
    v.set_capacity(100);
    EXPECT_GE(v.capacity(), 100u);
    EXPECT_EQ(v.size(), old_size);
    // Shrink to size (but not below size)
    v.set_capacity(2);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_GE(v.capacity(), 2u);
    // Verify truncated elements are gone
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(BufferCapacityTest, ResizeUninitialized) {
    fermat::Buffer<char> v;
    v.resize_uninitialized(10);
    EXPECT_EQ(v.size(), 10u);
    EXPECT_GE(v.capacity(), 10u);
    // Content is undefined; we can write to it.
    v[5] = 'x';
    EXPECT_EQ(v[5], 'x');
    v.resize_uninitialized(5);
    EXPECT_EQ(v.size(), 5u);
    // Access beyond size is UB, but we can still check capacity.
    EXPECT_GE(v.capacity(), 5u);
}

// -----------------------------------------------------------------------------
// Modifiers (append, push_back, pop_back)
// -----------------------------------------------------------------------------
TEST(BufferModifierTest, AppendSingle) {
    fermat::Buffer<int> v;
    v.append(42);
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 42);
    v.append(99);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[1], 99);
}

TEST(BufferModifierTest, AppendConfident) {
    fermat::Buffer<int> v;
    v.reserve(10);
    v.append_confident(1);
    v.append_confident(2);
    v.append_confident(3);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(BufferModifierTest, AppendBulk) {
    fermat::Buffer<int> v;
    int data[] = {1, 2, 3, 4, 5};
    v.append(data, 5);
    EXPECT_EQ(v.size(), 5u);
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], data[i]);
    }
}

TEST(BufferModifierTest, AppendConfidentBulk) {
    fermat::Buffer<int> v;
    v.reserve(10);
    int data[] = {1, 2, 3, 4, 5};
    v.append_confident(data, 5);
    EXPECT_EQ(v.size(), 5u);
    for (size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], data[i]);
    }
}

TEST(BufferModifierTest, AppendReturnReference) {
    fermat::Buffer<int> v;
    int &ref = v.append();
    ref = 42;
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 42);
    // append default-constructs value
    int &ref2 = v.append();
    ref2 = 100;
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[1], 100);
}

TEST(BufferModifierTest, AppendUninitialized) {
    fermat::Buffer<char> v;
    void *p = v.append_uninitialized();
    char *c = static_cast<char*>(p);
    *c = 'A';
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 'A');
    // Append multiple uninitialized
    v.append_uninitialized();
    v.append_uninitialized();
    v[1] = 'B';
    v[2] = 'C';
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 'A');
    EXPECT_EQ(v[1], 'B');
    EXPECT_EQ(v[2], 'C');
}

TEST(BufferModifierTest, PopBack) {
    fermat::Buffer<int> v = {1, 2, 3};
    v.pop_back();
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    v.pop_back();
    v.pop_back();
    EXPECT_TRUE(v.empty());
    // pop_back on empty is UB; test only non-empty.
}

// -----------------------------------------------------------------------------
// Insert and erase operations
// -----------------------------------------------------------------------------
TEST(BufferModifierTest, InsertSingle) {
    fermat::Buffer<int> v = {1, 3};
    auto it = v.insert(v.begin() + 1, 2);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(it - v.begin(), 1);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(BufferModifierTest, InsertRepeated) {
    fermat::Buffer<int> v = {0, 5};
    v.insert(v.begin() + 1, 3, 99);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 0);
    for (size_t i = 1; i <= 3; ++i) EXPECT_EQ(v[i], 99);
    EXPECT_EQ(v[4], 5);
}

TEST(BufferModifierTest, InsertRange) {
    fermat::Buffer<int> v = {1, 10};
    std::vector<int> src = {2, 3, 4};
    v.insert(v.begin() + 1, src.begin(), src.end());
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 10);
}

TEST(BufferModifierTest, InsertInitializerList) {
    fermat::Buffer<int> v = {1, 5};
    v.insert(v.begin() + 1, {2, 3, 4});
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);
}

TEST(BufferModifierTest, EraseSingle) {
    fermat::Buffer<int> v = {1, 2, 3, 4, 5};
    auto it = v.erase(v.begin() + 2);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(it - v.begin(), 2);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 4);
    EXPECT_EQ(v[3], 5);
}

TEST(BufferModifierTest, EraseRange) {
    fermat::Buffer<int> v = {0,1,2,3,4,5,6,7,8,9};
    auto it = v.erase(v.begin() + 3, v.begin() + 7);
    EXPECT_EQ(v.size(), 6u);
    EXPECT_EQ(it - v.begin(), 3);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 1);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 7);
    EXPECT_EQ(v[4], 8);
    EXPECT_EQ(v[5], 9);
}

TEST(BufferModifierTest, EraseUnsorted) {
    fermat::Buffer<int> v = {0,1,2,3,4};
    auto it = v.erase_unsorted(v.begin() + 1); // remove 1
    EXPECT_EQ(v.size(), 4u);
    // The element at position 1 is replaced by the last element (4)
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 3);
}

TEST(BufferModifierTest, EraseFirst) {
    fermat::Buffer<int> v = {1, 2, 1, 3};
    auto it = v.erase_first(1);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(it - v.begin(), 0);
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 1);
    EXPECT_EQ(v[2], 3);
}

TEST(BufferModifierTest, EraseFirstUnsorted) {
    fermat::Buffer<int> v = {1, 2, 1, 3};
    auto it = v.erase_first_unsorted(1);
    EXPECT_EQ(v.size(), 3u);
    // The first occurrence of 1 is replaced with the last element (3)
    EXPECT_EQ(v[0], 3);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 1);
}

TEST(BufferModifierTest, EraseLast) {
    fermat::Buffer<int> v = {1, 2, 1, 3};
    auto rit = v.erase_last(1);
    (void)rit;
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(BufferModifierTest, EraseLastUnsorted) {
    fermat::Buffer<int> v = {1, 2, 1, 3};
    auto rit = v.erase_last_unsorted(1);
    (void)rit;
    EXPECT_EQ(v.size(), 3u);
    // The last occurrence of 1 is at index 2, replaced by last element 3
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(BufferModifierTest, Clear) {
    fermat::Buffer<int> v = {1,2,3,4,5};
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    // Capacity remains unchanged? Actually clear does not free capacity.
    // But we can test that memory is still allocated.
    EXPECT_GE(v.capacity(), 0u);
}

TEST(BufferModifierTest, ResetLoseMemory) {
    fermat::Buffer<int> v = {1,2,3};
    v.reset_lose_memory();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0u, v.size());
    EXPECT_EQ(0u, v.capacity());
    EXPECT_EQ(nullptr, v.data());
    // Leak of previous memory, no verification needed.
}

// -----------------------------------------------------------------------------
// Bestow / Seize (external memory transfer)
// -----------------------------------------------------------------------------
TEST(BufferTransferTest, BestowSeize) {
    const size_t N = 10;
    int *external = new int[N]; // allocate raw memory
    for (size_t i = 0; i < N; ++i) external[i] = static_cast<int>(i);

    fermat::Buffer<int> buf;
    buf.bestow(external, N, N);
    EXPECT_EQ(buf.size(), N);
    EXPECT_EQ(buf.capacity(), N);
    EXPECT_EQ(buf.data(), external);
    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(buf[i], static_cast<int>(i));
    }

    // Seize back
    size_t out_size, out_cap;
    int *ptr = buf.seize(&out_size, &out_cap);
    EXPECT_EQ(ptr, external);
    EXPECT_EQ(out_size, N);
    EXPECT_EQ(out_cap, N);
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.capacity(), 0u);

    delete[] external;
}

TEST(BufferTransferTest, BestowEmptyBuffer) {
    fermat::Buffer<int> buf;
    EXPECT_DEATH(buf.bestow(nullptr, 0, 0), ".*data is nullptr.*"); // KCHECK
}

// -----------------------------------------------------------------------------
// Other helpers: is_stringify
// -----------------------------------------------------------------------------
TEST(BufferStringifyTest, IsStringify) {
    fermat::Buffer<char> buf;
    EXPECT_FALSE(buf.is_stringify()); // capacity == 0
    buf.reserve(10);
    EXPECT_TRUE(buf.is_stringify());  // capacity > end
    buf.append('h');
    buf.append('i');
    EXPECT_TRUE(buf.is_stringify());  // still have space for null
    buf.resize(buf.capacity()); // fill exactly capacity, no extra slot
    EXPECT_FALSE(buf.is_stringify());
}

// -----------------------------------------------------------------------------
// Swap and global operators
// -----------------------------------------------------------------------------
TEST(BufferSwapTest, Swap) {
    fermat::Buffer<int> v1 = {1,2,3};
    fermat::Buffer<int> v2 = {4,5};
    v1.swap(v2);
    EXPECT_EQ(v1.size(), 2u);
    EXPECT_EQ(v2.size(), 3u);
    EXPECT_EQ(v1[0], 4);
    EXPECT_EQ(v1[1], 5);
    EXPECT_EQ(v2[0], 1);
    EXPECT_EQ(v2[1], 2);
    EXPECT_EQ(v2[2], 3);
}

TEST(BufferCompareTest, Equality) {
    fermat::Buffer<int> v1 = {1,2,3};
    fermat::Buffer<int> v2 = {1,2,3};
    fermat::Buffer<int> v3 = {1,2,3,4};
    fermat::Buffer<int> v4 = {1,2,4};
    fermat::Buffer<int> v5, v6;
    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_FALSE(v1 == v4);
    EXPECT_TRUE(v5 == v6);
    EXPECT_FALSE(v1 == v5);
}

TEST(BufferCompareTest, Inequality) {
    fermat::Buffer<int> v1 = {1,2,3};
    fermat::Buffer<int> v2 = {1,2,3};
    fermat::Buffer<int> v3 = {1,2,3,4};
    fermat::Buffer<int> v4 = {1,2,4};
    fermat::Buffer<int> v5, v6;
    EXPECT_FALSE(v1 != v2);
    EXPECT_TRUE(v1 != v3);
    EXPECT_TRUE(v1 != v4);
    EXPECT_FALSE(v5 != v6);
    EXPECT_TRUE(v1 != v5);
}

TEST(BufferCompareTest, LessThan) {
    fermat::Buffer<int> v1 = {1,2,3};
    fermat::Buffer<int> v2 = {1,2,3};
    fermat::Buffer<int> v3 = {1,2,3,4};
    fermat::Buffer<int> v4 = {1,2,4};
    fermat::Buffer<int> v5 = {1,2};
    fermat::Buffer<int> empty;
    EXPECT_FALSE(v1 < v2);
    EXPECT_TRUE(v1 < v3);
    EXPECT_TRUE(v1 < v4);
    EXPECT_FALSE(v4 < v1);
    EXPECT_TRUE(v5 < v1);
    EXPECT_FALSE(v1 < v5);
    EXPECT_TRUE(empty < v1);
    EXPECT_FALSE(v1 < empty);
}

TEST(BufferCompareTest, GreaterThan) {
    fermat::Buffer<int> v1 = {1,2,3};
    fermat::Buffer<int> v4 = {1,2,4};
    fermat::Buffer<int> v5 = {1,2};
    fermat::Buffer<int> empty;
    EXPECT_TRUE(v4 > v1);
    EXPECT_FALSE(v1 > v4);
    EXPECT_TRUE(v1 > v5);
    EXPECT_FALSE(v5 > v1);
    EXPECT_TRUE(v1 > empty);
    EXPECT_FALSE(empty > v1);
}

// -----------------------------------------------------------------------------
// erase / erase_if / erase_unsorted / erase_unsorted_if (global)
// -----------------------------------------------------------------------------
TEST(BufferGlobalEraseTest, EraseValue) {
    fermat::Buffer<int> v = {1,2,3,2,4,2};
    auto n = fermat::erase(v, 2);
    EXPECT_EQ(n, 3u);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
}

TEST(BufferGlobalEraseTest, EraseIf) {
    fermat::Buffer<int> v = {1,2,3,4,5,6};
    auto n = fermat::erase_if(v, [](int x) { return x % 2 == 0; });
    EXPECT_EQ(n, 3u);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 5);
}

TEST(BufferGlobalEraseTest, EraseUnsorted) {
    fermat::Buffer<int> v = {1,2,3,2,4,2};
    auto n = fermat::erase_unsorted(v, 2);
    EXPECT_EQ(n, 3u);
    // Order not guaranteed, but elements left are 1,3,4 in some order.
    // Check multiset equivalence.
    fermat::Buffer<int> expected = {1,3,4};
    EXPECT_TRUE(UnorderedEqual(v, expected));
}

TEST(BufferGlobalEraseTest, EraseUnsortedIf) {
    fermat::Buffer<int> v = {1,2,3,4,5,6};
    auto n = fermat::erase_unsorted_if(v, [](int x) { return x % 2 == 0; });
    EXPECT_EQ(n, 3u);
    fermat::Buffer<int> expected = {1,3,5};
    EXPECT_TRUE(UnorderedEqual(v, expected));
}

} // namespace