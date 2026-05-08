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

namespace fermat {
    using TestIOBuf = IOBuf<64, 4096>;

    class IOBufTest : public ::testing::Test {
    protected:
        void SetUp() override {
        }

        void TearDown() override {
        }
    };

    // Basic borrow and commit flow
    TEST_F(IOBufTest, BasicBorrowCommit) {
        TestIOBuf buf;
        std::string_view data = "hello fermat";

        auto span = buf.borrow().value_or_die();
        ASSERT_GE(span.size(), data.size());
        EXPECT_EQ(buf.size(), 0);

        std::memcpy(span.data(), data.data(), data.size());
        buf.commit(data.size());
        EXPECT_EQ(buf.size(), data.size());
        EXPECT_EQ(buf.flatten_aligned<64>(), data);
    }

    // Multi-block borrow with combine
    TEST_F(IOBufTest, MultiBlockCombineBorrow) {
        TestIOBuf buf;
        size_t big_size = 8192 + 100; // > 2 blocks

        auto spans = buf.borrow(big_size, 4096).value_or_die();
        EXPECT_GE(spans.size(), 3);

        for (auto s: spans) {
            std::memset(s.data(), 'A', s.size());
        }
        buf.commit(big_size);
        EXPECT_EQ(buf.size(), big_size);

        auto reclaimed = buf.shrink().value_or_die();
        EXPECT_EQ(reclaimed, 0); // no trailing empty writeable block
    }

    // Move append (steal ownership)
    TEST_F(IOBufTest, MoveAppendOwnership) {
        TestIOBuf buf1;
        std::string_view s1 = "part1_";
        auto span1 = buf1.borrow().value_or_die();
        std::memcpy(span1.data(), s1.data(), s1.size());
        buf1.commit(s1.size());

        TestIOBuf buf2;
        std::string_view s2 = "part2";
        auto span2 = buf2.borrow().value_or_die();
        std::memcpy(span2.data(), s2.data(), s2.size());
        buf2.commit(s2.size());

        auto status = buf1.append(std::move(buf2));
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(buf1.size(), s1.size() + s2.size());
        EXPECT_EQ(buf2.size(), 0);
        EXPECT_EQ(buf1.flatten(), "part1_part2");
    }

    // Append_to shares and seals local blocks
    TEST_F(IOBufTest, AppendToStatusLock) {
        TestIOBuf buf1;
        auto span1 = buf1.borrow().value_or_die();
        std::memcpy(span1.data(), "original", 8);
        buf1.commit(8);

        TestIOBuf buf2;
        auto status = buf1.append_to(buf2);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(buf2.size(), 8);

        // buf1's existing block is now Immutable, next borrow must allocate a new block
        auto new_span = buf1.borrow().value_or_die();
        EXPECT_NE(new_span.data(), span1.data());
    }


    // Preappend_to – share to the front of another buffer
    TEST_F(IOBufTest, PreappendToShared) {
        TestIOBuf src;
        EXPECT_TRUE(src.append("prefix").ok());
        TestIOBuf dst;
        ASSERT_TRUE(dst.append("suffix").ok());

        auto status = src.prepend_to(dst);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(dst.size(), 12);
        KLOG(INFO)<<"dst:"<<dst._views.size();
        EXPECT_EQ(dst.flatten(), "prefixsuffix");
        // src is sealed, but its data remains valid (shared)
        KLOG(INFO)<<"src:"<<src._views.size();
        EXPECT_EQ(src.flatten(), "prefix");
    }

    // Partial commit across multiple borrowed blocks
    TEST_F(IOBufTest, PartialCommitAcrossBlocks) {
        TestIOBuf buf;
        auto spans = buf.borrow(3 * 4096, 0).value_or_die();
        ASSERT_GE(spans.size(), 3);
        // Fully fill first block
        std::memset(spans[0].data(), 'A', spans[0].size());
        // Half fill second block
        size_t half_second = spans[1].size() / 2;
        std::memset(spans[1].data(), 'B', half_second);
        buf.commit(spans[0].size() + half_second);

        EXPECT_EQ(buf.size(), spans[0].size() + half_second);
        // The remaining part of second block and the whole third block are still borrowed,
        // but commit() resets their status to Writeable.
        auto cont = buf.borrow().value_or_die();
        // Should point to the unwritten part of second block
        EXPECT_EQ(cont.data(), spans[1].data() + half_second);
    }

