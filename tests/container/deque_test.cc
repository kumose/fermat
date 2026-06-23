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

#include <fermat/container/deque.h>
#include <fermat/container/list.h>
#include <fermat/container/string.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <iterator>
#include <list>
#include <memory>
#include <utility>
#include <vector>

namespace {
    template<typename T, unsigned kSubarraySize>
    using DequeK = fermat::Deque<T, 64, fermat::TimesPolicy<2, 1>,
        fermat::AlignedAllocator<T, 64>,
        fermat::AlignedAllocator<T *, 64>,
        kSubarraySize>;

    using IntDeque = DequeK<int, 512>;
    using IntDeque1 = DequeK<int, 1>;
    using IntDeque32768 = DequeK<int, 32768>;

    struct TestObject {
        static std::atomic<size_t> ctor_count;
        static std::atomic<size_t> dtor_count;
        static std::atomic<size_t> copy_ctor_count;
        static std::atomic<size_t> move_ctor_count;

        int value;

        TestObject(int v = 0) : value(v) { ++ctor_count; }
        TestObject(int a, int b, int c) : value(a + b + c) { ++ctor_count; }
        TestObject(const TestObject &other) : value(other.value) { ++copy_ctor_count; }
        TestObject(TestObject &&other) noexcept : value(other.value) { ++move_ctor_count; }
        ~TestObject() { ++dtor_count; }

        TestObject &operator=(const TestObject &other) {
            value = other.value;
            return *this;
        }

        TestObject &operator=(TestObject &&other) noexcept {
            value = other.value;
            return *this;
        }

        static void Reset() {
            ctor_count = 0;
            dtor_count = 0;
            copy_ctor_count = 0;
            move_ctor_count = 0;
        }

        static bool IsBalanced() {
            return (ctor_count.load() + copy_ctor_count.load() + move_ctor_count.load()) == dtor_count.load();
        }

        bool operator==(const TestObject &other) const { return value == other.value; }
    };

    std::ostream &operator<<(std::ostream &os, const TestObject &obj) {
        os << obj.value;
        return os;
    }

    std::atomic<size_t> TestObject::ctor_count{0};
    std::atomic<size_t> TestObject::dtor_count{0};
    std::atomic<size_t> TestObject::copy_ctor_count{0};
    std::atomic<size_t> TestObject::move_ctor_count{0};

    using TestObjectDeque = DequeK<TestObject, 512>;
    using TestObjectDeque1 = DequeK<TestObject, 1>;

    template<typename Range, typename Expected>
    bool VerifySequence(const Range &range, const Expected &expected) {
        return static_cast<size_t>(std::distance(std::begin(range), std::end(range))) == expected.size() &&
               std::equal(std::begin(range), std::end(range), expected.begin());
    }

    template<typename It, typename Expected>
    bool VerifySequence(It first, It last, const Expected &expected) {
        return static_cast<size_t>(std::distance(first, last)) == expected.size() &&
               std::equal(first, last, expected.begin());
    }

    struct MoveOnly {
        int value{0};

        MoveOnly() = default;

        explicit MoveOnly(int v) : value(v) {
        }

        MoveOnly(MoveOnly &&) noexcept = default;

        MoveOnly &operator=(MoveOnly &&) noexcept = default;

        MoveOnly(const MoveOnly &) = delete;

        MoveOnly &operator=(const MoveOnly &) = delete;

        static MoveOnly Create() { return MoveOnly(42); }
    };

    template<typename T>
    class ValueInputIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;

        ValueInputIterator() : value_(0), end_(0) {
        }

        ValueInputIterator(T value, T end) : value_(value), end_(end) {
        }

        reference operator*() const { return value_; }
        pointer operator->() const { return &value_; }

        ValueInputIterator &operator++() {
            ++value_;
            return *this;
        }

        ValueInputIterator operator++(int) {
            ValueInputIterator tmp(*this);
            ++*this;
            return tmp;
        }

        friend bool operator==(const ValueInputIterator &a, const ValueInputIterator &b) {
            return a.value_ == b.value_;
        }

        friend bool operator!=(const ValueInputIterator &a, const ValueInputIterator &b) {
            return !(a == b);
        }

