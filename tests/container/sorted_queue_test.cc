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

#include <fermat/container/sorted_queue.h>
#include <fermat/container/vector.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

namespace {

using fermat::SortedQueue;
using fermat::Vector;

using IntSortedQueue = SortedQueue<int, 0>;

TEST(SortedQueueTest, DefaultEmptyAndPushPopBack) {
    IntSortedQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);

    q.push(3);
    q.push(1);
    q.push(2);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_TRUE(q.validate());
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);

    q.pop_back();
    EXPECT_EQ(q.back(), 2);
    q.pop_front();
    EXPECT_EQ(q.front(), 2);
    EXPECT_EQ(q.size(), 1u);

    q.pop_back();
    EXPECT_TRUE(q.empty());
    EXPECT_TRUE(q.validate());
}

TEST(SortedQueueTest, PopFrontDrainsAscending) {
    IntSortedQueue q;
    for (int v : {5, 1, 4, 2, 3})
        q.push(v);

    std::vector<int> out;
    while (!q.empty()) {
        out.push_back(q.front());
        q.pop_front();
    }
    EXPECT_TRUE(std::is_sorted(out.begin(), out.end()));
    EXPECT_EQ(out, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(SortedQueueTest, PopBackDrainsDescending) {
    IntSortedQueue q;
    for (int v : {5, 1, 4, 2, 3})
        q.push(v);

    std::vector<int> out;
    while (!q.empty()) {
        out.push_back(q.back());
        q.pop_back();
    }
    EXPECT_TRUE(std::is_sorted(out.begin(), out.end(), std::greater<int>()));
    EXPECT_EQ(out, (std::vector<int>{5, 4, 3, 2, 1}));
}

TEST(SortedQueueTest, ConstructFromInitializerList) {
    IntSortedQueue q({3, 1, 2});
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);
    EXPECT_TRUE(q.validate());
}

TEST(SortedQueueTest, BoundedWrapperPattern) {
    IntSortedQueue q;
    constexpr size_t kLimit = 5;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 100);

    for (int i = 0; i < 200; ++i) {
        q.push(dist(rng));
        if (q.size() > kLimit)
            q.pop_front();
    }

    EXPECT_EQ(q.size(), kLimit);
    EXPECT_TRUE(q.validate());
    EXPECT_TRUE(std::is_sorted(q.start, q.end));
}

TEST(SortedQueueTest, ClearAndReuse) {
    IntSortedQueue q;
    q.push(10);
    q.push(1);
    q.clear();
    EXPECT_TRUE(q.empty());

    q.push(7);
    q.push(3);
    EXPECT_EQ(q.front(), 3);
    EXPECT_EQ(q.back(), 7);
    EXPECT_TRUE(q.validate());
}

TEST(SortedQueueTest, PopWithOutParameter) {
    IntSortedQueue q;
    q.push(2);
    q.push(5);

    int v = 0;
    q.pop_front(v);
    EXPECT_EQ(v, 2);
    q.pop_back(v);
    EXPECT_EQ(v, 5);
    EXPECT_TRUE(q.empty());
}

TEST(SortedQueueTest, CompareOperators) {
    IntSortedQueue a;
    IntSortedQueue b;
    a.push(1);
    a.push(2);
    b.push(1);
    b.push(2);
    EXPECT_EQ(a, b);
    EXPECT_FALSE(a < b);

    b.pop_back();
    b.push(3);
    EXPECT_NE(a, b);
    EXPECT_LT(a, b);
}

} // namespace
