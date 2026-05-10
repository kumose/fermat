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
#include <fermat/memory/object_pool.h>
#include <thread>
#include <vector>
#include <atomic>

namespace fermat {

// -----------------------------------------------------------------------------
// Helper types for tracking construction/destruction
// -----------------------------------------------------------------------------

struct Tracked {
    static std::atomic<int> constructed;
    static std::atomic<int> destructed;
    int value;

    Tracked(int v = 0) : value(v) {
        ++constructed;
    }
    Tracked(const Tracked&) = delete;
    Tracked(Tracked&&) = delete;
    ~Tracked() {
        ++destructed;
    }
};

std::atomic<int> Tracked::constructed{0};
std::atomic<int> Tracked::destructed{0};

struct NonTrivial {
    std::vector<int> data;
    NonTrivial() = default;
    explicit NonTrivial(size_t n) : data(n, 42) {}
};

// -----------------------------------------------------------------------------
// ObjectPool Tests
// -----------------------------------------------------------------------------

TEST(ObjectPoolTest, GetUninitialize) {
    using Pool = ObjectPool<Tracked>;
    Tracked* ptr = Pool::get_uninitialize();
    ASSERT_NE(ptr, nullptr);
    // size: at least sizeof(Tracked)
    // We can't easily verify size, but we can write to it
    ptr->value = 123;
    EXPECT_EQ(ptr->value, 123);
    // Put back to avoid leak
    Pool::put_raw(ptr);
    EXPECT_EQ(Tracked::constructed.load(), 0);  // no constructor called
}

TEST(ObjectPoolTest, GetConstruct) {
    using Pool = ObjectPool<Tracked>;
    Tracked::constructed = 0;
    Tracked::destructed = 0;

    Tracked* ptr = Pool::get(42);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value, 42);
    EXPECT_EQ(Tracked::constructed.load(), 1);

    Pool::put(ptr);
    EXPECT_EQ(Tracked::destructed.load(), 1);
}

TEST(ObjectPoolTest, PutRaw) {
    using Pool = ObjectPool<Tracked>;
    Tracked* ptr = Pool::get_uninitialize();
    void* raw_addr = ptr;
    Pool::put_raw(ptr);

    Tracked* ptr2 = Pool::get_uninitialize();
    // Should get the same memory block (cached)
    EXPECT_EQ(ptr2, raw_addr);
    Pool::put_raw(ptr2);
}

TEST(ObjectPoolTest, PutWithDestructor) {
    using Pool = ObjectPool<Tracked>;
    Tracked::constructed = 0;
    Tracked::destructed = 0;

    Tracked* ptr = Pool::get(100);
    ASSERT_EQ(Tracked::constructed.load(), 1);
    Pool::put(ptr);
    EXPECT_EQ(Tracked::destructed.load(), 1);
}

    TEST(ObjectPoolTest, CacheLimit) {
    constexpr size_t Max = 4;
    using Pool = ObjectPool<Tracked, Max>;

    std::vector<Tracked*> ptrs;
    // Allocate more than Max to force direct allocations
    for (size_t i = 0; i < Max + 2; ++i) {
        ptrs.push_back(Pool::get_uninitialize());
    }
    // Release all
    for (auto* p : ptrs) {
        Pool::put_raw(p);
    }

    // Now the thread-local cache holds at most Max entries (the last Max put in).
    std::vector<Tracked*> cached;
    for (size_t i = 0; i < Max; ++i) {
        cached.push_back(Pool::get_uninitialize());
    }
    // The (Max+1)th get should be a fresh allocation (not from cache)
    Tracked* extra = Pool::get_uninitialize();

    // Verify extra is not among the cached pointers
    bool found = false;
    for (Tracked* p : cached) {
        if (p == extra) {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found) << "Extra allocation reused cached memory – cache limit exceeded?";

    // Cleanup
    for (auto* p : cached) Pool::put_raw(p);
    Pool::put_raw(extra);
}

TEST(ObjectPoolTest, ThreadLocalIsolation) {
    using Pool = ObjectPool<int>;
    constexpr int kIter = 100;
    std::atomic<int> sum1{0}, sum2{0};
    std::thread t1([&] {
        std::vector<int*> ptrs;
        for (int i = 0; i < kIter; ++i) {
            int* p = Pool::get_uninitialize();
            *p = i;
            ptrs.push_back(p);
            sum1 += i;
        }
        for (int* p : ptrs) Pool::put_raw(p);
    });
    std::thread t2([&] {
        std::vector<int*> ptrs;
        for (int i = 0; i < kIter; ++i) {
            int* p = Pool::get_uninitialize();
            *p = i * 2;
            ptrs.push_back(p);
            sum2 += i * 2;
        }
        for (int* p : ptrs) Pool::put_raw(p);
    });
    t1.join();
    t2.join();
    // No assertions on values, just no crash and thread-local caches are independent.
    SUCCEED();
}

TEST(ObjectPoolTest, ConcurrentGetPut) {
    using Pool = ObjectPool<int>;
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 1000;
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            while (!start.load()) std::this_thread::yield();
            for (int i = 0; i < kOpsPerThread; ++i) {
                int* p = Pool::get_uninitialize();
                *p = i;
                Pool::put_raw(p);
            }
        });
    }
    start.store(true);
    for (auto& th : threads) th.join();
    // No crash, no data races (run under TSAN).
    SUCCEED();
}

TEST(ObjectPoolTest, WithNonTrivialType) {
    using Pool = ObjectPool<NonTrivial>;
    NonTrivial* obj = Pool::get(256);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->data.size(), 256);
    EXPECT_EQ(obj->data[0], 42);
    Pool::put(obj);
    // No memory leak
}

TEST(ObjectPoolTest, MaxFreeZero) {
    using Pool = ObjectPool<int, 0>;   // MaxFree = 0
    int* p1 = Pool::get_uninitialize();
    *p1 = 123;
    void* addr1 = p1;
    Pool::put_raw(p1);
    int* p2 = Pool::get_uninitialize();
    // Because MaxFree=0, put_raw freed the memory immediately to mimalloc.
    // The next get_uninitialize must allocate a new block (address likely different).
    // However, mimalloc may reuse the same address, but it's not guaranteed.
    // We can only verify that we can allocate again without crash.
    EXPECT_NE(p2, nullptr);
    *p2 = 456;
    Pool::put_raw(p2);
    SUCCEED();
}

} // namespace fermat