    private:
        T value_;
        T end_;
    };

    // Incomplete-type pattern: non-default subarray size is required.
    struct StructWithContainerOfStructs {
        DequeK<StructWithContainerOfStructs, 16> children;
    };

    template<typename Deque>
    void TestDequeConstruction() {
        using value_type = typename Deque::value_type;
        using size_type = typename Deque::size_type;

        Deque dA;
        EXPECT_EQ(dA.size(), 0u);
        EXPECT_TRUE(dA.empty());
        EXPECT_TRUE(dA.validate());

        Deque dB(static_cast<size_type>(0));
        EXPECT_EQ(dB.size(), 0u);

        Deque dC(1000);
        EXPECT_EQ(dC.size(), 1000u);
        for (const auto &elem: dC) {
            EXPECT_EQ(elem, value_type());
        }

        Deque dD(2000, value_type(1));
        EXPECT_EQ(dD.size(), 2000u);
        for (const auto &elem: dD) {
            EXPECT_EQ(elem, value_type(1));
        }

        Deque dE(dC);
        EXPECT_EQ(dE.size(), 1000u);
        for (const auto &elem: dE) {
            EXPECT_EQ(elem, value_type());
        }

        Deque dF(dC.begin(), dC.end());
        EXPECT_EQ(dF.size(), 1000u);
        for (const auto &elem: dF) {
            EXPECT_EQ(elem, value_type());
        }

        dE = dD;
        EXPECT_EQ(dE.size(), 2000u);
        for (const auto &elem: dE) {
            EXPECT_EQ(elem, value_type(1));
        }

        dE.swap(dC);
        EXPECT_EQ(dE.size(), 1000u);
        for (const auto &elem: dE) {
            EXPECT_EQ(elem, value_type());
        }
        EXPECT_EQ(dC.size(), 2000u);
        for (const auto &elem: dC) {
            EXPECT_EQ(elem, value_type(1));
        }

        dA.clear();
        EXPECT_TRUE(dA.empty());
        dB.clear();
        EXPECT_TRUE(dB.empty());
    }

    template<typename Deque>
    void TestDequeSimpleMutation() {
        using value_type = typename Deque::value_type;
        using size_type = typename Deque::size_type;

        {
            Deque d;
            for (int i = 0; i < 1000; ++i) {
                d.push_back(value_type(i));
                EXPECT_EQ(d.back(), value_type(i));
            }
            EXPECT_EQ(d.front(), value_type(0));
            for (size_type i = 0; i < d.size(); ++i) {
                EXPECT_EQ(d[i], value_type(static_cast<int>(i)));
                EXPECT_EQ(d.at(i), value_type(static_cast<int>(i)));
            }
        }

        {
            Deque d;
            for (int i = 0; i < 1000; ++i) {
                value_type &ref = d.push_back();
                EXPECT_EQ(&ref, &d.back());
                EXPECT_EQ(d.back(), value_type());
            }
            EXPECT_EQ(d.front(), value_type());
            for (size_type i = 0; i < d.size(); ++i) {
                EXPECT_EQ(d[i], value_type());
                EXPECT_EQ(d.at(i), value_type());
            }
        }

        {
            Deque d;
            for (int i = 0; i < 1000; ++i) {
                d.push_front(value_type(i));
                EXPECT_EQ(d.front(), value_type(i));
            }
            EXPECT_EQ(d.size(), 1000u);
            for (int i = 0; i < 1000; ++i) {
                EXPECT_EQ(d[1000 - 1 - i], value_type(i));
                EXPECT_EQ(d.at(1000 - 1 - i), value_type(i));
            }
        }

        {
            Deque d;
            for (int i = 0; i < 1000; ++i) {
                value_type &ref = d.push_front();
                EXPECT_EQ(&ref, &d.front());
                EXPECT_EQ(d.front(), value_type());
            }
            EXPECT_EQ(d.back(), value_type());

            for (size_type i = 0; i < d.size(); ++i) {
                EXPECT_EQ(d[i], value_type());
            }

            for (int i = 0; i < 500; ++i) {
                d.pop_back();
            }
            EXPECT_EQ(d.size(), 500u);
            for (const auto &elem: d) {
                EXPECT_EQ(elem, value_type());
            }

            for (int i = 0; i < 500; ++i) {
                d.pop_front();
            }
            EXPECT_EQ(d.size(), 0u);
        }

        {
            Deque d;
            for (int i = 0; i < 500; ++i) {
                d.resize(d.size() + 3, value_type(i));
                EXPECT_EQ(d.size(), static_cast<size_type>((i + 1) * 3));
            }
            EXPECT_EQ(d.size(), 1500u);
            for (int i = 0; i < 500; ++i) {
                EXPECT_EQ(d[i * 3 + 0], value_type(i));
                EXPECT_EQ(d[i * 3 + 1], value_type(i));
                EXPECT_EQ(d[i * 3 + 2], value_type(i));
            }

            for (int i = 0; i < 500; ++i) {
                d.resize(d.size() - 2);
                EXPECT_EQ(d.size(), static_cast<size_type>(1500 - ((i + 1) * 2)));
            }
            EXPECT_EQ(d.size(), 500u);
        }
    }

    template<typename Deque>
    void TestDequeComplexMutation() {
        using value_type = typename Deque::value_type;
        using size_type = typename Deque::size_type;
        using iterator = typename Deque::iterator;
        using reverse_iterator = typename Deque::reverse_iterator;

        {
            Deque d;

            d.assign(100, value_type(1));
            EXPECT_EQ(d.size(), 100u);
            for (const auto &elem: d) {
                EXPECT_EQ(elem, value_type(1));
            }

            d.assign(50, value_type(2));
            EXPECT_EQ(d.size(), 50u);

            d.assign(150, value_type(3));
            EXPECT_EQ(d.size(), 150u);

            std::list<value_type> valueList;
            for (int i = 0; i < 100; ++i) {
                valueList.push_back(value_type(i));
            }

            d.assign(valueList.begin(), valueList.end());
            EXPECT_EQ(d.size(), 100u);
            for (int i = 0; i < 100; ++i) {
                EXPECT_EQ(d[i], value_type(i));
            }

            iterator itFirstInserted = d.insert(d.begin(), d[1]);
            EXPECT_EQ(itFirstInserted, d.begin());
            EXPECT_EQ(d[0], value_type(1));

            value_type value = d[d.size() - 2];
            itFirstInserted = d.insert(d.end(), value);
            EXPECT_EQ(itFirstInserted, d.end() - 1);
            EXPECT_EQ(*(d.end() - 1), value);

            iterator itNearBegin = d.begin();
            std::advance(itNearBegin, 1);
            value = d[3];
            itFirstInserted = d.insert(itNearBegin, value);
            EXPECT_EQ(itFirstInserted, d.begin() + 1);
            EXPECT_EQ(d[1], value);

            iterator itNearEnd = d.begin();
            std::advance(itNearEnd, d.size() - 1);
            value = d[d.size() - 2];
            itFirstInserted = d.insert(itNearEnd, value);
            EXPECT_EQ(itFirstInserted, d.end() - 2);
            EXPECT_EQ(d[d.size() - 2], value);

            itFirstInserted = d.insert(d.begin(), d.size() * 2, value_type(3));
            EXPECT_EQ(itFirstInserted, d.begin());
            for (size_type i = 0; i < d.size() / 2; ++i) {
                EXPECT_EQ(d[i], value_type(3));
            }

            itFirstInserted = d.insert(d.end(), d.size() * 2, value_type(3));
            EXPECT_EQ(itFirstInserted, d.begin() + d.size() / 3);
            for (size_type i = 0; i < d.size() / 2; ++i) {
                EXPECT_EQ(d[d.size() - 1 - i], value_type(3));
            }

            itNearBegin = d.begin();
            std::advance(itNearBegin, 3);
            itFirstInserted = d.insert(itNearBegin, 3, value_type(4));
            EXPECT_EQ(itFirstInserted, d.begin() + 3);
            EXPECT_TRUE(VerifySequence(
                d.begin() + 3, d.begin() + 6,
                std::vector<value_type>{value_type(4), value_type(4), value_type(4)}))<<*(d.begin() + 3);

            itNearEnd = d.begin();
            std::advance(itNearEnd, d.size() - 1);
            itFirstInserted = d.insert(d.end(), 5, value_type(6));
            EXPECT_EQ(itFirstInserted, d.end() - 5);
            EXPECT_TRUE(VerifySequence(
                d.end() - 5, d.end(),
                std::vector<value_type>{value_type(6), value_type(6), value_type(6),
                value_type(6), value_type(6)}));

            EXPECT_EQ(d.begin(), d.insert(d.begin(), 0, value_type(9)));

            itNearBegin = d.begin();
            std::advance(itNearBegin, 3);
            itFirstInserted = d.insert(itNearBegin, valueList.begin(), valueList.end());
            for (int i = 0; i < 100; ++i, ++itFirstInserted) {
                EXPECT_EQ(*itFirstInserted, value_type(i));
            }

            if constexpr (std::is_same_v<value_type, int>) {
                itFirstInserted = d.insert(d.begin(),
                                           ValueInputIterator<int>(0, 5),
                                           ValueInputIterator<int>(5, 5));
                EXPECT_EQ(itFirstInserted, d.begin());
                EXPECT_TRUE(VerifySequence(
                    d.begin(), d.begin() + 5,
                    std::vector<int>{0, 1, 2, 3, 4}));

                auto inputEnd = ValueInputIterator<int>(5, 5);
                EXPECT_EQ(d.begin(), d.insert(d.begin(), inputEnd, inputEnd));
            }

            value_type *itContiguous = nullptr;
            EXPECT_EQ(d.begin(), d.insert(d.begin(), itContiguous, itContiguous));

            itNearBegin = d.begin();
            const size_type sizeBeforeErase = d.size();
            while (itNearBegin != d.end()) {
                for (int i = 0; (i < 3) && (itNearBegin != d.end()); ++i) {
                    ++itNearBegin;
                }
                if (itNearBegin != d.end()) {
                    itNearBegin = d.erase(itNearBegin);
                }
            }
            EXPECT_EQ(d.size(), sizeBeforeErase - (sizeBeforeErase / 4));

            itNearBegin = d.begin();
            while (itNearBegin != d.end()) {
                iterator itSaved = itNearBegin;

                size_type numElementsToErase = 0;
                for (; (numElementsToErase < 22) && (itNearBegin != d.end()); ++numElementsToErase) {
                    ++itNearBegin;
                }

                if (itNearBegin != d.end()) {
                    const size_type numElementsPrior = d.size();
                    const auto dist = itNearBegin - itSaved;
                    ASSERT_EQ(dist, static_cast<typename Deque::difference_type>(numElementsToErase));
                    itNearBegin = d.erase(itSaved, itNearBegin);
                    EXPECT_EQ(d.size(), numElementsPrior - static_cast<size_type>(dist));
                }

                for (int i = 0; (i < 17) && (itNearBegin != d.end()); ++i) {
                    ++itNearBegin;
                }
            }
        }

        {
            Deque dErase;
            for (int i = 0; i < 20; ++i) {
                dErase.push_back(value_type(i));
            }
            EXPECT_EQ(dErase.size(), 20u);
            EXPECT_EQ(dErase[0], value_type(0));
            EXPECT_EQ(dErase[19], value_type(19));

            reverse_iterator rA = dErase.rbegin();
            reverse_iterator rB = rA + 3;
            dErase.erase(rA, rB);
            EXPECT_EQ(dErase.size(), 17u);
            EXPECT_EQ(dErase[0], value_type(0));
            EXPECT_EQ(dErase[16], value_type(16));

            rB = dErase.rend();
            rA = rB - 3;
            dErase.erase(rA, rB);
            EXPECT_EQ(dErase.size(), 14u);
            EXPECT_EQ(dErase[0], value_type(3));
            EXPECT_EQ(dErase[13], value_type(16));

            rB = dErase.rend() - 1;
            dErase.erase(rB);
            EXPECT_EQ(dErase.size(), 13u);
            EXPECT_EQ(dErase[0], value_type(4));
            EXPECT_EQ(dErase[12], value_type(16));

            rB = dErase.rbegin();
            dErase.erase(rB);
            EXPECT_EQ(dErase.size(), 12u);
            EXPECT_EQ(dErase[0], value_type(4));
            EXPECT_EQ(dErase[11], value_type(15));

            rA = dErase.rbegin();
            rB = dErase.rend();
            dErase.erase(rA, rB);
            EXPECT_EQ(dErase.size(), 0u);
        }
    }

    template<typename Deque>
    void RunDequeTestSuite() {
        TestObject::Reset();
        TestDequeConstruction<Deque>();
        EXPECT_TRUE(TestObject::IsBalanced());

        TestObject::Reset();
        TestDequeSimpleMutation<Deque>();
        EXPECT_TRUE(TestObject::IsBalanced());

        TestObject::Reset();
        TestDequeComplexMutation<Deque>();
        EXPECT_TRUE(TestObject::IsBalanced());
    }

    template<typename T>
    class DequeTypedTest : public ::testing::Test {
    };
} // namespace

