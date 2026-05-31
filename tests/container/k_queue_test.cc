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
#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <queue>
#include <random>
#include <vector>

namespace {

using fermat::MaxKQueue;
using fermat::MinKQueue;

using IntMaxKQueue = MaxKQueue<int>;
using IntMinKQueue = MinKQueue<int>;

template<typename KQueue>
std::vector<int> SortedElements(const KQueue &q) {
    std::vector<int> sorted(q.get_container().begin(), q.get_container().end());
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

bool RefMaxKPush(std::priority_queue<int, std::vector<int>, std::greater<int>> &pq, size_t k, int value) {
    if (k == 0)
        return false;

    if (pq.size() >= k) {
        if (!(value > pq.top()))
            return false;
        pq.pop();
    }

    pq.push(value);
    return true;
}

bool RefMinKPush(std::priority_queue<int, std::vector<int>, std::less<int>> &pq, size_t k, int value) {
    if (k == 0)
        return false;

    if (pq.size() >= k) {
        if (!(value < pq.top()))
            return false;
        pq.pop();
    }

    pq.push(value);
    return true;
}

template<typename Compare>
std::vector<int> SortedMultisetFromRef(std::priority_queue<int, std::vector<int>, Compare> pq) {
    std::vector<int> elements;
    while (!pq.empty()) {
        elements.push_back(pq.top());
        pq.pop();
    }
    std::sort(elements.begin(), elements.end());
    return elements;
}

TEST(MaxKQueueTest, KeepsKLargest) {
    IntMaxKQueue q(3);

    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.capacity(), 3u);
    EXPECT_EQ(q.top(), 1);

    q.push(4);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(SortedElements(q), (std::vector<int>{2, 3, 4}));
    EXPECT_EQ(q.top(), 2);
}

TEST(MaxKQueueTest, RejectsSmallerWhenFull) {
    IntMaxKQueue q(3);
    for (int v : {10, 20, 30})
        q.push(v);

    q.push(5);
    EXPECT_EQ(SortedElements(q), (std::vector<int>{10, 20, 30}));
    EXPECT_EQ(q.top(), 10);
}

TEST(MaxKQueueTest, ConstAndMovePushMatchReference) {
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, 100);

    for (int trial = 0; trial < 50; ++trial) {
        IntMaxKQueue q_const(5);
        IntMaxKQueue q_move(5);
        std::priority_queue<int, std::vector<int>, std::greater<int>> ref;

        for (int i = 0; i < 200; ++i) {
            const int value = dist(rng);
            RefMaxKPush(ref, 5, value);
            q_const.push(value);
            q_move.push(value);
        }

        EXPECT_EQ(SortedElements(q_const), SortedMultisetFromRef(ref));
        EXPECT_EQ(SortedElements(q_move), SortedMultisetFromRef(ref));
        EXPECT_TRUE(fermat::is_heap(q_const.get_container().begin(), q_const.get_container().end(),
                                    std::greater<int>()));
        EXPECT_TRUE(fermat::is_heap(q_move.get_container().begin(), q_move.get_container().end(),
                                    std::greater<int>()));
    }
}

TEST(MaxKQueueTest, SetCapacityShrinks) {
    IntMaxKQueue q(5);
    for (int v : {1, 5, 3, 9, 7, 2, 8})
        q.push(v);

    EXPECT_EQ(q.size(), 5u);
    q.set_capacity(2);
    EXPECT_EQ(q.size(), 2u);
    EXPECT_EQ(q.capacity(), 2u);
    EXPECT_EQ(SortedElements(q), (std::vector<int>{8, 9}));
}

TEST(MaxKQueueTest, PopRemovesThreshold) {
    IntMaxKQueue q(3);
    for (int v : {4, 1, 3, 2})
        q.push(v);

    EXPECT_EQ(q.top(), 2);
    q.pop();
    EXPECT_EQ(SortedElements(q), (std::vector<int>{3, 4}));
    EXPECT_EQ(q.top(), 3);
}

TEST(MaxKQueueTest, ZeroCapacityIgnoresPush) {
    IntMaxKQueue q(0);
    q.push(42);
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.capacity(), 0u);
}

TEST(MaxKQueueTest, Swap) {
    IntMaxKQueue a(2);
    IntMaxKQueue b(2);
    a.push(10);
    a.push(20);
    b.push(1);
    b.push(2);

    swap(a, b);
    EXPECT_EQ(SortedElements(a), (std::vector<int>{1, 2}));
    EXPECT_EQ(SortedElements(b), (std::vector<int>{10, 20}));
}

TEST(MinKQueueTest, KeepsKSmallest) {
    IntMinKQueue q(3);

    q.push(4);
    q.push(3);
    q.push(2);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.top(), 4);

    q.push(1);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(SortedElements(q), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(q.top(), 3);
}

TEST(MinKQueueTest, RejectsLargerWhenFull) {
    IntMinKQueue q(3);
    for (int v : {1, 2, 3})
        q.push(v);

    q.push(10);
    EXPECT_EQ(SortedElements(q), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(q.top(), 3);
}

TEST(MinKQueueTest, ConstAndMovePushMatchReference) {
    std::mt19937 rng(456);
    std::uniform_int_distribution<int> dist(0, 100);

    for (int trial = 0; trial < 50; ++trial) {
        IntMinKQueue q_const(5);
        IntMinKQueue q_move(5);
        std::priority_queue<int, std::vector<int>, std::less<int>> ref;

        for (int i = 0; i < 200; ++i) {
            const int value = dist(rng);
            RefMinKPush(ref, 5, value);
            q_const.push(value);
            q_move.push(value);
        }

        EXPECT_EQ(SortedElements(q_const), SortedMultisetFromRef(ref));
        EXPECT_EQ(SortedElements(q_move), SortedMultisetFromRef(ref));
        EXPECT_TRUE(fermat::is_heap(q_const.get_container().begin(), q_const.get_container().end(),
                                    std::less<int>()));
        EXPECT_TRUE(fermat::is_heap(q_move.get_container().begin(), q_move.get_container().end(),
                                    std::less<int>()));
    }
}

TEST(MinKQueueTest, SetCapacityShrinks) {
    IntMinKQueue q(5);
    for (int v : {9, 1, 7, 3, 5, 8, 2})
        q.push(v);

    EXPECT_EQ(q.size(), 5u);
    q.set_capacity(2);
    EXPECT_EQ(q.size(), 2u);
    EXPECT_EQ(SortedElements(q), (std::vector<int>{1, 2}));
}

TEST(MinKQueueTest, PopRemovesThreshold) {
    IntMinKQueue q(3);
    for (int v : {5, 1, 4, 2})
        q.push(v);

    EXPECT_EQ(SortedElements(q), (std::vector<int>{1, 2, 4}));
    EXPECT_EQ(q.top(), 4);
    q.pop();
    EXPECT_EQ(SortedElements(q), (std::vector<int>{1, 2}));
    EXPECT_EQ(q.top(), 2);
}

TEST(MinKQueueTest, ZeroCapacityIgnoresPush) {
    IntMinKQueue q(0);
    q.push(42);
    EXPECT_TRUE(q.empty());
}

TEST(MinKQueueTest, Swap) {
    IntMinKQueue a(2);
    IntMinKQueue b(2);
    a.push(10);
    a.push(20);
    b.push(1);
    b.push(2);

    swap(a, b);
    EXPECT_EQ(SortedElements(a), (std::vector<int>{1, 2}));
    EXPECT_EQ(SortedElements(b), (std::vector<int>{10, 20}));
}

} // namespace
