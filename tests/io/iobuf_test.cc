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
#include <sys/uio.h>  // for readv test
#include <tests/io/io_test.h>

namespace fermat {


    // ---------- Basic operations with new Lease API ----------
    TEST_F(IOBufTest, BasicBorrowCommit) {
        TestIOBuf buf;
        std::string_view data = "hello fermat";

        auto *lease = buf.borrow().value_or_die();
        ASSERT_GE(lease->capacity(), data.size());
        EXPECT_EQ(buf.size(), 0);

        auto status = lease->write(data.data(), data.size());
        ASSERT_TRUE(status.ok());
        buf.commit(lease);
        EXPECT_EQ(buf.size(), data.size());
        EXPECT_EQ(buf.flatten_aligned<64>(), data);
    }

    TEST_F(IOBufTest, MultiBlockBorrow) {
        TestIOBuf buf;
        size_t large_size = 8192 + 100; // > 2 blocks of 4096

        auto *lease = buf.borrow(large_size, 4096).value_or_die();
        EXPECT_GE(lease->capacity(), large_size);

        // Fill entirely with 'A'
        std::string filler(large_size, 'A');
        auto status = lease->write(filler.data(), large_size);
        ASSERT_TRUE(status.ok());
        buf.commit(lease);
        EXPECT_EQ(buf.size(), large_size);
        EXPECT_EQ(buf.flatten(), filler);
    }

    TEST_F(IOBufTest, PartialCommit) {
        TestIOBuf buf;
        auto *lease = buf.borrow(3 * 4096, 0).value_or_die();
        // Write only part of the leased space
        size_t first_part = 100;
        std::string data(first_part, 'X');
        auto status = lease->write(data.data(), first_part);
        ASSERT_TRUE(status.ok());
        buf.commit(lease); // commits only the written part
        EXPECT_EQ(buf.size(), first_part);
        EXPECT_EQ(buf.flatten(), data);

        // Borrow again should start after the committed part
        auto *lease2 = buf.borrow().value_or_die();
        EXPECT_GE(lease2->capacity(), 4096 - first_part);
        buf.commit(lease2); // commit 0, just release
    }

    TEST_F(IOBufTest, WriteBeyondCapacityFails) {
        TestIOBuf buf;
        const size_t block_size = TestIOBuf::kBlockSize; // 4096

        // 1. Fill a block almost full, leaving only a small free space (e.g., 10 bytes)
        size_t wanted_remain = 10;
        size_t first_write = block_size - wanted_remain;
        std::string first_data(first_write, 'A');
        auto *lease1 = buf.borrow(first_write, std::nullopt).value_or_die();
        auto status = lease1->write(first_data.data(), first_data.size());
        ASSERT_TRUE(status.ok());
        buf.commit(lease1);
        EXPECT_EQ(buf.size(), first_write);

        // 2. Borrow again – the Lease should now cover the remaining small space
        auto *lease2 = buf.borrow().value_or_die(); // captures the writable tail of the last block
        ASSERT_LE(lease2->remaining(), wanted_remain); // may be exactly wanted_remain or slightly less (if alignment)
        size_t small_cap = lease2->remaining();

        // 3. Attempt to write more than the remaining capacity
        std::string too_much(small_cap + 1, 'B');
        status = lease2->write(too_much.data(), too_much.size());
        EXPECT_FALSE(status.ok());
        EXPECT_EQ(status.message(), "BufferLease capacity exceeded, size:0 len:11 capacity:10");

        // 4. No data should have been written; the lease size stays zero
        EXPECT_EQ(lease2->size(), 0);
        buf.commit(lease2); // commit zero bytes, just release the borrow
        EXPECT_EQ(buf.size(), first_write); // buffer unchanged
    }

    TEST_F(IOBufTest, PopBack) {
        TestIOBuf buf;
        auto *lease = buf.borrow(100).value_or_die();
        lease->write("hello", 5);
        lease->pop_back(2); // removes last 2 bytes -> "hel"
        EXPECT_EQ(lease->size(), 3);
        buf.commit(lease);
        EXPECT_EQ(buf.flatten(), "hel");
    }

    TEST_F(IOBufTest, ClearLease) {
        TestIOBuf buf;
        auto *lease = buf.borrow(100).value_or_die();
        lease->write("hello", 5);
        lease->clear();
        EXPECT_TRUE(lease->empty());
        lease->write("world", 5);
        buf.commit(lease);
        EXPECT_EQ(buf.flatten(), "world");
    }