TYPED_TEST_SUITE_P(DequeTypedTest);

TYPED_TEST_P(DequeTypedTest, ConstructionSimpleAndComplex) {
    RunDequeTestSuite<TypeParam>();
}

REGISTER_TYPED_TEST_SUITE_P(DequeTypedTest, ConstructionSimpleAndComplex);

using DequeTestTypes = ::testing::Types<
    IntDeque,
    IntDeque1,
    IntDeque32768,
    TestObjectDeque,
    TestObjectDeque1>;

INSTANTIATE_TYPED_TEST_SUITE_P(Deque, DequeTypedTest, DequeTestTypes);


TEST(DequeTest, SubarraySizeConstants) {
    EXPECT_GE(IntDeque::kSubarraySize, 4u);
    EXPECT_EQ(IntDeque1::kSubarraySize, 1u);
    EXPECT_EQ(IntDeque32768::kSubarraySize, 32768u);

    EXPECT_GE(TestObjectDeque::kSubarraySize, 2u);
    EXPECT_EQ(TestObjectDeque1::kSubarraySize, 1u);
}

TEST(DequeTest, StructWithContainerOfStructsCompiles) {
    StructWithContainerOfStructs node;
    (void) node;
    SUCCEED();
}

TEST(DequeTest, InitializerListAndAssign) {
    fermat::Deque<int> d = {0, 1, 2};
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{0, 1, 2}));

    d = {13, 14, 15};
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{13, 14, 15}));

    d.assign({16, 17, 18});
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{16, 17, 18}));

    auto it = d.insert(d.begin(), {14, 15});
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{14, 15, 16, 17, 18}));
    EXPECT_EQ(*it, 14);
}

