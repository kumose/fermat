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
//

#include <gtest/gtest.h>
#include <fermat/memory/malloc.h>
#include <cstring>

namespace fermat {

// ============================================================================
// Tests for Malloc (non‑aligned)
// ============================================================================

TEST(MallocTest, GoodAllocSize) {
    size_t small = 1;
    size_t rounded = Malloc::good_alloc_size(small);
    EXPECT_GE(rounded, small);
    // mimalloc typically rounds up to 8 or 16; we just check it's not smaller.
    EXPECT_LE(rounded, 16);  // robust upper bound

    size_t exact = 64;
    size_t rounded2 = Malloc::good_alloc_size(exact);
    EXPECT_GE(rounded2, exact);
}

TEST(MallocTest, GoodAllocAndFree) {
    size_t n = 100;
    void* ptr = Malloc::good_alloc(&n);
    ASSERT_NE(ptr, nullptr);
    EXPECT_GE(Malloc::good_usable_size(ptr), 100);
    Malloc::good_free(ptr, n);
}

TEST(MallocTest, GoodAllocUpdatesSize) {
    size_t n = 13;
    size_t orig_n = n;
    void* ptr = Malloc::good_alloc(&n);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(n, Malloc::good_alloc_size(orig_n));
    Malloc::good_free(ptr, n);
}

TEST(MallocTest, GoodRealloc) {
    size_t n1 = 100;
    void* ptr1 = Malloc::good_alloc(&n1);
    ASSERT_NE(ptr1, nullptr);
    std::memset(ptr1, 0xAB, n1);

    size_t n2 = 300;
    void* ptr2 = Malloc::good_realloc(ptr1, &n2);
    ASSERT_NE(ptr2, nullptr);
    // first 100 bytes preserved
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_EQ(static_cast<unsigned char*>(ptr2)[i], 0xAB);
    }

    size_t n3 = 50;
    void* ptr3 = Malloc::good_realloc(ptr2, &n3);
    ASSERT_NE(ptr3, nullptr);
    for (size_t i = 0; i < 50; ++i) {
        EXPECT_EQ(static_cast<unsigned char*>(ptr3)[i], 0xAB);
    }

    Malloc::good_free(ptr3, n3);
}

// ============================================================================
// Tests for AlignedMalloc<Alignment>
// ============================================================================

TEST(AlignedMallocTest, AlignedAlloc64) {
    constexpr size_t Alignment = 64;
    using AM = AlignedMalloc<Alignment>;
    size_t n = 128;
    void* ptr = AM::good_alloc(&n);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(AM::is_aligned(ptr));
    EXPECT_TRUE(AM::is_aligned_size(n));
    EXPECT_GE(AM::good_usable_size(ptr), 128);
    AM::good_free(ptr, n);
}

TEST(AlignedMallocTest, AlignedAlloc128) {
    constexpr size_t Alignment = 128;
    using AM = AlignedMalloc<Alignment>;
    size_t n = 256;
    void* ptr = AM::good_alloc(&n);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(AM::is_aligned(ptr));
    EXPECT_TRUE(AM::is_aligned_size(n));
    AM::good_free(ptr, n);
}

TEST(AlignedMallocTest, AlignedAllocLargeAlignment) {
    constexpr size_t Alignment = 4096;
    using AM = AlignedMalloc<Alignment>;
    size_t n = 8192;
    void* ptr = AM::good_alloc(&n);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(AM::is_aligned(ptr));
    EXPECT_TRUE(AM::is_aligned_size(n));
    EXPECT_GE(AM::good_usable_size(ptr), 8192);
    AM::good_free(ptr, n);
}

TEST(AlignedMallocTest, AlignedFreeSizeAligned) {
    constexpr size_t Alignment = 64;
    using AM = AlignedMalloc<Alignment>;
    size_t n = 128;
    void* ptr = AM::good_alloc(&n);
    ASSERT_NE(ptr, nullptr);
    AM::good_free(ptr, n);
    // no crash
}

TEST(AlignedMallocTest, AlignedAllocZeroSize) {
    constexpr size_t Alignment = 64;
    using AM = AlignedMalloc<Alignment>;
    size_t n = 0;
    void* ptr = AM::good_alloc(&n);
    // Some allocators return nullptr for zero size; others return a minimal chunk.
    // We only require that it does not crash. If non‑null, we must free it.
    if (ptr != nullptr) {
        AM::good_free(ptr, n);
    }
}

TEST(AlignedMallocTest, AlignmentCheckFunctions) {
    constexpr size_t Alignment = 64;
    using AM = AlignedMalloc<Alignment>;
    char buffer[128];
    void* unaligned = &buffer[1];
    EXPECT_FALSE(AM::is_aligned(unaligned));
    EXPECT_TRUE(AM::is_aligned_size(64));
    EXPECT_TRUE(AM::is_aligned_size(128));
    EXPECT_FALSE(AM::is_aligned_size(65));
}

} // namespace fermat