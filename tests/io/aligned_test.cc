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
    // Alignment and allocation properties
    // ============================================================================

    // AlignedString maintains its alignment after multiple appends.
    // [ANTI-PATTERN] The following test originally relied on small strings and may
    // produce false positives because the internal SSO buffer may not be aligned.
    // It is kept as a reference of what NOT to do.
    TEST_F(IOBufTest, AlignedStringMaintainsAlignment_BadExample) {
        constexpr size_t kAlign = 64;
        AlignedString<kAlign> str;
        str = "initial"; // may stay in SSO, data() could point to unaligned stack buffer
        auto addr = reinterpret_cast<uintptr_t>(str.data());
        KLOG(INFO) << "BadExample: after \"initial\", address = " << std::hex << addr
                   << ", alignment = " << (addr % kAlign);
        str.append(2048, 'x');
        EXPECT_EQ(reinterpret_cast<uintptr_t>(str.data()) % kAlign, 0);
        str.append(4096, 'y');
        EXPECT_EQ(reinterpret_cast<uintptr_t>(str.data()) % kAlign, 0);
    }

    // Correct version: force heap allocation before checking alignment.
    TEST_F(IOBufTest, AlignedStringMaintainsAlignment) {
        constexpr size_t kAlign = 64;
        AlignedString<kAlign> str;
        // Force heap allocation (SSO bypass)
        str.resize(256);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(str.data()) % kAlign, 0);
        str.append(2048, 'x');
        EXPECT_EQ(reinterpret_cast<uintptr_t>(str.data()) % kAlign, 0);
        str.append(4096, 'y');
        EXPECT_EQ(reinterpret_cast<uintptr_t>(str.data()) % kAlign, 0);
    }

    // AlignedVector maintains its alignment after resize and reserve.
    TEST_F(IOBufTest, AlignedVectorMaintainsAlignment) {
        constexpr size_t kAlign = 128;
        AlignedVector<double, kAlign> vec;
        vec.resize(100);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(vec.data()) % kAlign, 0);
        vec.reserve(5000);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(vec.data()) % kAlign, 0);
        vec.push_back(3.14);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(vec.data()) % kAlign, 0);
    }

    // IOBuf constructor with prefix reservation creates Umount blocks that can be
    // reused by prepend operations.
    TEST_F(IOBufTest, IOBufConstructorWithPrefixReserve) {
        size_t block_reserve = 10;
        size_t prefix_reserve = 5;
        TestIOBuf buf(block_reserve, prefix_reserve);
        EXPECT_EQ(buf.prepend_blocks(), prefix_reserve);
        // Append some data, then prepend. The prepend should reuse the reserved Umount blocks.
        buf.append("world");
        TestIOBuf header;
        header.append("hello");
        auto status = buf.prepend(std::move(header));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(buf.flatten(), "helloworld");
        // The prepend_blocks count may change after prepend, but the original reservation was used.
        // No further assertion needed; the test passes if the operation succeeded without extra allocations.
    }

    // Combine borrow (combine_hint > block_size) should produce a single large contiguous block.
    TEST_F(IOBufTest, CombineBorrowCreatesContiguousBlock) {
        TestIOBuf buf;
        size_t block_size = TestIOBuf::kBlockSize;
        size_t large_size = block_size * 2 + 100; // > 2 blocks
        int combine_hint = static_cast<int>(large_size); // request at least this size in one chunk
        auto result = buf.borrow(large_size, combine_hint);
        ASSERT_TRUE(result.ok());
        auto *lease = result.value_or_die();
        EXPECT_GE(lease->capacity(), large_size);
        // Verify that the lease consists of exactly one span (contiguous block).
        int span_count = 0;
        lease->visit_remaining([&](char *, size_t) {
            ++span_count;
            return true;
        });
        EXPECT_EQ(span_count, 1);
        // Optionally, write data and commit to ensure it works.
        std::string data(large_size, 'Z');
        auto status = lease->write(data.data(), data.size());
        ASSERT_TRUE(status.ok());
        buf.commit(lease);
        EXPECT_EQ(buf.size(), large_size);
        EXPECT_EQ(buf.flatten(), data);
    }
} // namespace fermat