TEST(DequeTest, MoveSemantics) {
    TestObject::Reset();
    {
        fermat::Deque<TestObject> src(3, TestObject(33));
        fermat::Deque<TestObject> moved(std::move(src));
        EXPECT_EQ(moved.size(), 3u);
        EXPECT_EQ(moved.front().value, 33);
        EXPECT_EQ(src.size(), 0u);

        fermat::Deque<TestObject> dst;
        dst = std::move(moved);
        EXPECT_EQ(dst.size(), 3u);
        EXPECT_EQ(dst.front().value, 33);
    }
    EXPECT_TRUE(TestObject::IsBalanced());
}

TEST(DequeTest, EmplaceOperations) {
    TestObject::Reset();
    fermat::Deque<TestObject, 64, fermat::TimesPolicy<2, 1>,
        fermat::AlignedAllocator<TestObject, 64>,
        fermat::AlignedAllocator<TestObject *, 64>, 16> d;

    EXPECT_EQ(d.emplace_back(2, 3, 4).value, 2 + 3 + 4);
    EXPECT_EQ(d.size(), 1u);

    d.emplace(d.begin(), 3, 4, 5);
    EXPECT_EQ(d.size(), 2u);
    EXPECT_EQ(d.front().value, 3 + 4 + 5);

    EXPECT_EQ(d.emplace_front(6, 7, 8).value, 6 + 7 + 8);
    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d.front().value, 6 + 7 + 8);
}

