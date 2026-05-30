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

#include <fermat/container/list.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

namespace {

using fermat::List;
using fermat::erase;
using fermat::erase_if;

using IntList = List<int>;

template<typename ListType>
void ExpectListContents(const ListType &list, std::initializer_list<typename ListType::value_type> expected) {
    EXPECT_EQ(list.size(), expected.size());
    EXPECT_EQ(list, ListType(expected));
}

void TestList() {
    // list()
    {
        IntList l;
        EXPECT_EQ(l.size(), 0u);
        EXPECT_TRUE(l.empty());
        EXPECT_TRUE(l.validate());
        EXPECT_EQ(l.begin(), l.end());
    }

    // explicit list(size_type n, const allocator_type& allocator = ...);
    {
        const int test_size = 42;
        IntList l(test_size);
        EXPECT_FALSE(l.empty());
        EXPECT_EQ(l.size(), static_cast<size_t>(test_size));
        EXPECT_TRUE(l.validate());
        EXPECT_TRUE(std::all_of(l.begin(), l.end(), [](int e) { return e == 0; }));
    }

    // list(size_type n, const value_type& value, ...);
    {
        const int test_size = 42;
        const int test_val = 435;
        IntList l(test_size, test_val);
        EXPECT_FALSE(l.empty());
        EXPECT_EQ(l.size(), static_cast<size_t>(test_size));
        EXPECT_TRUE(l.validate());
        EXPECT_TRUE(std::all_of(l.begin(), l.end(), [=](int e) { return e == test_val; }));
    }

    // list(const this_type& x);
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList b(a);
        EXPECT_EQ(a, b);
        EXPECT_TRUE(a.validate());
        EXPECT_EQ(a.size(), b.size());
        EXPECT_TRUE(b.validate());
    }

    // list(this_type&& x);
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(a.size(), 10u);
        EXPECT_TRUE(a.validate());

