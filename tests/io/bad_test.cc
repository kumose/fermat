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
// Error paths and constraints
// ============================================================================

// Double borrow fails while a lease is active.
TEST_F(IOBufTest, DoubleBorrowFails) {
    TestIOBuf buf;
    auto* lease = buf.borrow().value_or_die();
    auto second = buf.borrow();
    EXPECT_FALSE(second.ok());
    EXPECT_EQ(second.status().code(), turbo::StatusCode::kUnavailable);
    buf.commit(lease);
    // After commit, borrow should succeed.
    auto third = buf.borrow();
    EXPECT_TRUE(third.ok());
    buf.commit(third.value_or_die());
}

// Committing a lease that does not belong to this IOBuf triggers a fatal CHECK.
TEST_F(IOBufTest, CommitWithForeignLeaseDies) {
    TestIOBuf buf1, buf2;
    auto* lease = buf1.borrow().value_or_die();
    // Attempt to commit buf1's lease into buf2 – this is a logic error.
    EXPECT_DEATH(buf2.commit(lease), "Lease does not belong to this IOBuf");
    // Clean up to avoid leak in death test (but death test process terminates).
    buf1.commit(lease);  // not reached if death occurs.
}

// Clear() resets written size but does not release capacity; commit after clear works.
TEST_F(IOBufTest, CommitAfterLeaseClearWorks) {
    TestIOBuf buf;
    auto* lease = buf.borrow(100, std::nullopt).value_or_die();
    lease->write("hello", 5);
    lease->clear();               // reset cursor, size becomes 0
    EXPECT_EQ(lease->size(), 0);
    // Commit with zero bytes written – should release lease normally.
    buf.commit(lease);
    EXPECT_EQ(buf.size(), 0);
    // Should be able to borrow again.
    auto* lease2 = buf.borrow().value_or_die();
    EXPECT_TRUE(lease2 != nullptr);
    buf.commit(lease2);
}

// Shrink and shrink_immutable fail when a lease is active.
TEST_F(IOBufTest, ShrinkDuringBorrowingFails) {
    TestIOBuf buf;
    auto* lease = buf.borrow().value_or_die();
    auto shrink_status = buf.shrink();
    EXPECT_FALSE(shrink_status.ok());
    EXPECT_EQ(shrink_status.status().code(), turbo::StatusCode::kUnavailable);
    auto shrink_imm_status = buf.shrink_immutable();
    EXPECT_FALSE(shrink_imm_status.ok());
    EXPECT_EQ(shrink_imm_status.status().code(), turbo::StatusCode::kUnavailable);
    buf.commit(lease);
    // After commit, shrink should work.
    shrink_status = buf.shrink();
    EXPECT_TRUE(shrink_status.ok());
}

// After commit, a new borrow/write does not affect previously committed data.
TEST_F(IOBufTest, WriteAfterCommitSucceeds) {
    TestIOBuf buf;
    // First write
    auto* lease1 = buf.borrow(100, std::nullopt).value_or_die();
    lease1->write("first", 5);
    buf.commit(lease1);
    EXPECT_EQ(buf.flatten(), "first");
    // Second write (new lease)
    auto* lease2 = buf.borrow().value_or_die();
    lease2->write("_second", 7);
    buf.commit(lease2);
    EXPECT_EQ(buf.flatten(), "first_second");
}

}  // namespace fermat