TEST(DequeTest, MoveOnlyType) {
    fermat::Deque<MoveOnly> d;
    EXPECT_EQ(d.emplace_back(MoveOnly::Create()).value, 42);
    EXPECT_EQ(d.emplace_front(MoveOnly::Create()).value, 42);

    auto moved = std::move(d);
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(moved.size(), 2u);

    fermat::Deque<MoveOnly> from;
    from.emplace_back(MoveOnly::Create());
    fermat::Deque<MoveOnly> to;
    to = std::move(from);
    EXPECT_TRUE(from.empty());
    EXPECT_FALSE(to.empty());

    d = std::move(to);
    EXPECT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].value, 42);
    d.clear();
    EXPECT_TRUE(d.empty());

    d.emplace(d.begin(), MoveOnly::Create());
    d.emplace(d.begin(), MoveOnly::Create());
    EXPECT_EQ(d.size(), 2u);

    d.pop_back();
    EXPECT_EQ(d.size(), 1u);

    {
        fermat::Deque<MoveOnly> other;
        other.emplace_front(MoveOnly::Create());
        other.emplace_front(MoveOnly::Create());
        other.emplace_front(MoveOnly::Create());
        other.swap(d);
        EXPECT_EQ(d.size(), 3u);
        EXPECT_EQ(other.size(), 1u);
    }

    d.pop_front();
    EXPECT_EQ(d.size(), 2u);

    d.insert(d.end(), MoveOnly::Create());
    EXPECT_EQ(d.size(), 3u);
}
/*
TEST(DequeTest, SetAllocatorOnEmptyDeque) {
    fermat::Deque<int> d;
    EXPECT_TRUE(d.empty());
    d.set_allocator(fermat::AlignedAllocator<int, 64>());
    EXPECT_TRUE(d.empty());
    EXPECT_TRUE(d.validate());
}
*/
TEST(DequeRegressionTest, PtrArrayRecenter) {
    fermat::Deque<int> x;
    fermat::Deque<int> y;

    for (int i = 0; i < 1001; ++i) {
        for (int j = 0; j < 5; ++j) {
            x.push_back(0);
        }
        x.swap(y);
        while (!x.empty()) {
            x.pop_front();
        }
    }

    EXPECT_TRUE(x.empty());
    EXPECT_TRUE(x.validate());
    EXPECT_TRUE(y.validate());
}