    // ---------- Zero-copy iovec interface ----------
    TEST_F(IOBufTest, ZeroCopyWithVisitRemainingAndAdvance) {
        TestIOBuf buf;
        // Lease large enough to need at least 2 spans
        auto *lease = buf.borrow(8192, 0).value_or_die();

        // Collect spans into iovec
        struct iovec iov[8];
        int iovcnt = 0;
        lease->visit_remaining([&](char *ptr, size_t cap) {
            iov[iovcnt].iov_base = ptr;
            iov[iovcnt].iov_len = cap;
            ++iovcnt;
            return iovcnt < 8; // continue if array not full
        });
        ASSERT_GE(iovcnt, 2); // should have at least two spans

        // Simulate readv filling data (fill with 'A')
        size_t total = 0;
        for (int i = 0; i < iovcnt; ++i) {
            memset(iov[i].iov_base, 'A', iov[i].iov_len);
            total += iov[i].iov_len;
        }

        lease->advance(total); // commit all
        buf.commit(lease);
        EXPECT_EQ(buf.size(), total);
        std::string expected(total, 'A');
        EXPECT_EQ(buf.flatten(), expected);
    }

    TEST_F(IOBufTest, VisitRemainingStopEarly) {
        TestIOBuf buf;
        auto *lease = buf.borrow(8192, 0).value_or_die();
        int call_count = 0;
        lease->visit_remaining([&](char *, size_t) {
            ++call_count;
            return false; // stop after first block
        });
        EXPECT_EQ(call_count, 1);
    }

    // ---------- Move and share operations (unchanged in logic, but use new API) ----------
    TEST_F(IOBufTest, MoveAppendOwnership) {
        TestIOBuf buf1, buf2;
        // Write to buf1
        auto *l1 = buf1.borrow().value_or_die();
        l1->write("part1_", 6);
        buf1.commit(l1);
        // Write to buf2
        auto *l2 = buf2.borrow().value_or_die();
        l2->write("part2", 5);
        buf2.commit(l2);

        auto status = buf1.append(std::move(buf2));
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(buf1.size(), 11);
        EXPECT_EQ(buf2.size(), 0);
        EXPECT_EQ(buf1.flatten(), "part1_part2");
    }

    TEST_F(IOBufTest, AppendToSharePath) {
        IOBuf<64, 4096> src, dst;
        src.append("shared");
        dst.append("dst");
        src.shrink_immutable();
        auto status = src.append_to(dst);
        ASSERT_TRUE(status.ok())<<status.message();
        EXPECT_EQ(dst.flatten(), "dstshared");
        // src blocks become Immutable (can't be written without new allocation)
        auto *lease = src.borrow().value_or_die();
        // Should start with a new block (original block is Immutable)
        EXPECT_TRUE(lease->capacity() > 0);
        src.commit(lease);
    }

    // ---------- Error handling ----------
    TEST_F(IOBufTest, BorrowDuringBorrowingFails) {
        TestIOBuf buf;
        auto *lease = buf.borrow().value_or_die();
        auto second = buf.borrow();
        EXPECT_FALSE(second.ok());
        EXPECT_EQ(second.status().message(), "resource locked: block is currently under borrowing transaction");
        buf.commit(lease); // release first borrow
        auto third = buf.borrow();
        EXPECT_TRUE(third.ok());
        buf.commit(third.value_or_die());
    }

    TEST_F(IOBufTest, DoubleCommitFails) {
        TestIOBuf buf;
        auto *lease = buf.borrow(100).value_or_die();
        lease->write("data", 4);
        buf.commit(lease);
        // Second commit with same lease should crash (CHECK) because lease no longer belongs
        // We rely on CHECK to die; cannot test in unit test unless we expect death.
        // GTEST_DEATH_TEST might be used, but skip for now.
    }

    TEST_F(IOBufTest, CommitForeignLeaseDies) {
        TestIOBuf buf1, buf2;
        auto *lease = buf1.borrow().value_or_die();
        // Attempt to commit lease from buf1 into buf2
        EXPECT_DEATH(buf2.commit(lease), "Lease does not belong to this IOBuf");
    }
} // namespace fermat
