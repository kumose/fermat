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

#include <fermat/container/heap.h>
#include <fermat/container/priority_queue.h>
#include <fermat/container/vector.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <random>
#include <vector>

namespace {

using fermat::change_heap;
using fermat::is_heap;
using fermat::is_heap_until;
using fermat::make_heap;
using fermat::pop_heap;
using fermat::PriorityQueue;
using fermat::push_heap;
using fermat::remove_heap;
using fermat::sort_heap;
using fermat::Vector;

using IntPriorityQueue = PriorityQueue<int, 0>;

void VerifyHeaps(uint32_t *pArray2, uint32_t *pArray3, uint32_t nArraySize) {
    ASSERT_TRUE(is_heap(pArray2, pArray2 + nArraySize));
    ASSERT_TRUE(is_heap(pArray3, pArray3 + nArraySize));

    std::unique_ptr<uint32_t[]> pArray2Copy(new uint32_t[nArraySize]);
    std::unique_ptr<uint32_t[]> pArray3Copy(new uint32_t[nArraySize]);

    std::memcpy(pArray2Copy.get(), pArray2, sizeof(uint32_t) * nArraySize);
    std::memcpy(pArray3Copy.get(), pArray3, sizeof(uint32_t) * nArraySize);

    for (uint32_t i = 0; i < nArraySize; ++i) {
        EXPECT_EQ(pArray2Copy[0], pArray3Copy[0]);
        std::pop_heap(pArray2Copy.get(), pArray2Copy.get() + nArraySize - i);
        pop_heap(pArray3Copy.get(), pArray3Copy.get() + nArraySize - i);
    }
}

TEST(HeapTest, RandomizedVsStd) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int32_t> sizeDist(2, 1000);
    std::uniform_int_distribution<int32_t> valueDist(0, 500);

    for (int i = 0; i < 25; ++i) {
        const uint32_t nArraySizeInitial = static_cast<uint32_t>(sizeDist(rng));
        uint32_t nArraySize = nArraySizeInitial;

        std::unique_ptr<uint32_t[]> pArray1(new uint32_t[nArraySize + 1]);
        std::unique_ptr<uint32_t[]> pArray2(new uint32_t[nArraySize + 1]);
        std::unique_ptr<uint32_t[]> pArray3(new uint32_t[nArraySize + 1]);

        for (uint32_t j = 0; j < nArraySize; ++j) {
            const uint32_t v = static_cast<uint32_t>(valueDist(rng));
            pArray1[j] = pArray2[j] = pArray3[j] = v;
        }

        std::make_heap(pArray2.get(), pArray2.get() + nArraySize);
        make_heap(pArray3.get(), pArray3.get() + nArraySize);
        VerifyHeaps(pArray2.get(), pArray3.get(), nArraySize);

        {
            pArray3[nArraySize] = 501;
            uint32_t *pUntil = is_heap_until(pArray3.get(), pArray3.get() + nArraySize + 1);
            EXPECT_EQ(pUntil, pArray3.get() + nArraySize);
        }

        const int popCount = static_cast<int>(std::min<uint32_t>(200, nArraySize));
        for (int k = 0; k < popCount; ++k, --nArraySize) {
            std::pop_heap(pArray2.get(), pArray2.get() + nArraySize);
            pArray2[nArraySize - 1] = 0xffffffffu;

            pop_heap(pArray3.get(), pArray3.get() + nArraySize);
            pArray3[nArraySize - 1] = 0xffffffffu;

            VerifyHeaps(pArray2.get(), pArray3.get(), nArraySize - 1);
        }

        for (int m = 0; m < popCount; ++m, ++nArraySize) {
            const uint32_t n = static_cast<uint32_t>(valueDist(rng));

            pArray2[nArraySize] = n;
            std::push_heap(pArray2.get(), pArray2.get() + nArraySize + 1);

            pArray3[nArraySize] = n;
            push_heap(pArray3.get(), pArray3.get() + nArraySize + 1);

            VerifyHeaps(pArray2.get(), pArray3.get(), nArraySize + 1);
        }

        const uint32_t originalSize = nArraySize;
        for (int e = 0; e < popCount; ++e, --nArraySize) {
            const uint32_t position = static_cast<uint32_t>(
                    std::uniform_int_distribution<uint32_t>(0, nArraySize - 1)(rng));

            remove_heap(pArray2.get(), nArraySize, position);
            pArray2[nArraySize - 1] = 0xffffffffu;

            remove_heap(pArray3.get(), nArraySize, position);
            pArray3[nArraySize - 1] = 0xffffffffu;

            if (nArraySize > 1) {
                EXPECT_EQ(is_heap_until(pArray2.get(), pArray2.get() + nArraySize), pArray2.get() + nArraySize - 1);
                EXPECT_EQ(is_heap_until(pArray3.get(), pArray3.get() + nArraySize), pArray3.get() + nArraySize - 1);
            }
        }

        for (int m = 0; m < popCount; ++m, ++nArraySize) {
            const uint32_t n = static_cast<uint32_t>(valueDist(rng));

            pArray2[nArraySize] = n;
            std::push_heap(pArray2.get(), pArray2.get() + nArraySize + 1);

            pArray3[nArraySize] = n;
            push_heap(pArray3.get(), pArray3.get() + nArraySize + 1);
        }

        EXPECT_EQ(nArraySize, originalSize);
        EXPECT_EQ(is_heap_until(pArray2.get(), pArray2.get() + nArraySize), pArray2.get() + nArraySize);
        EXPECT_EQ(is_heap_until(pArray3.get(), pArray3.get() + nArraySize), pArray3.get() + nArraySize);

        for (int r = 0; r < popCount; ++r) {
            uint32_t position = static_cast<uint32_t>(
                    std::uniform_int_distribution<uint32_t>(0, nArraySize - 1)(rng));
            const uint32_t newValue = static_cast<uint32_t>(valueDist(rng));

            if (std::uniform_int_distribution<int>(0, 4)(rng) == 0) {
                position = 0;
            }
            if (std::uniform_int_distribution<int>(0, 4)(rng) != 0) {
                pArray2[position] = pArray3[position] = newValue;
            }

            change_heap(pArray2.get(), nArraySize, position);
            change_heap(pArray3.get(), nArraySize, position);

            EXPECT_TRUE(is_heap(pArray2.get(), pArray2.get() + nArraySize));
            EXPECT_TRUE(is_heap(pArray3.get(), pArray3.get() + nArraySize));
        }

        std::sort_heap(pArray2.get(), pArray2.get() + nArraySize);
        sort_heap(pArray3.get(), pArray3.get() + nArraySize);

        for (uint32_t q = 1; q < nArraySize; ++q) {
            EXPECT_LE(pArray2[q - 1], pArray2[q]);
            EXPECT_LE(pArray3[q - 1], pArray3[q]);
        }
    }
}