TEST(DequeRegressionTest, StringCopyAssign) {
    fermat::Deque<fermat::KString> testArray;
    testArray.push_back(fermat::KString("a"));

    for (int j = 0; j < 65; ++j) {
        testArray.push_back(fermat::KString("a"));
    }

    fermat::Deque<fermat::KString> temp;
    temp = testArray;
    EXPECT_EQ(temp.size(), testArray.size());
    for (size_t i = 0; i < testArray.size(); ++i) {
        EXPECT_EQ(temp[i], testArray[i]);
    }
}

TEST(DequeTest, ConstDeque) {
    const fermat::Deque<int> empty;
    EXPECT_TRUE(empty.empty());

    int arr[3] = {37, 38, 39};
    const fermat::Deque<int> fromRange(arr, arr + 3);
    EXPECT_EQ(fromRange.size(), 3u);

    const fermat::Deque<int> sized(4, 37);
    EXPECT_EQ(sized.size(), 4u);

    const fermat::Deque<int> copied = sized;
    EXPECT_EQ(copied.size(), 4u);
}

TEST(DequeTest, ShrinkToFit) {
    fermat::Deque<int> d(4096, 7);
    d.erase(d.begin(), d.end());
    EXPECT_TRUE(d.empty());
    d.shrink_to_fit();
    EXPECT_TRUE(d.empty());
    EXPECT_TRUE(d.validate());
}

TEST(DequeTest, UniquePtrEmplaceAndErase) {
    struct Holder {
        explicit Holder(int v) : ptr(new int(v)) {
        }

        std::unique_ptr<int> ptr;
    };

    fermat::Deque<Holder> d;
    EXPECT_EQ(*d.emplace_back(1).ptr, 1);
    EXPECT_EQ(*d.emplace_back(2).ptr, 2);
    EXPECT_EQ(*d.emplace_back(3).ptr, 3);
    d.erase(d.begin() + 1);
    EXPECT_EQ(d.size(), 2u);
    EXPECT_EQ(*d[0].ptr, 1);
    EXPECT_EQ(*d[1].ptr, 3);
}

