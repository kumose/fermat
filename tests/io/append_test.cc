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
    // Helper to create an IOBuf with different alignment (incompatible for sharing)
    using IncompatibleIOBuf = IOBuf<128, 4096>;

    // ============================================================================
    // IOBuf write operations
    // ============================================================================

    TEST_F(IOBufTest, AppendStringView) {
        TestIOBuf buf;
        std::string_view data = "hello world";
        auto status = buf.append(data);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(buf.size(), data.size());
        EXPECT_EQ(buf.flatten(), data);
    }

    TEST_F(IOBufTest, AppendRawData) {
        TestIOBuf buf;
        const char *raw = "raw data";
        size_t len = strlen(raw);
        auto status = buf.append(const_cast<char *>(raw), len);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(buf.size(), len);
        EXPECT_EQ(buf.flatten(), raw);
    }

    TEST_F(IOBufTest, AppendMove) {
        TestIOBuf src, dst;
        src.append("source");
        dst.append("dest");

        auto status = dst.append(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "destsource");
        EXPECT_EQ(src.size(), 0); // src is empty after move
    }

    TEST_F(IOBufTest, AppendToShare) {
        TestIOBuf src, dst;
        src.append("shared data");
        dst.append("dest");
        src.shrink_immutable();
        auto status = src.append_to(dst);
        ASSERT_TRUE(status.ok()) << status;
        EXPECT_EQ(dst.size(), src.size() + 4); // "dest" length = 5
        EXPECT_EQ(dst.flatten(), "destshared data");

        // After sharing, src's blocks become Immutable
        // Check by trying to write – should allocate a new block
        auto *lease = src.borrow().value_or_die();
        EXPECT_TRUE(lease->capacity() > 0);
        src.commit(lease);
        // No direct way to check refcount, but the test passes if no crash
    }


    TEST_F(IOBufTest, AppendToCopy) {
        IncompatibleIOBuf src; // different alignment → must copy
        TestIOBuf dst;
        src.append("copy this");
        dst.append("dest");

        auto status = src.append_to(dst);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "destcopy this");
        EXPECT_EQ(src.size(), 9); // src unchanged (no sharing)
    }

    TEST_F(IOBufTest, PrependMove) {
        TestIOBuf src, dst;
        src.append("prefix");
        dst.append("suffix");

        auto status = dst.prepend(std::move(src));
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "prefixsuffix");
        EXPECT_EQ(src.size(), 0);
    }

    TEST_F(IOBufTest, PrependToShare) {
        TestIOBuf src, dst;
        src.append("head");
        dst.append("tail");
        src.shrink_immutable();
        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "headtail");
        // src becomes Immutable; can still read but new writes allocate new block
        EXPECT_EQ(src.flatten(), "head");
        auto *lease = src.borrow().value_or_die();
        EXPECT_TRUE(lease->capacity() > 0);
        src.commit(lease);
    }

    TEST_F(IOBufTest, PrependToCopy) {
        IncompatibleIOBuf src; // different alignment → copy
        TestIOBuf dst;
        src.append("pre");
        dst.append("post");

        auto status = src.prepend_to(dst);
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(dst.flatten(), "prepost");
        EXPECT_EQ(src.flatten(), "pre"); // src unchanged
    }
} // namespace fermat
