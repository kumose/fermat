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

    /// Mock fixed container to test FixedContainerTag logic and threshold enforcement.
    class MockFixedBuffer {
    public:
        using value_type = char;
        MockFixedBuffer(size_t cap) : _cap(cap), _data(new char[cap]) {}
        ~MockFixedBuffer() { delete[] _data; }

        char* data() { return _data; }
        size_t size() const { return _size; }
        size_t capacity() const { return _cap; }
        void set_size(size_t s) { _size = s; }

    private:
        char* _data;
        size_t _size{0};
        size_t _cap{0};
    };

    /// -----------------------------------------------------------------------------
    /// Case 1: Physical Asset Lifecycle (Borrow, Commit, Custom)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, PhysicalLifecycle) {
        IOBuf<64, 1024> iob;
        std::string_view msg = "hardcore asset management";

        // 1. Borrowing multiple spans (debt model)
        auto spans = iob.borrow(msg.size(), std::nullopt).value_or_die();
        ASSERT_EQ(spans.size(), 1);
        std::memcpy(spans[0].data(), msg.data(), msg.size());

        // 2. Settlement
        iob.commit(msg.size());
        EXPECT_EQ(iob.size(), msg.size());
        EXPECT_EQ(iob.blocks(), 1);

        // 3. Partial consumption (logical window slide)
        size_t part = 5;
        ASSERT_TRUE(iob.custom(part).ok());
        EXPECT_EQ(iob.size(), msg.size() - part);

        // 4. Exact boundary consumption (remin == v.length)
        // This should trigger BlockStatus::Umount and release_block
        ASSERT_TRUE(iob.custom(iob.size()).ok());
        EXPECT_EQ(iob.size(), 0);
        EXPECT_EQ(iob.blocks(), 1);
    }

    /// -----------------------------------------------------------------------------
    /// Case 2: Archipelago Transfer - Dynamic Island (std::string)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, DynamicIslandTransfer) {
        IOBuf<64, 1024> iob;
        std::string payload(2048, 'x'); // Cross multiple blocks

        // Fill IOBuf
        auto spans = iob.borrow(payload.size(), std::nullopt).value_or_die();
        size_t offset = 0;
        for (auto s : spans) {
            std::memcpy(s.data(), payload.data() + offset, s.size());
            offset += s.size();
        }
        iob.commit(payload.size());

        // Scenario: Move all data to string (Custom = true)
        std::string target;
        auto rs = FlattenCustomer<std::string, true>::push_back(iob, target, payload.size());
        ASSERT_TRUE(rs.ok())<<rs;

        EXPECT_EQ(target, payload);
        EXPECT_EQ(iob.size(), 0);
    }

    /// -----------------------------------------------------------------------------
    /// Case 3: Archipelago Transfer - Fixed Island (Threshold Enforcement)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, FixedIslandEnforcement) {
        IOBuf<64, 1024> iob;
        std::string data = "this data is too large for the target";

        auto spans = iob.borrow(data.size(), std::nullopt).value_or_die();
        std::memcpy(spans[0].data(), data.data(), data.size());
        iob.commit(data.size());

        // Target island with only 10 bytes capacity
        MockFixedBuffer island(10);

        // Should fail fast via TURBO_RETURN_NOT_OK(reserve)
        auto status = FlattenCustomer<MockFixedBuffer, true>::push_back(iob, island, data.size());

        EXPECT_FALSE(status.ok());
        EXPECT_EQ(status.code(), turbo::StatusCode::kOutOfRange);
        // Transactional Integrity: Source assets MUST remain untouched
        EXPECT_EQ(iob.size(), data.size());
    }

    /// -----------------------------------------------------------------------------
    /// Case 4: Zero-Copy Asset Sharing (Compatible Alignments)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, ZeroCopyShare) {
        IOBuf<64, 1024> src;
        IOBuf<64, 1024> dst;
        std::string secret = "zero-copy-magic";

        auto spans = src.borrow(secret.size(), std::nullopt).value_or_die();
        std::memcpy(spans[0].data(), secret.data(), secret.size());
        src.commit(secret.size());

        // Direct asset transfer: shareable because alignment matches
        ASSERT_TRUE(src.append_to(dst).ok());

        EXPECT_EQ(dst.size(), secret.size());

        // Audit physical block: should have multiple owners
        auto* view = dst.peek(0);
        ASSERT_NE(view, nullptr);
        EXPECT_TRUE(view->block->is_shared());

        // Local index should be pushed to end after share
        EXPECT_EQ(src.size(), dst.size());
    }

    /// -----------------------------------------------------------------------------
    /// Case 5: Incompatible Alignment (Automatic Fallback to Copy)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, AlignmentMismatchFallback) {
        IOBuf<64, 1024> src;
        IOBuf<16, 512>  dst; // Different alignment and block size
        std::string data = "alignment mismatch requires copy";

        auto spans = src.borrow(data.size(), std::nullopt).value_or_die();
        std::memcpy(spans[0].data(), data.data(), data.size());
        src.commit(data.size());

        // Should detect incompatibility and fallback to append_copy_to
        ASSERT_TRUE(src.append_to(dst).ok());

        EXPECT_EQ(dst.size(), data.size());

        // Physical check: dst should have its OWN block, not shared with src
        auto* view = dst.peek(0);
        ASSERT_NE(view, nullptr);
        EXPECT_FALSE(view->block->is_shared());
    }

    /// -----------------------------------------------------------------------------
    /// Case 6: The "Stitch" Model (Combine Borrow)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, CombineBorrowStitch) {
        IOBuf<64, 1024> iob;
        size_t large_size = 3000; // > 1024 * 2

        // Request a physically contiguous large block
        auto res = iob.borrow(large_size, static_cast<int>(large_size));
        ASSERT_TRUE(res.ok());

        auto spans = res.value_or_die();
        // Even if the total size is 3000, it should be one contiguous span
        // because we requested 'combine'.
        EXPECT_EQ(spans.size(), 1);
        EXPECT_GE(spans[0].size(), large_size);
    }

    /// -----------------------------------------------------------------------------
    /// Case 7: Double Borrow Prevention (State Machine)
    /// -----------------------------------------------------------------------------
    TEST(IOBufTest, TransactionLock) {
        IOBuf<64, 1024> iob;
        auto res1 = iob.borrow(100, std::nullopt);
        ASSERT_TRUE(res1.ok());

        // Concurrent borrow on the same iobuf must fail
        auto res2 = iob.borrow(100, std::nullopt);
        EXPECT_FALSE(res2.ok());
        EXPECT_EQ(res2.status().code(), turbo::StatusCode::kUnavailable);

        iob.commit(100);
        // After commit, it should be available again
        EXPECT_TRUE(iob.borrow(100, std::nullopt).ok());
    }


    /// @brief Test suite for fermat aligned stl containers.
    ///
    /// Focuses on verifying the physical alignment invariant and
    /// the compatibility with the ContainerTraits protocol.
    class AlignedContainerTest : public ::testing::Test {
    protected:
        static constexpr size_t kAlign64 = 64;
        static constexpr size_t kAlign128 = 128;
    };

    /// @brief Verifies that AlignedString maintains memory alignment across allocations.
    TEST_F(AlignedContainerTest, StringAlignmentInvariant) {
        AlignedString<kAlign64> s;

        /// 1. Test small allocation (Potential SSO area).
        s = "short_asset";

        /// 2. Force heap allocation via large resize to trigger AlignedAllocator.
        s.resize(4096);
        auto addr = reinterpret_cast<uintptr_t>(s.data());
        EXPECT_EQ(addr % kAlign64, 0) << "AlignedString failed 64-byte alignment";

        /// 3. Verify alignment persists after multiple append operations (reallocations).
        for (int i = 0; i < 10; ++i) {
            s.append(2048, 'x');
            EXPECT_EQ(reinterpret_cast<uintptr_t>(s.data()) % kAlign64, 0);
        }
    }

    /// @brief Verifies that AlignedVector respects high-alignment requirements.
    TEST_F(AlignedContainerTest, VectorAlignmentInvariant) {
        AlignedVector<double, kAlign128> v;

        v.resize(100);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(v.data()) % kAlign128, 0);

        v.reserve(5000);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(v.data()) % kAlign128, 0);
    }

    /// @brief Verifies that Aligned containers are correctly identified as Dynamic Islands.
    TEST_F(AlignedContainerTest, VectorCompatibility) {
        {
            using Target = AlignedVector<char,kAlign64>;

            /// 1. Compile-time check for DynamicContainerTag.
            static_assert(ContainerTraits<Target>::is_dynamic_container,
                          "AlignedString must be recognized as dynamic");

            static_assert(std::is_same_v<typename ContainerTraits<Target>::container_tag,
                          VectorContainerTag>,
                          "AlignedString must carry DynamicContainerTag");

            /// 2. Functional check: Transfer data from IOBuf to AlignedString.
            IOBuf<kAlign64, 1024> iob;
            std::string payload(500, 'A');
            KLOG(INFO)<<1;
            Lease spans = iob.borrow(payload.size(), std::nullopt).value_or_die();
            std::memcpy(spans[0].data(), payload.data(), payload.size());
            iob.commit(payload.size());

            std::unique_ptr<Target> destination = std::make_unique<Target>();
            /// Transfer via FlattenCustomer (Custom = true)
            //auto status = FlattenCustomer<Target, false>::push_back(iob, *destination, payload.size());

          //  ASSERT_TRUE(status.ok()) << status;
            KLOG(INFO)<<1.1;
            EXPECT_EQ(destination->size(), payload.size());
            KLOG(INFO)<<1.2;
            EXPECT_EQ(iob.size(), 0); // Source drained
            KLOG(INFO)<<2;
            destination.reset();
            KLOG(INFO)<<3;
        }
        KLOG(INFO)<<4;
    }
    /*
    /// @brief Verifies that Aligned containers are correctly identified as Dynamic Islands.
    TEST_F(AlignedContainerTest, StdStingCompatibility) {
        {
            using Target = AlignedString<kAlign64>;

            /// 1. Compile-time check for DynamicContainerTag.
            static_assert(ContainerTraits<Target>::is_dynamic_container,
                          "AlignedString must be recognized as dynamic");

            static_assert(std::is_same_v<typename ContainerTraits<Target>::container_tag,
                          StringContainerTag>,
                          "AlignedString must carry DynamicContainerTag");

            /// 2. Functional check: Transfer data from IOBuf to AlignedString.
            IOBuf<kAlign64, 1024> iob;
            std::string payload(500, 'A');
            KLOG(INFO)<<1;
            auto spans = iob.borrow(payload.size(), std::nullopt).value_or_die();
            std::memcpy(spans.data(), payload.data(), payload.size());
            iob.commit(payload.size());

            std::unique_ptr<Target> destination = std::make_unique<Target>();
            /// Transfer via FlattenCustomer (Custom = true)
            auto status = FlattenCustomer<Target, false>::push_back(iob, *destination, payload.size());
            KLOG(INFO)<<2;
            ASSERT_TRUE(status.ok()) << status;
            EXPECT_EQ(destination->size(), payload.size());
            EXPECT_EQ(iob.size(), 0); // Source drained
            destination.reset();
            KLOG(INFO)<<3;
        }
        KLOG(INFO)<<4;
    }
    */
    /// @brief Verifies that AlignedBasicString handles custom character types.
    TEST_F(AlignedContainerTest, AlignedBasicStringSupport) {
        using WString = AlignedBasicString<wchar_t, kAlign64>;
        WString ws;

        ws.resize(1024);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ws.data()) % kAlign64, 0);
    }
} // namespace fermat