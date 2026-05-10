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

#include <gtest/gtest.h>
#include <fermat/memory/allocator.h>
#include <vector>
#include <list>
#include <memory>
#include <atomic>

namespace fermat {

// Helper type to track construction/destruction
struct Tracked {
    static std::atomic<int> constructed;
    static std::atomic<int> destructed;
    int value;

    Tracked(int v = 0) : value(v) { ++constructed; }
    Tracked(const Tracked&) = delete;
    Tracked(Tracked&&) = delete;
    ~Tracked() { ++destructed; }
};
std::atomic<int> Tracked::constructed{0};
std::atomic<int> Tracked::destructed{0};

// ============================================================================
// Tests for Allocator<T>
// ============================================================================

TEST(AllocatorTest, AllocateDeallocate) {
    Allocator<int> alloc;
    int* p = alloc.allocate(10);
    ASSERT_NE(p, nullptr);
    // Write something to ensure memory is usable
    for (int i = 0; i < 10; ++i) p[i] = i;
    for (int i = 0; i < 10; ++i) EXPECT_EQ(p[i], i);
    alloc.deallocate(p, 10);
    // no crash
}

TEST(AllocatorTest, ConstructDestroy) {
    Allocator<Tracked> alloc;
    Tracked::constructed = 0;
    Tracked::destructed = 0;

    Tracked* p = alloc.allocate(1);
    ASSERT_NE(p, nullptr);
    alloc.construct(p, 42);
    EXPECT_EQ(p->value, 42);
    EXPECT_EQ(Tracked::constructed.load(), 1);

    alloc.destroy(p);
    EXPECT_EQ(Tracked::destructed.load(), 1);
    alloc.deallocate(p, 1);
}

TEST(AllocatorTest, Rebind) {
    Allocator<int> intAlloc;
    using CharAlloc = typename Allocator<int>::template rebind<char>::other;
    CharAlloc charAlloc(intAlloc);  // should be convertible
    // Use it with a container
    std::vector<char, CharAlloc> vec(charAlloc);
    vec.push_back('a');
    vec.push_back('b');
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], 'a');
    EXPECT_EQ(vec[1], 'b');
}

TEST(AllocatorTest, MaxSize) {
    Allocator<int> alloc;
    size_t max = alloc.max_size();
    EXPECT_GT(max, 0);
    // max_size() should not be unreasonably small
    EXPECT_GT(max, 1000);
}

TEST(AllocatorTest, Equality) {
    Allocator<int> a1, a2;
    EXPECT_TRUE(a1 == a2);
    EXPECT_FALSE(a1 != a2);
}

TEST(AllocatorTest, WithContainers) {
    // vector
    std::vector<int, Allocator<int>> vec;
    for (int i = 0; i < 100; ++i) vec.push_back(i);
    EXPECT_EQ(vec.size(), 100);
    EXPECT_EQ(vec[42], 42);
    vec.clear();
    vec.shrink_to_fit();

    // list
    std::list<double, Allocator<double>> lst;
    for (int i = 0; i < 100; ++i) lst.push_back(i * 1.5);
    int count = 0;
    for (auto it = lst.begin(); it != lst.end(); ++it) ++count;
    EXPECT_EQ(count, 100);
}

TEST(AllocatorTest, ExceptionSafety) {
    Allocator<int> alloc;
    size_t huge = alloc.max_size() + 1;
    EXPECT_THROW(alloc.allocate(huge), std::bad_alloc);
}

} // namespace fermat
