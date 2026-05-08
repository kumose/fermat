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
        KLOG(INFO) << "dst:" << dst._views.size();
        EXPECT_EQ(dst.flatten(), "prefixsuffix");
        // src is sealed, but its data remains valid (shared)
        KLOG(INFO) << "src:" << src._views.size();
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
        std::string large(block_sz + 100, 'X'); // first block full, second block 100 bytes
        ASSERT_TRUE(buf.append(large).ok());
        ASSERT_TRUE(buf.append("end").ok());
        EXPECT_GE(buf.blocks(), 2); // before pop, at least 2 blocks

        buf.pop_front(block_sz); // consume first full block
        // Remaining: second block contains 100 'X' + "end" = 103 bytes
        EXPECT_EQ(buf.flatten(), std::string(100, 'X') + "end");
        EXPECT_EQ(buf.blocks(), 1); // only the second block remains

        auto fragment = buf.block_view(0);
        EXPECT_EQ(fragment.size(), 103u);
        EXPECT_EQ(fragment, std::string(100, 'X') + "end");

        auto second = buf.block_view(1);
        EXPECT_TRUE(second.empty()); // no second block
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

    TEST_F(IOBufTest, PrependCopyToDirect) {
        TestIOBuf src;
        src.append("hello");
        TestIOBuf dst;
        dst.append("world");

        // Call the copy prepend method directly
        auto status = src.prepend_copy_to(dst);
        ASSERT_TRUE(status.ok());

        EXPECT_EQ(dst.flatten(), "helloworld");
        // src should be unchanged
        EXPECT_EQ(src.flatten(), "hello");

        // Verify independence: modify src
        src.append("!");
        EXPECT_EQ(src.flatten(), "hello!");
        EXPECT_EQ(dst.flatten(), "helloworld"); // dst unchanged

        // Also verify no block sharing: we can check reference counts if we had access, but not necessary.
    }

    TEST_F(IOBufTest, PrependCopyToViaPrependTo) {
        // Different alignment forces copy path
        IOBuf<128, 4096> src;
        IOBuf<64, 4096> dst;

        ASSERT_TRUE(src.append("hello").ok());
        ASSERT_TRUE(dst.append("world").ok());

        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "helloworld");
        EXPECT_EQ(src.flatten(), "hello");

        // Modify src, dst unchanged (copy, not share)
        ASSERT_TRUE(src.append("!").ok());
        EXPECT_EQ(src.flatten(), "hello!");
        EXPECT_EQ(dst.flatten(), "helloworld");
    }

    // Test prepend_copy_to path with multi-block source and target (force copy due to different alignment)
    TEST_F(IOBufTest, PrependCopyToMultiBlockDifferentAlignment) {
        // Source: 128-byte alignment, target: 64-byte alignment -> share_able_to false
        IOBuf<128, 4096> src;
        IOBuf<64, 4096> dst;

        // Fill source with enough data to occupy at least 2 blocks (8KB + 100 bytes)
        std::string src_data(8192 + 100, 'A');
        ASSERT_TRUE(src.append(src_data).ok());

        // Target already contains some data
        ASSERT_TRUE(dst.append("suffix").ok());

        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());

        // Expected result: src_data + "suffix"
        EXPECT_EQ(dst.flatten(), src_data + "suffix");
        EXPECT_EQ(src.flatten(), src_data); // src unchanged

        // Modify src, dst should remain unchanged (deep copy)
        ASSERT_TRUE(src.append("!").ok());
        EXPECT_EQ(src.flatten(), src_data + "!");
        EXPECT_EQ(dst.flatten(), src_data + "suffix");
    }

    // Test prepend_copy_to reusing head Umount slots in target
    TEST_F(IOBufTest, PrependCopyToReuseHeadUmountSlots) {
        // Create target with prefix reservation (5 Umount blocks at head)
        IOBuf<64, 4096> dst(10, 5); // 5 Umount, capacity 10 blocks
        ASSERT_TRUE(dst.append("world").ok());

        // Source with different alignment to force copy path
        IOBuf<128, 4096> src;
        // Fill source with data that fits exactly into 3 standard blocks (4096 bytes each)
        // Actually 3 * 4096 = 12288, but we'll use 12288 bytes to ensure 3 blocks
        std::string src_data(12288, 'B');
        ASSERT_TRUE(src.append(src_data).ok());

        // Before prepend, dst has _views: [Umount x5, Block0 (world)]
        // After prepend, should reuse first 3 Umount slots directly without vector rebuild
        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());

        // dst should have: src_data (3 blocks) + "world" (1 block)
        EXPECT_EQ(dst.flatten(), src_data + "world");
        // The total size should be correct
        EXPECT_EQ(dst.size(), src_data.size() + 5); // "world" is 5 bytes

        // Verify that the Umount slots were overwritten and _view_start reduced
        EXPECT_EQ(dst._view_start, 2); // originally 5, minus 3 inserted blocks
    }

    // Test prepend_copy_to when target has no Umount slots (full rebuild)
    TEST_F(IOBufTest, PrependCopyToNoUmountSlots) {
        IOBuf<128, 4096> src;
        IOBuf<64, 4096> dst;
        // No prefix reservation, target has only one block with data
        ASSERT_TRUE(dst.append("world").ok());

        // Source data spans 2 blocks
        std::string src_data(8192, 'C'); // exactly 2 blocks of 4096 each
        ASSERT_TRUE(src.append(src_data).ok());

        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());

        EXPECT_EQ(dst.flatten(), src_data + "world");
        EXPECT_EQ(dst.size(), src_data.size() + 5);

        // After rebuild, _view_start should be 0
        EXPECT_EQ(dst._view_start, 0);
    }

    // Test prepend_copy_to with source spanning multiple fragments (already covered)
    // but also ensure target data remains writable after prepend (immutable blocks unaffected)
    TEST_F(IOBufTest, PrependCopyToTargetStillWritable) {
        IOBuf<128, 4096> src;
        IOBuf<64, 4096> dst;
        ASSERT_TRUE(src.append("prefix").ok());
        ASSERT_TRUE(dst.append("original").ok());

        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());

        // After prepend, dst should have "prefixoriginal"
        EXPECT_EQ(dst.flatten(), "prefixoriginal");

        // dst should still allow appending new data (target's mutable state unchanged)
        ASSERT_TRUE(dst.append("!").ok());
        EXPECT_EQ(dst.flatten(), "prefixoriginal!");

        // Also test prepending again (second copy)
        IOBuf<128, 4096> src2;
        ASSERT_TRUE(src2.append("another_").ok());
        status = src2.prepend_to(dst);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "another_prefixoriginal!");
    }


    // Positive: block sizes equal
    TEST_F(IOBufTest, PrependMoveSameBlockSize) {
        TestIOBuf src, dst;
        src.append("A");
        dst.append("B");
        auto status = dst.prepend(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "AB");
    }

    // Negative: alignment mismatch
    TEST_F(IOBufTest, PrependMoveDifferentAlignment) {
        IOBuf<128, 4096> src;
        IOBuf<64, 4096> dst;
        src.append("X");
        dst.append("Y");
        auto status = dst.prepend(std::move(src));
        ASSERT_FALSE(status.ok());
        EXPECT_TRUE(status.message().find("alignment") != std::string::npos);
        EXPECT_EQ(dst.flatten(), "Y"); // unchanged
        EXPECT_EQ(src.flatten(), "X"); // src not moved
    }

    // Positive: same alignment, lhs block size >= this block size
    TEST_F(IOBufTest, PrependMoveShareable) {
        IOBuf<64, 8192> src; // block size 8192
        IOBuf<64, 4096> dst; // block size 4096, same alignment

        ASSERT_TRUE(src.append("hello").ok());
        ASSERT_TRUE(dst.append("world").ok());

        auto status = dst.prepend(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "helloworld");
        EXPECT_EQ(src.size(), 0); // moved-from
    }

    // Negative: lhs block size < this block size (fails share_able_to)
    TEST_F(IOBufTest, PrependMoveLhsSmallerBlockSize) {
        IOBuf<64, 2048> src; // block size 2048
        IOBuf<64, 4096> dst; // block size 4096
        src.append("small");
        dst.append("large");
        auto status = dst.prepend(std::move(src));
        ASSERT_FALSE(status.ok());
        EXPECT_TRUE(status.message().find("size mismatch") != std::string::npos);
        EXPECT_EQ(dst.flatten(), "large");
        EXPECT_EQ(src.flatten(), "small");
    }

    // Positive case: append move with same alignment and compatible block size (source block size >= target block size)
    TEST_F(IOBufTest, AppendMoveShareable) {
        IOBuf<64, 8192> src; // block size 8192 >= 4096
        IOBuf<64, 4096> dst;
        src.append("world");
        dst.append("hello");
        auto status = dst.append(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "helloworld");
        EXPECT_EQ(src.size(), 0); // src moved-from
        EXPECT_TRUE(src.flatten().empty());
    }

    // Positive case: equal block sizes
    TEST_F(IOBufTest, AppendMoveSameBlockSize) {
        TestIOBuf src, dst;
        src.append("foo");
        dst.append("bar");
        auto status = dst.append(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "barfoo");
        EXPECT_TRUE(src.flatten().empty());
    }

    // Negative case: alignment mismatch
    TEST_F(IOBufTest, AppendMoveDifferentAlignment) {
        IOBuf<128, 4096> src;
        IOBuf<64, 4096> dst;
        src.append("X");
        dst.append("Y");
        auto status = dst.append(std::move(src));
        ASSERT_FALSE(status.ok());
        EXPECT_TRUE(status.message().find("alignment") != std::string::npos);
        EXPECT_EQ(dst.flatten(), "Y"); // unchanged
        EXPECT_EQ(src.flatten(), "X"); // src not moved
    }

    // Negative case: source block size < target block size (fails share_able_to)
    TEST_F(IOBufTest, AppendMoveSourceSmallerBlockSize) {
        IOBuf<64, 2048> src;
        IOBuf<64, 4096> dst;
        src.append("small");
        dst.append("large");
        auto status = dst.append(std::move(src));
        ASSERT_FALSE(status.ok());
        EXPECT_TRUE(status.message().find("size mismatch") != std::string::npos);
        EXPECT_EQ(dst.flatten(), "large");
        EXPECT_EQ(src.flatten(), "small");
    }

    // Edge case: appending empty source
    TEST_F(IOBufTest, AppendMoveEmptySource) {
        TestIOBuf src; // empty
        TestIOBuf dst;
        dst.append("data");
        auto status = dst.append(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "data");
        EXPECT_EQ(src.size(), 0);
    }

    // Edge case: self move (should do nothing)
    TEST_F(IOBufTest, AppendMoveSelf) {
        TestIOBuf buf;
        buf.append("test");
        auto status = buf.append(std::move(buf));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(buf.flatten(), "test");
    }

    // Ensure that moved-from src can be reused after reset (though not required, but safe)
    TEST_F(IOBufTest, AppendMoveReuseMovedFrom) {
        TestIOBuf src, dst;
        src.append("hello");
        dst.append("world");
        dst.append(std::move(src));
        // src is now moved-from, but we can still assign new data or call clear/release
        src.do_release(); // explicit reset (or just use new assignment)
        src.append("new");
        EXPECT_EQ(src.flatten(), "new");
        EXPECT_EQ(dst.flatten(), "worldhello");
    }

    // AppendToSharePath: share_able_to == true -> append_share_to
    // The source blocks become Immutable and reference count increases.
    TEST_F(IOBufTest, AppendToSharePath) {
        IOBuf<64, 4096> src;
        IOBuf<64, 4096> dst;
        src.append("shared");
        dst.append("dst");

        // Get direct access to the first block view (test friend or use public getter)
        const auto& src_view = src._views[src._view_start];
        uint32_t old_ref = src_view.block->ref_count.load();

        auto status = src.append_to(dst);
        ASSERT_TRUE(status.ok());

        // After sharing, src blocks become Immutable and ref_count increased by 1.
        EXPECT_EQ(src_view.status, IOBufBase::BlockStatus::Immutable);
        EXPECT_EQ(src_view.block->ref_count.load(), old_ref + 1);
        EXPECT_EQ(dst.flatten(), "dstshared");
    }

    // AppendToCopyPath: share_able_to == false -> append_copy_to
    // Source blocks stay Writeable and reference count unchanged.
    TEST_F(IOBufTest, AppendToCopyPath) {
        IOBuf<128, 4096> src;      // different alignment -> share_able_to false
        IOBuf<64, 4096> dst;
        src.append("copy");
        dst.append("dst");

        const auto& src_view = src._views[src._view_start];
        uint32_t old_ref = src_view.block->ref_count.load();

        auto status = src.append_to(dst);
        ASSERT_TRUE(status.ok());

        // Source remains Writeable, ref_count unchanged (no sharing).
        EXPECT_EQ(src_view.status, IOBufBase::BlockStatus::Writeable);
        EXPECT_EQ(src_view.block->ref_count.load(), old_ref);
        EXPECT_EQ(dst.flatten(), "dstcopy");
    }
} // namespace fermat
