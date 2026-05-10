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
    // Resource lifetime and thread safety (optional)
    // ============================================================================

    // Verify that reference counts increase when blocks are shared and decrease
    // when the shared copies are destroyed.
    TEST_F(IOBufTest, BlockReferenceCount) {
        TestIOBuf src;
        src.append("shared data");
        const auto *view = src.readable_peek(0);
        ASSERT_NE(view, nullptr);
        auto *block = view->block;
        uint32_t initial_ref = block->ref_count.load(); // should be 1

        // Share src's data with dst inside a nested scope
        {
            TestIOBuf dst;
            auto status = src.append_to(dst);
            ASSERT_TRUE(status.ok());
            // After sharing, reference count should increase by 1
            EXPECT_EQ(block->ref_count.load(), initial_ref + 1);
        } // dst destroyed here, reference count drops back to initial_ref

        EXPECT_EQ(block->ref_count.load(), initial_ref);

        // Share again with another dst, check increment
        {
            TestIOBuf dst2;
            auto status = src.append_to(dst2);
            ASSERT_TRUE(status.ok());
            EXPECT_EQ(block->ref_count.load(), initial_ref + 1);
            // Share again with dst3 while dst2 still alive
            {
                TestIOBuf dst3;
                auto status2 = src.append_to(dst3);
                ASSERT_TRUE(status2.ok());
                EXPECT_EQ(block->ref_count.load(), initial_ref + 2);
            } // dst3 destroyed, count back to initial_ref + 1
            EXPECT_EQ(block->ref_count.load(), initial_ref + 1);
        } // dst2 destroyed, count back to initial_ref
        EXPECT_EQ(block->ref_count.load(), initial_ref);
    }

    // Destructor releases all blocks – this test is best run under AddressSanitizer.
    // The test itself does not assert anything but creates and destroys IOBuf objects
    // while ASan detects leaks.
    TEST_F(IOBufTest, DestructorReleasesAllBlocks) {
        // No explicit assertions – memory leaks will be reported by ASan.
        {
            TestIOBuf buf;
            auto rs =buf.append(std::string(10000, 'X'));
            ASSERT_TRUE(rs.ok())<<rs;
            rs = buf.append(std::string(20000, 'Y'));
            ASSERT_TRUE(rs.ok())<<rs;
            auto *lease = buf.borrow(5000, std::nullopt).value_or_die();
            lease->write("leak check", 10);
            buf.commit(lease);
        } // buf destroyed here – all blocks should be freed.
        // ASan will fail if any block leaked.
    }
} // namespace fermat
