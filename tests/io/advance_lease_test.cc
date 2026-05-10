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

#include <sys/uio.h>

namespace fermat {

using TestIOBuf = IOBuf<64, 4096>;

// ============================================================================
// Zero-copy interface: visit_remaining + advance
// ============================================================================

// 1. Collect all spans from a multi-block lease.
TEST(AdvanceLeaseTest, CollectMultipleSpans) {
    TestIOBuf buf;
    size_t need = 8192; // > block size (4096) to force at least two blocks
    auto* lease = buf.borrow(need, 0).value_or_die();

    std::vector<std::pair<char*, size_t>> segments;
    lease->visit_remaining([&](char* ptr, size_t cap) {
        segments.emplace_back(ptr, cap);
        return true;
    });

    EXPECT_GE(segments.size(), 2);
    size_t total_cap = 0;
    for (auto& seg : segments) total_cap += seg.second;
    EXPECT_EQ(total_cap, lease->capacity());

    buf.commit(lease);
}
    TEST(IOBufTest, LeaseWriteBeyondCapacity) {
    TestIOBuf buf;
    const size_t block_size = TestIOBuf::kBlockSize;  // 4096

    // 1. Fill most of a block, leaving only a few bytes free.
    size_t tail = 16;
    size_t first_write = block_size - tail;
    std::string first(first_write, 'A');
    auto* lease1 = buf.borrow(first_write, std::nullopt).value_or_die();
    auto status = lease1->write(first.data(), first.size());
    ASSERT_TRUE(status.ok());
    buf.commit(lease1);
    EXPECT_EQ(buf.size(), first_write);

    // 2. Borrow again – should get a lease covering the tiny remaining space.
    auto* lease2 = buf.borrow().value_or_die();
    size_t small_cap = lease2->capacity();
    ASSERT_GT(small_cap, 0);
    ASSERT_LE(small_cap, tail);   // may be ≤ tail due to alignment

    // 3. Try to write more than that small capacity.
    std::string overflow(small_cap + 1, 'B');
    status = lease2->write(overflow.data(), overflow.size());
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), turbo::StatusCode::kOutOfRange);

    // 4. No bytes written; lease2 untouched.
    EXPECT_EQ(lease2->size(), 0);
    buf.commit(lease2);   // commit 0, just release
    EXPECT_EQ(buf.size(), first_write);
}

// 3. Fill all spans exactly with a continuous pattern.
TEST(AdvanceLeaseTest, FillAllSpansExactly) {
    TestIOBuf buf;
    size_t need = 8192;
    auto* lease = buf.borrow(need, 0).value_or_die();

    std::vector<size_t> caps;
    lease->visit_remaining([&](char*, size_t cap) {
        caps.push_back(cap);
        return true;
    });

    size_t total = 0;
    for (size_t c : caps) total += c;

    std::string all_data(total, 'Z');
    size_t offset = 0;
    lease->visit_remaining([&](char* ptr, size_t cap) {
        std::memcpy(ptr, all_data.data() + offset, cap);
        offset += cap;
        return true;
    });
    lease->advance(total);
    buf.commit(lease);

    EXPECT_EQ(buf.size(), total);
    EXPECT_EQ(buf.flatten(), all_data);
}

// 4. Stop early when visitor returns false.
TEST(AdvanceLeaseTest, StopEarly) {
    TestIOBuf buf;
    size_t need = 8192;
    auto* lease = buf.borrow(need, 0).value_or_die();

    int call_count = 0;
    lease->visit_remaining([&](char*, size_t) {
        ++call_count;
        return false;   // stop after first block
    });
    EXPECT_EQ(call_count, 1);
    buf.commit(lease);
}

// 5. advance(0) is a no-op.
TEST(AdvanceLeaseTest, AdvanceZeroDoesNothing) {
    TestIOBuf buf;
    auto* lease = buf.borrow().value_or_die();
    size_t cap = lease->capacity();
    lease->advance(0);
    EXPECT_EQ(lease->size(), 0);
    EXPECT_EQ(lease->remaining(), cap);
    buf.commit(lease);
    EXPECT_EQ(buf.size(), 0);
}

// 6. advance beyond capacity: current implementation silently returns,
//    no state change. Caller must guarantee n <= remaining().
TEST(AdvanceLeaseTest, AdvanceBeyondCapacitySilentlyFails) {
    TestIOBuf buf;
    auto* lease = buf.borrow(100, std::nullopt).value_or_die();
    size_t orig_cap = lease->capacity();
    lease->advance(orig_cap + 100);
    EXPECT_EQ(lease->size(), 0);
    EXPECT_EQ(lease->remaining(), orig_cap);
    buf.commit(lease);
    EXPECT_EQ(buf.size(), 0);
}

// 7. Mixing write() and visit_remaining() is not recommended but should not crash.
TEST(AdvanceLeaseTest, MixedWriteAndVisitRemaining) {
    TestIOBuf buf;
    auto* lease = buf.borrow(200, std::nullopt).value_or_die();

    auto status = lease->write("hello", 5);
    ASSERT_TRUE(status.ok());

    bool visited = false;
    lease->visit_remaining([&](char* ptr, size_t cap) {
        visited = true;
        // The remaining capacity should start right after "hello"
        EXPECT_EQ(cap, lease->remaining());
        return false;
    });
    EXPECT_TRUE(visited);
    buf.commit(lease);
    EXPECT_EQ(buf.flatten(), "hello");
}
}  // namespace fermat