TEST(DequeTest, EraseValue) {
    fermat::Deque<int> d = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    EXPECT_EQ(fermat::erase(d, 2), 1u);
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{1, 3, 4, 5, 6, 7, 8, 9}));

    EXPECT_EQ(fermat::erase(d, 7), 1u);
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{1, 3, 4, 5, 6, 8, 9}));

    EXPECT_EQ(fermat::erase(d, 9), 1u);
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{1, 3, 4, 5, 6, 8}));

    EXPECT_EQ(fermat::erase(d, 5), 1u);
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{1, 3, 4, 6, 8}));

    EXPECT_EQ(fermat::erase(d, 3), 1u);
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{1, 4, 6, 8}));
}

TEST(DequeTest, EraseIf) {
    fermat::Deque<int> d = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    EXPECT_EQ(fermat::erase_if(d, [](int i) { return i % 2 == 0; }), 4u);
    EXPECT_TRUE(VerifySequence(d, std::vector<int>{1, 3, 5, 7, 9}));
}

TEST(DequeTest, EraseUnsorted) {
    {
        fermat::Deque<int> d = {0, 1, 2, 3};
        EXPECT_EQ(fermat::erase_unsorted(d, 1), 1u);
        std::vector<int> sorted = {d.begin(), d.end()};
        std::sort(sorted.begin(), sorted.end());
        EXPECT_EQ(sorted, (std::vector<int>{0, 2, 3}));
    }
    {
        fermat::Deque<int> d;
        EXPECT_EQ(fermat::erase_unsorted(d, 42), 0u);
        EXPECT_TRUE(d.empty());
    }
    {
        fermat::Deque<int> d = {0, 1, 2, 3, 1, 5, 6, 1, 8, 9};
        EXPECT_EQ(fermat::erase_unsorted(d, 1), 3u);
        std::vector<int> sorted = {d.begin(), d.end()};
        std::sort(sorted.begin(), sorted.end());
        EXPECT_EQ(sorted, (std::vector<int>{0, 2, 3, 5, 6, 8, 9}));
    }
}

TEST(DequeTest, EraseUnsortedIf) {
    {
        fermat::Deque<int> d = {0, 1, 2, 3};
        EXPECT_EQ(fermat::erase_unsorted_if(d, [](int v) { return v % 2 == 1; }), 2u);
        EXPECT_TRUE(VerifySequence(d, std::vector<int>{0, 2}));
    }
    {
        fermat::Deque<int> d;
        EXPECT_EQ(fermat::erase_unsorted_if(d, [](int v) { return v % 2 == 1; }), 0u);
        EXPECT_TRUE(d.empty());
    }
    {
        fermat::Deque<int> d = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        EXPECT_EQ(fermat::erase_unsorted_if(d, [](int v) { return v % 2 == 1; }), 5u);
        std::vector<int> sorted = {d.begin(), d.end()};
        std::sort(sorted.begin(), sorted.end());
        EXPECT_EQ(sorted, (std::vector<int>{0, 2, 4, 6, 8}));
    }
}

TEST(DequeCompareTest, RelationalOperators) {
    fermat::Deque<int> d1 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    fermat::Deque<int> d2 = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    fermat::Deque<int> d3 = {1, 2, 3, 4, 5};
    fermat::Deque<int> d4 = {10};

    EXPECT_NE(d1, d2);
    EXPECT_LT(d1, d2);
    EXPECT_NE(d1, d3);
    EXPECT_GT(d1, d3);
    EXPECT_GT(d4, d1);
    EXPECT_GT(d4, d2);
    EXPECT_GT(d4, d3);
}

TEST(DequeCompareTest, Equality) {
    fermat::Deque<int> d1 = {1, 2, 3};
    fermat::Deque<int> d2 = {1, 2, 3};
    fermat::Deque<int> d3 = {1, 2, 4};
    EXPECT_TRUE(d1 == d2);
    EXPECT_FALSE(d1 == d3);
    EXPECT_TRUE(d1 != d3);
}
