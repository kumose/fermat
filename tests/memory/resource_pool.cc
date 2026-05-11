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
#include <thread>
#include <vector>
#include <fermat/memory/resource_pool.h>  // Assume the header file is named resource_pool.h

namespace fermat {

/// Define static members of ThreadShard (must be defined in one translation unit).
std::atomic<int32_t> ThreadShard::g_thread_shard_id{0};

/// Simple test object used for pool tests.
struct TestObject {
    int value;
    TestObject() : value(0) {}
    explicit TestObject(int v) : value(v) {}
    ~TestObject() { value = -1; }
};

/// Concrete pool type with default template parameters for testing.
using TestPool = ResourcePool<TestObject, 8, 64, 1024, 64>;

class ResourcePoolTest : public ::testing::Test {
protected:
    void TearDown() override {
        /// No explicit cleanup needed; the pool is a singleton, but it does not leak.
        /// Individual tests should release all allocated IDs to avoid cross-test interference.
    }
};

/// Test basic allocation, construction, find, and release.
TEST_F(ResourcePoolTest, BasicAllocateAndFree) {
    int64_t rid;
    TestObject* obj = TestPool::get_uninitialize(rid);
    ASSERT_NE(obj, nullptr);
    EXPECT_NE(rid, 0);

    /// Construct the object using placement new.
    new (obj) TestObject(42);
    EXPECT_EQ(obj->value, 42);

    /// find() should locate the object with correct version.
    TestObject* found = TestPool::find(rid);
    EXPECT_EQ(found, obj);

    /// Release the object (calls destructor + put_raw).
    TestPool::put(rid);

    /// After release, find() must return nullptr.
    found = TestPool::find(rid);
    EXPECT_EQ(found, nullptr);
}

/// Test allocation with construction arguments.
TEST_F(ResourcePoolTest, ConstructWithArguments) {
    int64_t rid;
    TestObject* obj = TestPool::get(rid, 100);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 100);

    /// Release using put_raw directly (skip destruction, but this is fine for trivial types).
    TestPool::put_raw(rid);

    /// ID should now be invalid.
    EXPECT_EQ(TestPool::find(rid), nullptr);
}

/// Test put_raw releases slot back to free list, making it reusable.
TEST_F(ResourcePoolTest, PutRawRecyclesSlot) {
    int64_t rid1, rid2;
    TestObject* obj1 = TestPool::get(rid1, 10);
    ASSERT_NE(obj1, nullptr);

    /// Release without destructor (put_raw). Slot becomes free.
    TestPool::put_raw(rid1);

    /// Next allocation may (or may not) reuse the same physical slot, but it must succeed.
    TestObject* obj2 = TestPool::get(rid2, 20);
    ASSERT_NE(obj2, nullptr);
    EXPECT_NE(rid1, rid2);  // version differs, so full ID is different.

    TestPool::put(rid2);
}

/// Test that stale IDs (wrong version) are rejected by put_raw and find.
TEST_F(ResourcePoolTest, StaleIdIgnored) {
    int64_t rid;
    TestObject* obj = TestPool::get(rid, 5);
    ASSERT_NE(obj, nullptr);

    /// Store a copy of the ID before releasing.
    int64_t stale_rid = rid;

    /// Release the object.
    TestPool::put(rid);

    /// Now stale_rid has old version. put_raw must ignore it.
    TestPool::put_raw(stale_rid);  // Should do nothing (no crash, no double free).

    /// find with stale ID must return nullptr.
    EXPECT_EQ(TestPool::find(stale_rid), nullptr);

    /// Pool should still be functional: allocate again.
    int64_t new_rid;
    TestObject* new_obj = TestPool::get(new_rid, 10);
    ASSERT_NE(new_obj, nullptr);
    TestPool::put(new_rid);
}

/// Test concurrent allocations and releases from multiple threads.
TEST_F(ResourcePoolTest, ConcurrentAccess) {
    constexpr int kNumThreads = 16;
    constexpr int kOpsPerThread = 200;

    std::vector<std::thread> threads;
    std::atomic<bool> start{false};

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&start] {
            while (!start.load()) { std::this_thread::yield(); }
            for (int i = 0; i < kOpsPerThread; ++i) {
                int64_t rid;
                TestObject* obj = TestPool::get(rid, i);
                if (obj != nullptr) {
                    EXPECT_EQ(obj->value, i);
                    TestPool::put(rid);
                } else {
                    // Allocation may rarely fail if pool is exhausted, but with defaults it should not.
                    // Just reattempt.
                    --i;
                }
            }
        });
    }

    start = true;
    for (auto& th : threads) {
        th.join();
    }
}

/// Test that find returns nullptr for invalid ID (0 or out-of-range shard).
TEST_F(ResourcePoolTest, FindInvalidId) {
    EXPECT_EQ(TestPool::find(0), nullptr);

    /// Create a valid ID, then corrupt shard_id to 255.
    int64_t rid;
    TestPool::get(rid, 99);
    ResourceId bad_rid(rid);
    bad_rid.set_shard_id(255);   // shard 255 may not exist (only 0..255 are valid, but out-of-range shard?)
    /// The shards_ array size is 256, index 255 is valid, but likely has no blocks. So find returns nullptr.
    EXPECT_EQ(TestPool::find(bad_rid.encode()), nullptr);

    /// Clean up the original object.
    TestPool::put(rid);
}

/// Test that create_block is called automatically when free list becomes empty.
TEST_F(ResourcePoolTest, AutoExpandBlock) {
    /// Allocate enough objects to force creation of a second block.
    /// SlotSize = 64, BlockSize = 8, so maximum objects = 512.
    std::vector<int64_t> rids;
    for (int i = 0; i < 130; ++i) {   // More than one block (64+64)
        int64_t rid;
        TestObject* obj = TestPool::get(rid, i);
        ASSERT_NE(obj, nullptr);
        rids.push_back(rid);
    }

    /// All objects should be valid.
    for (size_t i = 0; i < rids.size(); ++i) {
        TestObject* obj = TestPool::find(rids[i]);
        ASSERT_NE(obj, nullptr);
        EXPECT_EQ(obj->value, static_cast<int>(i));
    }

    /// Release them.
    for (int64_t rid : rids) {
        TestPool::put(rid);
    }
}

}  // namespace fermat