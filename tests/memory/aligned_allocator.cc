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
#include <string>

namespace fermat {

// ============================================================================
// Tests for AlignedAllocator<T, Alignment>
// ============================================================================

TEST(AlignedAllocatorTest, AlignmentProperty) {
    constexpr size_t Align64 = 64;
    AlignedAllocator<int, Align64> alloc64;
    EXPECT_EQ(alloc64.alignment(), Align64);

    constexpr size_t Align128 = 128;
    AlignedAllocator<double, Align128> alloc128;
    EXPECT_EQ(alloc128.alignment(), Align128);
}

TEST(AlignedAllocatorTest, IsAligned) {
    constexpr size_t Align = 64;
    AlignedAllocator<char, Align> alloc;
    char* p = alloc.allocate(1024);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(alloc.is_aligned(p));
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % Align, 0);
    alloc.deallocate(p, 1024);
}

TEST(AlignedAllocatorTest, IsAlignedSize) {
    constexpr size_t Align = 64;
    AlignedAllocator<char, Align> alloc;
    EXPECT_TRUE(alloc.is_aligned_size(0));     // 0 is trivially aligned
    EXPECT_TRUE(alloc.is_aligned_size(64));
    EXPECT_TRUE(alloc.is_aligned_size(128));
    EXPECT_FALSE(alloc.is_aligned_size(32));
    EXPECT_FALSE(alloc.is_aligned_size(100));
}

TEST(AlignedAllocatorTest, AllocateAlignedMemory) {
    constexpr size_t Align = 64;
    AlignedAllocator<int, Align> alloc;
    int* p = alloc.allocate(100);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % Align, 0);
    alloc.deallocate(p, 100);
}

TEST(AlignedAllocatorTest, DeallocateWithAlign) {
    constexpr size_t Align = 64;
    AlignedAllocator<char, Align> alloc;
    char* p = alloc.allocate(256);
    ASSERT_NE(p, nullptr);
    alloc.deallocate(p, 256);
    // No crash, memory correctly freed.
}

TEST(AlignedAllocatorTest, RebindPreservesAlignment) {
    constexpr size_t Align = 64;
    AlignedAllocator<int, Align> intAlloc;
    using CharAlloc = typename decltype(intAlloc)::template rebind<char>::other;
    CharAlloc charAlloc;
    EXPECT_EQ(charAlloc.alignment(), Align);
    // Use the rebound allocator
    char* p = charAlloc.allocate(512);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % Align, 0);
    charAlloc.deallocate(p, 512);
}

TEST(AlignedAllocatorTest, WithContainer) {
    constexpr size_t Align = 64;
    // std::basic_string uses an allocator of char type.
    using AlignedString = std::basic_string<char, std::char_traits<char>, AlignedAllocator<char, Align>>;
    AlignedString str;
    str = "Hello, aligned world!";
    // Check that the internal buffer is aligned to 64 bytes.
    // We can't directly access the buffer, but we can verify that the allocator's
    // allocate is used. As a simple test, allocate a large string and check alignment
    // of its data(). However, data() may be SSO for small strings. Force a heap allocation.
    str.resize(1024, 'x');
    const char* data = str.data();
    EXPECT_EQ(reinterpret_cast<uintptr_t>(data) % Align, 0);
    // Container operations work
    str += " suffix";
    EXPECT_EQ(str.size(), 1024 + 7);
}

TEST(AlignedAllocatorTest, MoveAndSwap) {
    constexpr size_t Align = 64;
    using Alloc = AlignedAllocator<int, Align>;
    std::vector<int, Alloc> v1, v2;
    for (int i = 0; i < 100; ++i) v1.push_back(i);
    for (int i = 0; i < 200; ++i) v2.push_back(i);

    // Move construction
    std::vector<int, Alloc> v3 = std::move(v1);
    EXPECT_EQ(v3.size(), 100);
    EXPECT_EQ(v1.size(), 0);

    // Move assignment
    v2 = std::move(v3);
    EXPECT_EQ(v2.size(), 100);
    EXPECT_EQ(v3.size(), 0);

    // Swap
    std::vector<int, Alloc> v4;
    v4.push_back(42);
    v2.swap(v4);
    EXPECT_EQ(v2.size(), 1);
    EXPECT_EQ(v2[0], 42);
    EXPECT_EQ(v4.size(), 100);
}

} // namespace fermat