    // Append copy (non‑zero‑copy)
    TEST_F(IOBufTest, AppendCopy) {
        TestIOBuf buf;
        std::string data = "Hello, IOBuf!";
        auto status = buf.append(data);
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(buf.size(), data.size());
        EXPECT_EQ(buf.flatten(), data);
    }

    // Flatten during borrowing – only committed data appears
    TEST_F(IOBufTest, FlattenDuringBorrowing) {
        TestIOBuf buf;
        ASSERT_TRUE(buf.append("committed").ok());
        auto span = buf.borrow().value_or_die();
        std::memcpy(span.data(), "uncommitted", 11);
        EXPECT_EQ(buf.flatten(), "committed");
        buf.commit(11);
        EXPECT_EQ(buf.flatten(), "committeduncommitted");
    }

    // Pop front then continue writing
    TEST_F(IOBufTest, PopFrontThenWrite) {
        TestIOBuf buf;
        ASSERT_TRUE(buf.append("12345678").ok());
        buf.pop_front(4); // leaves "5678"
        ASSERT_TRUE(buf.append("90").ok());
        EXPECT_EQ(buf.flatten(), "567890");
    }

    // Block view and block size after pop_front
    TEST_F(IOBufTest, BlockViewAfterPopFront) {
        TestIOBuf buf;
        const size_t block_sz = TestIOBuf::kBlockSize;
        std::string large(block_sz + 100, 'X');  // first block full, second block 100 bytes
        ASSERT_TRUE(buf.append(large).ok());
        ASSERT_TRUE(buf.append("end").ok());
        EXPECT_GE(buf.block_size(), 2);  // before pop, at least 2 blocks

        buf.pop_front(block_sz);          // consume first full block
        // Remaining: second block contains 100 'X' + "end" = 103 bytes
        EXPECT_EQ(buf.flatten(), std::string(100, 'X') + "end");
        EXPECT_EQ(buf.block_size(), 1);   // only the second block remains

        auto fragment = buf.block_view(0);
        EXPECT_EQ(fragment.size(), 103u);
        EXPECT_EQ(fragment, std::string(100, 'X') + "end");

        auto second = buf.block_view(1);
        EXPECT_TRUE(second.empty());      // no second block
    }
    // Constructor with prefix reservation
    TEST_F(IOBufTest, ConstructorWithPrefixReserve) {
        TestIOBuf buf(10, 5); // 5 prefix Umount blocks, reserve 10 blocks capacity
        // No data initially
        ASSERT_TRUE(buf.append("data").ok());
        EXPECT_EQ(buf.flatten(), "data");
        // Prepend should reuse the prefix slots efficiently
        TestIOBuf header;
        ASSERT_TRUE(header.append("HEAD").ok());
        auto status = buf.prepend(std::move(header));
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(buf.flatten(), "HEADdata");
    }

    // Borrow while already borrowing should fail
    TEST_F(IOBufTest, BorrowDuringBorrowingFails) {
        TestIOBuf buf;
        auto span = buf.borrow().value_or_die();
        auto second = buf.borrow();
        EXPECT_FALSE(second.ok()); // should return error status
        EXPECT_EQ(second.status().message(), "resource locked: block is currently under borrowing transaction");
        buf.commit(0); // release the borrow
        auto third = buf.borrow();
        EXPECT_TRUE(third.ok());
    }

    // Shrink_immutable seals a non‑empty last Writeable block
    TEST_F(IOBufTest, ShrinkImmutableSealsWriteableBlock) {
        TestIOBuf buf;
        // Borrow enough to get two blocks
        auto spans = buf.borrow(2 * 4096, 0).value_or_die();
        // Fill first block, leave second block empty
        std::memset(spans[0].data(), 'A', spans[0].size());
        buf.commit(spans[0].size());
        // Now second block is Writeable with length=0
        // Borrow and write 100 bytes to second block
        auto span2 = buf.borrow().value_or_die();
        std::memset(span2.data(), 'B', 100);
        buf.commit(100);
        // Second block now has length=100, status=Writeable (not full)
        auto reclaimed = buf.shrink_immutable().value_or_die();
        EXPECT_EQ(reclaimed, 0); // no block freed
        // After sealing, index should point to end, next borrow allocates new block
        auto new_span = buf.borrow().value_or_die();
        EXPECT_NE(new_span.data(), span2.data());
    }
} // namespace fermat