        IntList b(std::move(a));
        EXPECT_TRUE(a.empty());
        EXPECT_FALSE(b.empty());
        EXPECT_EQ(a.size(), 0u);
        EXPECT_EQ(b.size(), 10u);
        EXPECT_NE(a, b);
        EXPECT_NE(a.size(), b.size());
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
    }

    // list(std::initializer_list<value_type> ilist, ...);
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        int inc = 0;
        for (const int e : a) {
            EXPECT_EQ(inc++, e);
        }
    }

    // list(InputIterator first, InputIterator last);
    {
        const IntList ref = {3, 4, 5, 6, 7};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

        auto start = a.begin();
        std::advance(start, 3);
        auto end = start;
        std::advance(end, 5);

        IntList b(start, end);
        EXPECT_EQ(b, ref);
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
        EXPECT_EQ(a.size(), 10u);
        EXPECT_EQ(b.size(), 5u);
        EXPECT_FALSE(b.empty());
        EXPECT_FALSE(a.empty());
    }

    // operator= copy / move / ilist
    {
        const IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList b = a;
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
        EXPECT_EQ(a.size(), 10u);
        EXPECT_EQ(b.size(), 10u);
        EXPECT_FALSE(a.empty());
        EXPECT_FALSE(b.empty());
        EXPECT_EQ(b, a);

        IntList c = std::move(b);
        EXPECT_TRUE(b.empty());
        EXPECT_EQ(c, a);
        EXPECT_EQ(c.size(), 10u);
        EXPECT_TRUE(c.validate());
    }

    // swap
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList b = {};
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
        EXPECT_FALSE(a.empty());
        EXPECT_TRUE(b.empty());

        b.swap(a);
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
        EXPECT_TRUE(a.empty());
        EXPECT_FALSE(b.empty());
    }

    // assign(size_type n, const value_type& value);
    {
        const IntList ref = {42, 42, 42, 42};
        IntList a = {0, 1, 2, 3};
        a.assign(4, 42);
        EXPECT_EQ(a, ref);
        EXPECT_TRUE(a.validate());
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(a.size(), 4u);
    }

    // assign(InputIterator first, InputIterator last);
    {
        const IntList ref = {3, 4, 5, 6, 7};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList b;

        auto start = a.begin();
        std::advance(start, 3);
        auto end = start;
        std::advance(end, 5);

        b.assign(start, end);
        EXPECT_EQ(b, ref);
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
        EXPECT_EQ(a.size(), 10u);
        EXPECT_EQ(b.size(), 5u);
        EXPECT_FALSE(b.empty());
        EXPECT_FALSE(a.empty());
    }

    // assign(std::initializer_list<value_type> ilist);
    {
        const IntList ref = {3, 4, 5, 6, 7};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList b;
        b.assign({3, 4, 5, 6, 7});
        EXPECT_EQ(b, ref);
        EXPECT_TRUE(a.validate());
        EXPECT_TRUE(b.validate());
        EXPECT_EQ(a.size(), 10u);
        EXPECT_EQ(b.size(), 5u);
        EXPECT_FALSE(b.empty());
        EXPECT_FALSE(a.empty());
    }

    // begin / end / cbegin / cend
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        {
            int inc = 0;
            auto iter = a.begin();
            while (iter != a.end()) {
                EXPECT_EQ(*iter++, inc++);
            }
        }
        {
            int inc = 0;
            auto iter = a.cbegin();
            while (iter != a.cend()) {
                EXPECT_EQ(*iter++, inc++);
            }
        }
    }

    // reverse iterators
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        {
            int inc = 9;
            auto iter = a.rbegin();
            while (iter != a.rend()) {
                EXPECT_EQ(*iter, inc--);
                ++iter;
            }
        }
        {
            int inc = 9;
            auto iter = a.crbegin();
            while (iter != a.crend()) {
                EXPECT_EQ(*iter, inc--);
                ++iter;
            }
        }
    }

    // empty / size
    {
        EXPECT_FALSE((IntList{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}.empty()));
        EXPECT_TRUE(IntList{}.empty());
        EXPECT_EQ((IntList{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}).size(), 10u);
        EXPECT_EQ((IntList{0, 1, 2, 3, 4}).size(), 5u);
        EXPECT_EQ((IntList{0, 1}).size(), 2u);
        EXPECT_EQ(IntList{}.size(), 0u);
    }

    // resize
    {
        IntList a;
        a.resize(10);
        EXPECT_EQ(a.size(), 10u);
        EXPECT_FALSE(a.empty());
        EXPECT_TRUE(std::all_of(a.begin(), a.end(), [](int i) { return i == 0; }));

        IntList b;
        b.resize(10, 42);
        EXPECT_EQ(b.size(), 10u);
        EXPECT_FALSE(b.empty());
        EXPECT_TRUE(std::all_of(b.begin(), b.end(), [](int i) { return i == 42; }));
    }

    // front
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        EXPECT_EQ(a.front(), 0);
        a.front() = 42;
        EXPECT_EQ(a.front(), 42);

        const IntList ca = {5, 6, 7, 8, 9};
        EXPECT_EQ(ca.front(), 5);

        IntList single = {9};
        EXPECT_EQ(single.front(), 9);
        single.front() = 42;
        EXPECT_EQ(single.front(), 42);
    }

    // back
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        EXPECT_EQ(a.back(), 9);
        a.back() = 42;
        EXPECT_EQ(a.back(), 42);

        const IntList ca = {5, 6, 7, 8, 9};
        EXPECT_EQ(ca.back(), 9);

        IntList single = {9};
        EXPECT_EQ(single.back(), 9);
        single.back() = 42;
        EXPECT_EQ(single.back(), 42);
    }

    // emplace_front
    {
        const IntList ref = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        IntList a;
        for (int i = 0; i < 10; ++i) {
            EXPECT_EQ(a.emplace_front(i), i);
        }
        EXPECT_EQ(a, ref);
    }

    // emplace_back
    {
        {
            const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            IntList a;
            for (int i = 0; i < 10; ++i) {
                EXPECT_EQ(a.emplace_back(i), i);
            }
            EXPECT_EQ(a, ref);
        }

        struct A {
            A() : mValue(0) {}
            explicit A(int in) : mValue(in) {}
            int mValue;
            bool operator==(const A &other) const { return mValue == other.mValue; }
        };

        {
            const List<A> ref = {A(1), A(2), A(3)};
            List<A> a;
            EXPECT_EQ(a.emplace_back(1), A{1});
            EXPECT_EQ(a.emplace_back(2), A{2});
            EXPECT_EQ(a.emplace_back(3), A{3});
            EXPECT_EQ(a, ref);
        }

        {
            const List<A> ref = {A(1), A(2), A(3)};
            List<A> a;
            EXPECT_EQ(a.emplace_back(A(1)), A{1});
            EXPECT_EQ(a.emplace_back(A(2)), A{2});
            EXPECT_EQ(a.emplace_back(A(3)), A{3});
            EXPECT_EQ(a, ref);
        }

        {
            const List<A> ref = {A(1), A(2), A(3)};
            List<A> a;
            A a1(1), a2(2), a3(3);
            EXPECT_EQ(a.emplace_back(a1), A{1});
            EXPECT_EQ(a.emplace_back(a2), A{2});
            EXPECT_EQ(a.emplace_back(a3), A{3});
            EXPECT_EQ(a, ref);
        }
    }

    // push_front
    {
        const IntList ref = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        IntList a;
        for (int i = 0; i < 10; ++i) {
            a.push_front(i);
        }
        EXPECT_EQ(a, ref);

        IntList b;
        auto &front_ref = b.push_front();
        EXPECT_EQ(b.front(), 0);
        front_ref = 42;
        EXPECT_EQ(b.front(), 42);
    }

    // push_front_uninitialized
    {
        IntList a;
        for (unsigned i = 0; i < 100; ++i) {
            EXPECT_NE(a.push_front_uninitialized(), nullptr);
            EXPECT_EQ(a.size(), i + 1);
        }
    }

    // push_back
    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a;
        for (int i = 0; i < 10; ++i) {
            a.push_back(i);
        }
        EXPECT_EQ(a, ref);

        struct B {
            int mValue;
        };
        List<B> lb;
        lb.push_back(B{42});
        EXPECT_EQ(lb.back().mValue, 42);
    }

    // push_back() / push_back_uninitialized
    {
        IntList a;
        auto &back_ref = a.push_back();
        EXPECT_EQ(a.back(), 0);
        back_ref = 42;
        EXPECT_EQ(a.back(), 42);

        IntList b;
        for (unsigned i = 0; i < 100; ++i) {
            EXPECT_NE(b.push_back_uninitialized(), nullptr);
            EXPECT_EQ(b.size(), i + 1);
        }
    }

    // pop_front / pop_back
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        for (unsigned i = 0; i < 10; ++i) {
            EXPECT_EQ(static_cast<unsigned>(a.front()), i);
            a.pop_front();
        }
        EXPECT_TRUE(a.empty());
    }

    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        for (unsigned i = 0; i < 10; ++i) {
            EXPECT_EQ(static_cast<unsigned>(a.back()), 9u - i);
            a.pop_back();
        }
        EXPECT_TRUE(a.empty());
    }

    // emplace / insert
    {
        const IntList ref = {0, 1, 2, 3, 4, 42, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto insert_pos = a.begin();
        std::advance(insert_pos, 5);
        a.emplace(insert_pos, 42);
        EXPECT_EQ(a, ref);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 42, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto insert_pos = a.begin();
        std::advance(insert_pos, 5);
        a.insert(insert_pos, 42);
        EXPECT_EQ(a, ref);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 42, 42, 42, 42, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto insert_pos = a.begin();
        std::advance(insert_pos, 5);
        auto result = a.insert(insert_pos, 4, 42);
        EXPECT_EQ(a, ref);
        EXPECT_EQ(*result, 42);
        EXPECT_EQ(*(--result), 4);
    }

    {
        const IntList to_insert = {42, 42, 42, 42};
        const IntList ref = {0, 1, 2, 3, 4, 42, 42, 42, 42, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto insert_pos = a.begin();
        std::advance(insert_pos, 5);
        auto result = a.insert(insert_pos, to_insert.begin(), to_insert.end());
        EXPECT_EQ(a, ref);
        EXPECT_EQ(*result, 42);
        EXPECT_EQ(*(--result), 4);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 42, 42, 42, 42, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto insert_pos = a.begin();
        std::advance(insert_pos, 5);
        a.insert(insert_pos, {42, 42, 42, 42});
        EXPECT_EQ(a, ref);
    }

    // erase
    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 42, 5, 6, 7, 8, 9};
        auto erase_pos = a.begin();
        std::advance(erase_pos, 5);
        auto iter_after_removed = a.erase(erase_pos);
        EXPECT_EQ(*iter_after_removed, 5);
        EXPECT_EQ(a, ref);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 42, 42, 42, 42, 5, 6, 7, 8, 9};
        auto erase_begin = a.begin();
        std::advance(erase_begin, 5);
        auto erase_end = erase_begin;
        std::advance(erase_end, 4);
        a.erase(erase_begin, erase_end);
        EXPECT_EQ(a, ref);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 42, 5, 6, 7, 8, 9};
        auto erase_rbegin = a.rbegin();
        std::advance(erase_rbegin, 5);
        auto iter_after_remove = a.erase(erase_rbegin);
        EXPECT_EQ(*iter_after_remove, 4);
        EXPECT_EQ(a, ref);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a = {0, 1, 2, 3, 4, 42, 42, 42, 42, 5, 6, 7, 8, 9};
        auto erase_crbegin = a.crbegin();
        auto erase_crend = a.crbegin();
        std::advance(erase_crbegin, 4);
        std::advance(erase_crend, 8);
        auto iter_after_removed = a.erase(erase_crbegin, erase_crend);
        EXPECT_EQ(*iter_after_removed, 4);
        EXPECT_EQ(a, ref);
    }

    // clear
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        a.clear();
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(a.size(), 0u);
    }

    // remove / remove_if
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const IntList ref = {0, 1, 2, 3, 5, 6, 7, 8, 9};
        EXPECT_EQ(a.remove(4), 1u);
        EXPECT_EQ(a, ref);
    }

    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const IntList ref = {0, 1, 2, 3, 5, 6, 7, 8, 9};
        EXPECT_EQ(a.remove_if([](int e) { return e == 4; }), 1u);
        EXPECT_EQ(a, ref);
    }

    // reverse
    {
        IntList a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const IntList ref = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        a.reverse();
        EXPECT_EQ(a, ref);
    }

    // splice
    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a1 = {0, 1, 2, 3, 4};
        IntList a2 = {5, 6, 7, 8, 9};
        IntList a;
        a.splice(a.begin(), a2);
        a.splice(a.begin(), a1);
        EXPECT_EQ(a, ref);
        EXPECT_TRUE(a1.empty());
        EXPECT_TRUE(a2.empty());
    }

    {
        const IntList ref = {0, 5};
        IntList a1 = {-1, -1, 0};
        IntList a2 = {-1, -1, 5};
        auto a1_begin = a1.begin();
        auto a2_begin = a2.begin();
        std::advance(a1_begin, 2);
        std::advance(a2_begin, 2);
        IntList a;
        a.splice(a.begin(), a2, a2_begin);
        a.splice(a.begin(), a1, a1_begin);
        EXPECT_EQ(a, ref);
        EXPECT_FALSE(a1.empty());
        EXPECT_FALSE(a2.empty());
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a1 = {-1, -1, 0, 1, 2, 3, 4, -1, -1};
        IntList a2 = {-1, -1, 5, 6, 7, 8, 9, -1, -1};
        auto a1_begin = a1.begin();
        auto a2_begin = a2.begin();
        auto a1_end = a1.end();
        auto a2_end = a2.end();
        std::advance(a1_begin, 2);
        std::advance(a2_begin, 2);
        std::advance(a1_end, -2);
        std::advance(a2_end, -2);
        IntList a;
        a.splice(a.begin(), a2, a2_begin, a2_end);
        a.splice(a.begin(), a1, a1_begin, a1_end);
        const IntList rref = {-1, -1, -1, -1};
        EXPECT_EQ(a, ref);
        EXPECT_EQ(a1, rref);
        EXPECT_EQ(a2, rref);
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a1 = {0, 1, 2, 3, 4};
        IntList a2 = {5, 6, 7, 8, 9};
        IntList a;
        a.splice(a.begin(), std::move(a2));
        a.splice(a.begin(), std::move(a1));
        EXPECT_EQ(a, ref);
        EXPECT_TRUE(a1.empty());
        EXPECT_TRUE(a2.empty());
    }

    {
        const IntList ref = {0, 5};
        IntList a1 = {-1, -1, 0};
        IntList a2 = {-1, -1, 5};
        auto a1_begin = a1.begin();
        auto a2_begin = a2.begin();
        std::advance(a1_begin, 2);
        std::advance(a2_begin, 2);
        IntList a;
        a.splice(a.begin(), std::move(a2), a2_begin);
        a.splice(a.begin(), std::move(a1), a1_begin);
        EXPECT_EQ(a, ref);
        EXPECT_FALSE(a1.empty());
        EXPECT_FALSE(a2.empty());
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a1 = {-1, -1, 0, 1, 2, 3, 4, -1, -1};
        IntList a2 = {-1, -1, 5, 6, 7, 8, 9, -1, -1};
        auto a1_begin = a1.begin();
        auto a2_begin = a2.begin();
        auto a1_end = a1.end();
        auto a2_end = a2.end();
        std::advance(a1_begin, 2);
        std::advance(a2_begin, 2);
        std::advance(a1_end, -2);
        std::advance(a2_end, -2);
        IntList a;
        a.splice(a.begin(), std::move(a2), a2_begin, a2_end);
        a.splice(a.begin(), std::move(a1), a1_begin, a1_end);
        const IntList rref = {-1, -1, -1, -1};
        EXPECT_EQ(a, ref);
        EXPECT_EQ(a1, rref);
        EXPECT_EQ(a2, rref);
    }

    // merge
    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a1 = {0, 1, 2, 3, 4};
        IntList a2 = {5, 6, 7, 8, 9};
        a1.merge(a2);
        EXPECT_EQ(a1, ref);
        EXPECT_TRUE(a2.empty());
    }

    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a1 = {0, 1, 2, 3, 4};
        IntList a2 = {5, 6, 7, 8, 9};
        a1.merge(a2, [](int lhs, int rhs) { return lhs < rhs; });
        EXPECT_EQ(a1, ref);
        EXPECT_TRUE(a2.empty());
    }

    // unique
    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 3, 3, 3,
                     4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9};
        EXPECT_EQ(a.unique(), 34u);
        EXPECT_EQ(a, ref);
    }

    {
        static bool bBreakComparison;
        struct A {
            int mValue;
            bool operator==(const A &other) const {
                return bBreakComparison ? false : mValue == other.mValue;
            }
        };

        const List<A> ref = {A{0}, A{1}, A{2}, A{3}, A{4}, A{5}, A{6}, A{7}, A{8}, A{9}};
        List<A> a = {A{0}, A{0}, A{0}, A{0}, A{0}, A{0}, A{1}, A{2}, A{2}, A{2}, A{2}, A{3}, A{4}, A{5},
                     A{5}, A{5}, A{5}, A{5}, A{6}, A{7}, A{7}, A{7}, A{7}, A{8}, A{9}, A{9}, A{9}};

        bBreakComparison = true;
        EXPECT_EQ(a.unique(), 0u);
        EXPECT_NE(a, ref);

        EXPECT_EQ(a.unique([](const A &lhs, const A &rhs) { return lhs.mValue == rhs.mValue; }), 17u);
        bBreakComparison = false;
        EXPECT_EQ(a, ref);
    }

    // sort
    {
        const IntList ref = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        IntList a = {9, 4, 5, 3, 1, 0, 6, 2, 7, 8};
        a.sort();
        EXPECT_EQ(a, ref);
    }

    {
        struct A {
            int mValue;
            bool operator==(const A &other) const { return mValue == other.mValue; }
        };
        const List<A> ref = {A{0}, A{1}, A{2}, A{3}, A{4}, A{5}, A{6}, A{7}, A{8}, A{9}};
        List<A> a = {A{1}, A{0}, A{2}, A{9}, A{4}, A{5}, A{6}, A{7}, A{3}, A{8}};
        a.sort([](const A &lhs, const A &rhs) { return lhs.mValue < rhs.mValue; });
        EXPECT_EQ(a, ref);
    }

    // erase / erase_if (global)
    {
        IntList l = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        EXPECT_EQ(erase(l, 3), 1u);
        EXPECT_EQ(erase(l, 5), 1u);
        EXPECT_EQ(erase(l, 7), 1u);
        EXPECT_EQ(l, (IntList{1, 2, 4, 6, 8, 9}));
    }

    {
        IntList l = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        EXPECT_EQ(erase_if(l, [](auto i) { return i % 2 == 0; }), 4u);
        EXPECT_EQ(l, (IntList{1, 3, 5, 7, 9}));
    }

    // global operators
    {
        const IntList list1 = {0, 1, 2, 3, 4, 5};
        const IntList list2 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const IntList list3 = {5, 6, 7, 8};

        EXPECT_EQ(list1, list1);
        EXPECT_NE(list1, list2);
        EXPECT_NE(list2, list3);
        EXPECT_NE(list1, list3);

        EXPECT_LT(list1, list2);
        EXPECT_LE(list1, list2);
        EXPECT_GT(list2, list1);
        EXPECT_GE(list2, list1);
        EXPECT_GT(list3, list1);
        EXPECT_GT(list3, list2);
    }
}

} // namespace

TEST(ListTest, Comprehensive) {
    TestList();
}
