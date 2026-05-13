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
#include <fermat/io/receiver.h>
#include <fermat/io/customer.h>   // provides Customer, Reader
#include <string_view>
#include <tests/io/io_test.h>
#include <fermat/container/buffer.h>

namespace fermat {

// Helper to fill IOBuf with a string (using Lease write)
static void FillIOBuf(TestIOBuf &buf, std::string_view data) {
    auto *lease = buf.borrow(data.size(), std::nullopt).value_or_die();
    auto status = lease->write(data.data(), data.size());
    ASSERT_TRUE(status.ok());
    buf.commit(lease);
}


// ============================================================================
// Tests using Customer (Custom=true) and Reader (Custom=false)
// ============================================================================

TEST_F(IOBufTest, CustomerToAlignedVector) {
    TestIOBuf src;
    std::string payload = "hello from IOBuf to AlignedVector";
    FillIOBuf(src, payload);
    EXPECT_EQ(src.size(), payload.size());

    AlignedVector<char, 64> target;
    ContainerAppender<AlignedVector<char, 64>> receiver(target);
    auto status = Customer::custom(src, receiver, payload.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target.size(), payload.size());
    EXPECT_EQ(std::string(target.data(), target.size()), payload);
    EXPECT_EQ(src.size(), 0);   // source fully consumed
}

TEST_F(IOBufTest, CustomerToAlignedString) {
    TestIOBuf src;
    std::string payload = "AlignedString integration test";
    FillIOBuf(src, payload);
    EXPECT_EQ(src.size(), payload.size());

    AlignedString<64> target;
    ContainerAppender<AlignedString<64>> receiver(target);
    auto status = Customer::custom(src, receiver, payload.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target.size(), payload.size());
    EXPECT_EQ(std::string(target.data(), target.size()), payload);
    EXPECT_EQ(src.size(), 0);
}

TEST_F(IOBufTest, CustomerToStdString) {
    TestIOBuf src;
    std::string payload = "std::string test";
    FillIOBuf(src, payload);
    EXPECT_EQ(src.size(), payload.size());

    std::string target;
    ContainerAppender<std::string> receiver(target);
    auto status = Customer::custom(src, receiver, payload.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target, payload);
    EXPECT_EQ(src.size(), 0);
}

TEST_F(IOBufTest, ReaderCopiesWithoutDrain) {
    TestIOBuf src;
    std::string payload = "copy without drain";
    FillIOBuf(src, payload);
    EXPECT_EQ(src.size(), payload.size());

    std::string target;
    ContainerAppender<std::string> receiver(target);
    auto status = Reader::custom(src, receiver, payload.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target, payload);
    EXPECT_EQ(src.size(), payload.size());   // source unchanged
}


// ============================================================================
// Tests for custom_until (reading until delimiter)
// ============================================================================

TEST_F(IOBufTest, CustomerUntilCrossBlock) {
    TestIOBuf src;
    // Force two blocks: first block ends with "hello,", second begins with "world"
    std::string part1 = "hello,";
    std::string part2 = "world";
    auto *lease1 = src.borrow(part1.size(), std::nullopt).value_or_die();
    lease1->write(part1.data(), part1.size());
    src.commit(lease1);
    auto *lease2 = src.borrow(part2.size(), std::nullopt).value_or_die();
    lease2->write(part2.data(), part2.size());
    src.commit(lease2);

    std::string target;
    ContainerAppender<std::string> receiver(target);
    auto status = Customer::custom_until(src, receiver, ',');
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target, "hello");
    // Source should have consumed "hello," (6 bytes), leaving "world"
    EXPECT_EQ(src.size(), 5);
    EXPECT_EQ(src.flatten(), "world");
}

TEST_F(IOBufTest, CustomerUntilNoDelimiter) {
    TestIOBuf src;
    std::string payload = "no delimiter here";
    FillIOBuf(src, payload);
    std::string target;
    ContainerAppender<std::string> receiver(target);
    auto status = Customer::custom_until(src, receiver, ',');
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target, payload);
    // Since delimiter not found, the entire payload is read and (if Custom=true) consumed.
    // The implementation should consume `payload.size()` bytes, not +1.
    EXPECT_EQ(src.size(), 0);
}


TEST_F(IOBufTest, ReaderUntilDoesNotDrain) {
    TestIOBuf src;
    std::string payload = "test,data";
    FillIOBuf(src, payload);
    std::string target;
    ContainerAppender<std::string> receiver(target);
    auto status = Reader::custom_until(src, receiver, ',');
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target, "test");
    // Source unchanged because Custom=false.
    EXPECT_EQ(src.size(), payload.size());
    EXPECT_EQ(src.flatten(), payload);
}

    TEST_F(IOBufTest, CustomerToOldABuffer) {
    TestIOBuf src;
    std::string payload = "Hello, OldABuffer!";
    FillIOBuf(src, payload);
    EXPECT_EQ(src.size(), payload.size());

    Buffer<char, 64> target;
    ContainerAppender<Buffer<char, 64>> receiver(target);
    auto status = Customer::custom(src, receiver, payload.size());
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(target.size(), payload.size());
    EXPECT_EQ(std::string(target.data(), target.size()), payload);
    EXPECT_EQ(src.size(), 0);  // source consumed
}

    TEST_F(IOBufTest, CustomerUntilToOldABuffer) {
    TestIOBuf src;
    std::string payload = "prefix,separator,data";
    FillIOBuf(src, payload);
    Buffer<char, 64> target;
    ContainerAppender<Buffer<char, 64>> receiver(target);
    auto status = Customer::custom_until(src, receiver, ',');
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(std::string(target.data(), target.size()), "prefix");
    // Source should have consumed "prefix," (7 bytes)
    EXPECT_EQ(src.size(), payload.size() - 7);
    EXPECT_EQ(src.flatten(), "separator,data");
}

} // namespace fermat