TEST(HeapTest, VectorOperations) {
    Vector<int> heap;
    for (int i = 0; i < 16; ++i) {
        heap.push_back(i);
    }

    make_heap(heap.begin(), heap.end());
    EXPECT_TRUE(is_heap(heap.begin(), heap.end()));

    heap.push_back(7);
    push_heap(heap.begin(), heap.end());
    EXPECT_TRUE(is_heap(heap.begin(), heap.end()));

    heap.push_back(7);
    push_heap(heap.begin(), heap.end());
    heap.pop_back();
    EXPECT_TRUE(is_heap(heap.begin(), heap.end()));

    remove_heap(heap.begin(), heap.size(), static_cast<typename Vector<int>::size_type>(4));
    heap.pop_back();
    EXPECT_TRUE(is_heap(heap.begin(), heap.end()));

    sort_heap(heap.begin(), heap.end());
    EXPECT_TRUE(std::is_sorted(heap.begin(), heap.end()));
}

TEST(PriorityQueueTest, PushPopOrder) {
    IntPriorityQueue pq;
    const int values[] = {3, 1, 4, 1, 5, 9, 2, 6};
    for (int v : values) {
        pq.push(v);
    }
    EXPECT_TRUE(pq.validate());
    EXPECT_EQ(pq.size(), 8u);

    std::vector<int> popped;
    while (!pq.empty()) {
        popped.push_back(pq.top());
        pq.pop();
    }

    EXPECT_TRUE(std::is_sorted(popped.begin(), popped.end(), std::greater<int>()));
    EXPECT_EQ(popped.front(), 9);
    EXPECT_EQ(popped.back(), 1);
}

TEST(PriorityQueueTest, InitializerList) {
    IntPriorityQueue pq({3, 1, 4, 1, 5});
    EXPECT_TRUE(pq.validate());
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
    EXPECT_EQ(pq.top(), 4);
}

TEST(PriorityQueueTest, Remove) {
    IntPriorityQueue pq({10, 20, 30, 40, 50});
    EXPECT_TRUE(pq.validate());

    pq.remove(2);
    EXPECT_TRUE(pq.validate());
    EXPECT_EQ(pq.size(), 4u);

    std::vector<int> values;
    while (!pq.empty()) {
        values.push_back(pq.top());
        pq.pop();
    }
    EXPECT_EQ(values, (std::vector<int>{50, 40, 20, 10}));
}

TEST(PriorityQueueTest, Change) {
    IntPriorityQueue pq({1, 3, 5, 7, 9});
    EXPECT_TRUE(pq.validate());

    auto &container = pq.get_container();
    const auto it = container.begin() + 2;
    const size_t index = static_cast<size_t>(it - container.begin());
    *it = 100;
    pq.change(index);

    EXPECT_TRUE(pq.validate());
    EXPECT_EQ(pq.top(), 100);

    pq.pop();
    EXPECT_EQ(pq.top(), 9);
}

TEST(PriorityQueueTest, EmplaceAndPopWithValue) {
    IntPriorityQueue pq;
    pq.emplace(42);
    pq.emplace(17);
    EXPECT_TRUE(pq.validate());

    int value = 0;
    pq.pop(value);
    EXPECT_EQ(value, 42);
    EXPECT_EQ(pq.size(), 1u);
    EXPECT_TRUE(pq.validate());
}

} // namespace
