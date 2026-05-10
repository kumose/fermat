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

#include <gtest/gtest.h>
#include <fermat/memory/resource_pool.h>
#include <thread>
#include <vector>
#include <atomic>

namespace fermat {
    // Test type for tracking construction/destruction
    struct TrackedResource {
        static std::atomic<int> constructed;
        static std::atomic<int> destructed;
        int value;

        TrackedResource(int v = 0) : value(v) { ++constructed; }

        TrackedResource(const TrackedResource &) = delete;

        TrackedResource(TrackedResource &&) = delete;

        ~TrackedResource() { ++destructed; }
    };

    std::atomic<int> TrackedResource::constructed{0};
    std::atomic<int> TrackedResource::destructed{0};

    // Use small config for testing: 2 blocks, 4 slots per block, small TLS cache
    using TestPool = ResourcePool<TrackedResource, 2, 4, 4, 2>;

    // Helper to wait for a flag
    static void WaitFor(std::atomic<bool> &flag) {
        while (!flag.load()) std::this_thread::yield();
    }

    // -----------------------------------------------------------------------------
    // ResourceId utility tests
    // -----------------------------------------------------------------------------
    TEST(ResourceIdTest, DefaultConstruct) {
        ResourceId rid;
        EXPECT_EQ(rid.encode(), ResourceId::kInvalidId);
        EXPECT_EQ(rid.block_id(), 0);
        EXPECT_EQ(rid.slot_id(), 0);
        EXPECT_EQ(rid.version(), 0);
        EXPECT_EQ(rid.user_space(), 0);
    }

    TEST(ResourceIdTest, ConstructFromValue) {
        uint64_t value = 0x1234'5678'9ABC'DEF0ULL;
        ResourceId rid(value);
        EXPECT_EQ(rid.encode(), value);
        // Decode manually (should match the stored fields)
        EXPECT_EQ(rid.block_id(), static_cast<uint16_t>(value >> 0));
        EXPECT_EQ(rid.slot_id(), static_cast<uint16_t>(value >> 16));
        EXPECT_EQ(rid.version(), static_cast<uint16_t>(value >> 32));
        EXPECT_EQ(rid.user_space(), static_cast<uint16_t>(value >> 48));
    }

    TEST(ResourceIdTest, SettersAndGetters) {
        ResourceId rid;
        rid.set_block_id(0x11);
        rid.set_slot_id(0x22);
        rid.set_version(0x33);
        rid.set_user_space(0x44);
        EXPECT_EQ(rid.block_id(), 0x11);
        EXPECT_EQ(rid.slot_id(), 0x22);
        EXPECT_EQ(rid.version(), 0x33);
        EXPECT_EQ(rid.user_space(), 0x44);
        // Encode and decode back
        uint64_t encoded = rid.encode();
        ResourceId rid2(encoded);
        EXPECT_EQ(rid2.block_id(), 0x11);
        EXPECT_EQ(rid2.slot_id(), 0x22);
        EXPECT_EQ(rid2.version(), 0x33);
        EXPECT_EQ(rid2.user_space(), 0x44);
    }

    TEST(ResourceIdTest, NextVersion) {
        ResourceId rid;
        rid.set_version(0);
        rid.next_version();
        EXPECT_EQ(rid.version(), 1);
        rid.next_version();
        EXPECT_EQ(rid.version(), 2);
    }

    // -----------------------------------------------------------------------------
    // ResourcePool basic operations
    // -----------------------------------------------------------------------------
    TEST(ResourcePoolTest, GetUninitializeAndPutRaw) {
        int64_t id1;
        TrackedResource *p1 = TestPool::get_uninitialize(id1);
        ASSERT_NE(p1, nullptr);
        EXPECT_NE(id1, 0);

        int64_t id2;
        TrackedResource *p2 = TestPool::get_uninitialize(id2);
        ASSERT_NE(p2, nullptr);
        EXPECT_NE(id2, 0);

        // Should be different objects (different IDs)
        EXPECT_NE(p1, p2);

        // Release both
        TestPool::put_raw(id1);
        TestPool::put_raw(id2);
    }

    TEST(ResourcePoolTest, GetConstructAndPut) {
        TrackedResource::constructed = 0;
        TrackedResource::destructed = 0;

        int64_t id;
        TrackedResource *ptr = TestPool::get(id, 42);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ(TrackedResource::constructed.load(), 1);

        TestPool::put(id);
        EXPECT_EQ(TrackedResource::destructed.load(), 1);
    }

    TEST(ResourcePoolTest, FindWorks) {
        int64_t id;
        TrackedResource *ptr = TestPool::get_uninitialize(id);
        ASSERT_NE(ptr, nullptr);
        TrackedResource *found = TestPool::find(id);
        EXPECT_EQ(found, ptr);
        TestPool::put_raw(id);
    }

    TEST(ResourcePoolTest, FindInvalidId) {
        TrackedResource *found = TestPool::find(0xFFFFFFFFFFFFFFFF);
        EXPECT_EQ(found, nullptr);
    }

    TEST(ResourcePoolTest, PutInvalidId) {
        // Should not crash
        TestPool::put_raw(0x1234);
        TestPool::put(0x5678);
    }

