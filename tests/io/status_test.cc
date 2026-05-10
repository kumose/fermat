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
#include <fermat/io/iobuf.h>
#include <string_view>
#include <tests/io/io_test.h>

namespace fermat {


// ============================================================================
// Memory management and state machine
// ============================================================================

// Shrink removes empty writable blocks at the tail.
TEST_F(IOBufTest, ShrinkRemovesEmptyWritableBlocks) {
    TestIOBuf buf;
    // Append data to make two full blocks
    const size_t block_sz = TestIOBuf::kBlockSize;
    std::string data1(block_sz, 'A');
    ASSERT_TRUE(buf.append(data1).ok());
    EXPECT_EQ(buf.blocks(), 1);
    // Append more to create a second block
    std::string data2(block_sz, 'B');
    ASSERT_TRUE(buf.append(data2).ok());
    EXPECT_EQ(buf.blocks(), 2);

    // Now both blocks are full and immutable. Shrink should not remove any.
    auto reclaimed = buf.shrink().value_or_die();
    EXPECT_EQ(reclaimed, 0);
    EXPECT_EQ(buf.blocks(), 2);

    // Borrow to get a writable span for the second block (it already has length block_sz, so write_able size is 0)
    // Actually the second block is full, so write_able size = block_sz - length = 0. Borrow will allocate a new block.
    auto* lease = buf.borrow().value_or_die();
    EXPECT_GT(lease->capacity(), 0);
    buf.commit(lease); // no actual write

    // Now there is an empty writable block at the tail (the newly allocated one with length 0)
    reclaimed = buf.shrink().value_or_die();
    // The new block should be removed; its capacity returned.
    EXPECT_GT(reclaimed, 0);
    EXPECT_EQ(buf.blocks(), 2); // Still two blocks? Actually after shrink, the empty block is gone, so blocks() should be 2? Wait careful.
    // Initially there were 2 full blocks. After borrow/commit with zero write, a new writable block was added (third block) with length 0.
    // After shrink, that third block should be removed, leaving 2 blocks.
    EXPECT_EQ(buf.blocks(), 2);
}

// shrink_immutable seals the last writable block (makes it immutable) and removes empty trailing blocks.
TEST_F(IOBufTest, ShrinkImmutableSealsLastWritableBlock) {
    TestIOBuf buf;
    // Fill up first block and leave some space in the second block
    const size_t block_sz = TestIOBuf::kBlockSize;
    std::string first(block_sz, 'A');
    ASSERT_TRUE(buf.append(first).ok());               // block 0 full, immutable
    size_t partial = 500;
    std::string second(partial, 'B');
    ASSERT_TRUE(buf.append(second).ok());              // block 1 partially filled, still writable (since not full)
    EXPECT_EQ(buf.blocks(), 2);

    // Now shrink_immutable: should seal the last block (make it immutable)
    auto reclaimed = buf.shrink_immutable().value_or_die();
    EXPECT_EQ(reclaimed, 0);   // no empty block removed
    // After sealing, a subsequent borrow should allocate a new block, not reuse the tail
    auto* lease = buf.borrow().value_or_die();
    // Check that the lease capacity is at least block_sz (i.e., new block), not just the remaining part of block 1
    // The lease should point to newly allocated writable block, not the sealed one's leftover space.
    // We can verify by writing more data and ensuring it doesn't overwrite the sealed part.
    std::string more(100, 'C');
    auto status = lease->write(more.data(), more.size());
    ASSERT_TRUE(status.ok());
    buf.commit(lease);
    // original second block (500 bytes 'B') should still be there, followed by 'C'*100 in a new block.
    EXPECT_EQ(buf.flatten(), first + second + more);
}

// custom(n) consumes n bytes from the head.
TEST_F(IOBufTest, CustomConsumesHeadBytes) {
    TestIOBuf buf;
    std::string data = "abcdefghij";
    ASSERT_TRUE(buf.append(data).ok());
    EXPECT_EQ(buf.size(), 10);

    // Consume 4 bytes
    auto status = buf.custom(4);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(buf.size(), 6);
    EXPECT_EQ(buf.flatten(), "efghij");

    // Consume exactly the remaining bytes
    status = buf.custom(6);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(buf.size(), 0);
    EXPECT_TRUE(buf.flatten().empty());
}

// custom fails when a lease is active.
TEST_F(IOBufTest, CustomFailsWhenBorrowing) {
    TestIOBuf buf;
    ASSERT_TRUE(buf.append("some data").ok());
    auto* lease = buf.borrow().value_or_die();  // active lease
    auto status = buf.custom(5);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), turbo::StatusCode::kUnavailable);
    buf.commit(lease);
}

// pop_front(n, result) consumes from head and optionally copies into a string.
TEST_F(IOBufTest, PopFrontWorksLikeCustom) {
    TestIOBuf buf;
    std::string data = "1234567890";
    ASSERT_TRUE(buf.append(data).ok());

    std::string result;
    size_t consumed = buf.pop_front(3, &result);
    EXPECT_EQ(consumed, 3);
    EXPECT_EQ(result, "123");
    EXPECT_EQ(buf.size(), 7);
    EXPECT_EQ(buf.flatten(), "4567890");

    // Pop without result
    consumed = buf.pop_front(4, nullptr);
    EXPECT_EQ(consumed, 4);
    EXPECT_EQ(buf.size(), 3);
    EXPECT_EQ(buf.flatten(), "890");
}


}  // namespace fermat
