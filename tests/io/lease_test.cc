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
// ================= Lease Basic Operations =================

TEST_F(IOBufTest, LeaseWriteCommit) {
    TestIOBuf buf;
    std::string data = "hello world";

    auto* lease = buf.borrow().value_or_die();
    ASSERT_NE(lease, nullptr);
    auto status = lease->write(data.data(), data.size());
    ASSERT_TRUE(status.ok());

    buf.commit(lease);
    EXPECT_EQ(buf.size(), data.size());
    EXPECT_EQ(buf.flatten(), data);
}

TEST_F(IOBufTest, LeaseWriteWithSize) {
    TestIOBuf buf;
    size_t requested = 1024;   // less than block size, but guarantee at least 1024
    auto* lease = buf.borrow(requested, std::nullopt).value_or_die();
    EXPECT_GE(lease->capacity(), requested);

    std::string data(requested, 'X');
    auto status = lease->write(data.data(), data.size());
    ASSERT_TRUE(status.ok());
    buf.commit(lease);
    EXPECT_EQ(buf.size(), requested);
    EXPECT_EQ(buf.flatten(), data);
}

TEST_F(IOBufTest, LeaseWriteAcrossSpans) {
    TestIOBuf buf;
    // Trigger multiple blocks: request size > block_size (4096)
    size_t data_size = 8192;   // two full blocks
    std::string data(data_size, 'Y');

    auto* lease = buf.borrow(data_size, 0).value_or_die();
    // The lease may consist of several spans; write() handles crossing automatically
    auto status = lease->write(data.data(), data.size());
    ASSERT_TRUE(status.ok());
    buf.commit(lease);
    EXPECT_EQ(buf.size(), data_size);
    EXPECT_EQ(buf.flatten(), data);
}

    TEST_F(IOBufTest, LeaseWriteBeyondCapacity) {
    TestIOBuf buf;
    const size_t block_size = TestIOBuf::kBlockSize;  // 4096

    // 1. Fill almost the entire first block, leaving only a small free space (e.g., 16 bytes)
    size_t remaining_wanted = 16;
    size_t first_write_size = block_size - remaining_wanted;
    std::string first_data(first_write_size, 'A');
    auto* lease1 = buf.borrow(first_write_size, std::nullopt).value_or_die();
    auto status = lease1->write(first_data.data(), first_data.size());
    ASSERT_TRUE(status.ok());
    buf.commit(lease1);
    EXPECT_EQ(buf.size(), first_write_size);

    // 2. Borrow again – the lease should now cover the remaining small space
    auto* lease2 = buf.borrow().value_or_die();
    size_t small_capacity = lease2->capacity();
    EXPECT_LE(small_capacity, remaining_wanted);  // may be exactly remaining_wanted or less due to alignment
    ASSERT_GT(small_capacity, 0);

    // 3. Try to write more than the small capacity
    std::string too_much(small_capacity + 1, 'B');
    status = lease2->write(too_much.data(), too_much.size());
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), turbo::StatusCode::kOutOfRange);

    // 4. No data should be written; lease2 size remains 0
    EXPECT_EQ(lease2->size(), 0);
    buf.commit(lease2);  // commit 0 bytes, just release
    EXPECT_EQ(buf.size(), first_write_size);  // unchanged
}

TEST_F(IOBufTest, LeasePopBack) {
    TestIOBuf buf;
    auto* lease = buf.borrow(100, std::nullopt).value_or_die();
    std::string original = "abcdefghij";
    auto status = lease->write(original.data(), original.size());
    ASSERT_TRUE(status.ok());

    // Remove last 3 bytes
    lease->pop_back(3);
    EXPECT_EQ(lease->size(), 7);
    buf.commit(lease);
    EXPECT_EQ(buf.flatten(), "abcdefg");
}

TEST_F(IOBufTest, LeaseClear) {
    TestIOBuf buf;
    auto* lease = buf.borrow(100, std::nullopt).value_or_die();

    // First write
    std::string first = "first";
    auto status = lease->write(first.data(), first.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(lease->size(), first.size());

    // Clear – should reset cursor but keep capacity and spans
    lease->clear();
    EXPECT_EQ(lease->size(), 0);
    EXPECT_GT(lease->capacity(), 0);
    EXPECT_TRUE(lease->empty());

    // Write again – should overwrite from the beginning
    std::string second = "second";
    status = lease->write(second.data(), second.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(lease->size(), second.size());

    buf.commit(lease);
    EXPECT_EQ(buf.size(), second.size());
    EXPECT_EQ(buf.flatten(), second);   // "first" should be gone
}

TEST_F(IOBufTest, LeaseRemainingAndBool) {
    TestIOBuf buf;
    auto* lease = buf.borrow(100, std::nullopt).value_or_die();
    size_t cap = lease->capacity();
    EXPECT_EQ(lease->remaining(), cap);
    EXPECT_TRUE(static_cast<bool>(*lease));   // operator bool

    std::string data1(50, 'A');
    auto status = lease->write(data1.data(), data1.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(lease->size(), 50);
    EXPECT_EQ(lease->remaining(), cap - 50);
    EXPECT_TRUE(static_cast<bool>(*lease));

    std::string data2(cap - 50, 'B');
    status = lease->write(data2.data(), data2.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(lease->remaining(), 0);
    EXPECT_FALSE(static_cast<bool>(*lease));   // full, bool becomes false

    buf.commit(lease);
    EXPECT_EQ(buf.size(), cap);
}
}  // namespace fermat