    // -----------------------------------------------------------------------------
    // TLS cache and reuse
    // -----------------------------------------------------------------------------
    TEST(ResourcePoolTest, CacheReuse) {
        int64_t id1, id2;
        TrackedResource *p1 = TestPool::get_uninitialize(id1);
        TestPool::put_raw(id1);
        TrackedResource *p2 = TestPool::get_uninitialize(id2);
        // Same physical slot but version increased
        EXPECT_EQ(p2, p1);
        EXPECT_NE(id2, id1); // version changed
        ResourceId rid1(id1), rid2(id2);
        EXPECT_EQ(rid1.block_id(), rid2.block_id());
        EXPECT_EQ(rid1.slot_id(), rid2.slot_id());
        EXPECT_EQ(rid2.version(), rid1.version() + 2);
        TestPool::put_raw(id2);
    }

    TEST(ResourcePoolTest, CacheLimit) {
        // TLS cache size = 4, Batch = 2
        std::vector<int64_t> ids;
        for (int i = 0; i < 8; ++i) {
            int64_t id;
            TrackedResource *p = TestPool::get_uninitialize(id);
            ASSERT_NE(p, nullptr);
            KLOG(INFO) << ResourceId(id).to_string();
            ids.push_back(id);
        }
        for (int64_t id: ids) {
            auto ptr = TestPool::find(id);
            ASSERT_TRUE(ptr != nullptr);
            TestPool::put_raw(id);
            KLOG(INFO) << ResourceId(id).to_string();
        }

        // Allocate again – should succeed and reuse cached slots
        std::vector<int64_t> ids2;
        for (int i = 0; i < 8; ++i) {
            int64_t id;
            TrackedResource *p = TestPool::get_uninitialize(id);
            ASSERT_NE(p, nullptr);
            ids2.push_back(id);
        }
        for (int64_t id: ids2) TestPool::put_raw(id);
        // No crash, and memory reclaimed
    }

    // -----------------------------------------------------------------------------
    // Pool capacity (BlockSize * SlotSize)
    // -----------------------------------------------------------------------------
    TEST(ResourcePoolTest, CapacityExhaustion) {
        constexpr size_t TotalSlots = 2 * 4; // BlockSize=2, SlotSize=4
        std::vector<int64_t> ids;
        for (size_t i = 0; i < TotalSlots; ++i) {
            int64_t id;
            TrackedResource *p = TestPool::get_uninitialize(id);
            ASSERT_NE(p, nullptr) << "Failed at i=" << i;
            ids.push_back(id);
        }
        // Next allocation should fail
        int64_t extra_id;
        TrackedResource *extra = TestPool::get_uninitialize(extra_id);
        EXPECT_EQ(extra, nullptr);
        // Release all and reacquire to verify pool empty state works
        for (int64_t id: ids) TestPool::put_raw(id);
        int64_t new_id;
        TrackedResource *pnew = TestPool::get_uninitialize(new_id);
        EXPECT_NE(pnew, nullptr);
        TestPool::put_raw(new_id);
    }

    // -----------------------------------------------------------------------------
    // Concurrency tests
    // -----------------------------------------------------------------------------
    TEST(ResourcePoolTest, ThreadLocalIsolation) {
        std::atomic<bool> start{false};
        std::vector<int64_t> ids1, ids2;
        std::thread t1([&] {
            WaitFor(start);
            for (int i = 0; i < 100; ++i) {
                int64_t id;
                TestPool::get_uninitialize(id);
                ids1.push_back(id);
            }
            for (int64_t id: ids1) TestPool::put_raw(id);
        });
        std::thread t2([&] {
            WaitFor(start);
            for (int i = 0; i < 100; ++i) {
                int64_t id;
                TestPool::get_uninitialize(id);
                ids2.push_back(id);
            }
            for (int64_t id: ids2) TestPool::put_raw(id);
        });
        start = true;
        t1.join();
        t2.join();
        // No crash, each thread uses its own TLS cache
        SUCCEED();
    }

    TEST(ResourcePoolTest, ConcurrentAllocateFree) {
        constexpr int kThreads = 8;
        constexpr int kOpsPerThread = 500;
        std::atomic<bool> start{false};
        std::vector<std::thread> threads;
        for (int i = 0; i < kThreads; ++i) {
            threads.emplace_back([&] {
                WaitFor(start);
                for (int j = 0; j < kOpsPerThread; ++j) {
                    int64_t id;
                    TrackedResource *p = TestPool::get_uninitialize(id);
                    if (p) {
                        p->value = j;
                        TestPool::put_raw(id);
                    }
                }
            });
        }
        start = true;
        for (auto &th: threads) th.join();
        // No data races (run under TSAN for verification)
        SUCCEED();
    }

    // -----------------------------------------------------------------------------
    // Non‑trivial type test
    // -----------------------------------------------------------------------------
    struct NonTrivial {
        std::vector<int> data;

        NonTrivial(size_t n = 0) : data(n, 42) {
        }
    };

    using NonTrivialPool = ResourcePool<NonTrivial, 2, 4, 4, 2>;

    TEST(ResourcePoolTest, NonTrivialType) {
        int64_t id;
        NonTrivial *obj = NonTrivialPool::get(id, 100);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(obj->data.size(), 100);
        EXPECT_EQ(obj->data[0], 42);
        NonTrivialPool::put(id);
    }
} // namespace fermat
