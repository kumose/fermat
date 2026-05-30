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

#include <fermat/container/expected.h>
#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using fermat::expected;
using fermat::unexpected;
using fermat::unexpect;

enum class TestError {
    Error1,
    Error2,
    Error3,
};

struct Point {
    Point(int x, int y) noexcept : mX{x}, mY{y} {}
    int mX{};
    int mY{};
};

struct SomeClass {
    int mI{};
};

struct ConversionTest {
    explicit ConversionTest(const SomeClass &s) : mI{s.mI} {}
    int mI{};
};

struct DefaultConstructible {
    static constexpr int DefaultID = 10;
    DefaultConstructible() : mId(DefaultID) {}
    int mId;
};

struct NoDefaultConstructible {
    explicit NoDefaultConstructible(int i) : mId(i) {}
    int mId;
};

struct NoCopyConstructible {
    NoCopyConstructible() = default;
    NoCopyConstructible(const NoCopyConstructible &) = delete;
    NoCopyConstructible(NoCopyConstructible &&) = default;
};

struct CopyNoMove {
    CopyNoMove() = default;
    CopyNoMove(const CopyNoMove &) = default;
    CopyNoMove(CopyNoMove &&) = delete;
};

struct MoveNoCopy {
    MoveNoCopy() = default;
    MoveNoCopy(const MoveNoCopy &) = delete;
    MoveNoCopy(MoveNoCopy &&) = default;
};

struct NoTriviallyCopyable {
    NoTriviallyCopyable() = default;
    explicit NoTriviallyCopyable(int i) : mId{i} {}
    NoTriviallyCopyable(const NoTriviallyCopyable &other) { mId = other.mId; }
    int mId{};
};

struct NoTriviallyDestructible {
    ~NoTriviallyDestructible() { ++mId; }
    int mId{};
};

struct NoTriviallyCopyableNoDefaultConstructible {
    NoTriviallyCopyableNoDefaultConstructible() = delete;
    explicit NoTriviallyCopyableNoDefaultConstructible(int i) : mId(i) {}
    NoTriviallyCopyableNoDefaultConstructible(const NoTriviallyCopyableNoDefaultConstructible &other) {
        mId = other.mId;
    }
    int mId{};
};

struct NoImplicitIntConversion {
    explicit NoImplicitIntConversion(int i) : mId(i) {}
    int mId;
};

struct ImplicitIntConversion {
    ImplicitIntConversion(int i) : mId(i) {}
    int mId;
};

struct CopyAssignableNoMoveAssignable {
    CopyAssignableNoMoveAssignable() = default;
    CopyAssignableNoMoveAssignable(const CopyAssignableNoMoveAssignable &) = default;
    CopyAssignableNoMoveAssignable(CopyAssignableNoMoveAssignable &&) = delete;
    CopyAssignableNoMoveAssignable &operator=(const CopyAssignableNoMoveAssignable &) = default;
    CopyAssignableNoMoveAssignable &operator=(CopyAssignableNoMoveAssignable &&) = delete;
};

struct ClearOnMove {
    ClearOnMove() = default;
    explicit ClearOnMove(int i) : mId{i} {}
    ClearOnMove(const ClearOnMove &) = default;
    ClearOnMove(ClearOnMove &&other) noexcept : mId{other.mId} { other.mId = 0; }
    ClearOnMove &operator=(const ClearOnMove &) = default;
    ClearOnMove &operator=(ClearOnMove &&other) noexcept {
        mId = other.mId;
        other.mId = 0;
        return *this;
    }
    int mId;
};

template<class T>
struct InitListTest {
    InitListTest(std::initializer_list<T> il) noexcept : mVec(il) {}
    std::vector<T> mVec;
};

template<class T, class Value, class Error, template<class> class... Rest>
struct ExpectedTypeTraitsChecker : std::true_type {};

template<class T, class Value, class Error, template<class> class Trait, template<class> class... Rest>
struct ExpectedTypeTraitsChecker<T, Value, Error, Trait, Rest...> {
    static_assert(Trait<T>::value == Trait<expected<Value, Error>>::value);
    static constexpr bool value = (Trait<T>::value == Trait<expected<Value, Error>>::value) &&
                                  ExpectedTypeTraitsChecker<T, Value, Error, Rest...>::value;
};

template<class T>
constexpr bool CheckExpectedValueTypeTraits = ExpectedTypeTraitsChecker<
    T, T, int,
    std::is_default_constructible,
    std::is_copy_constructible,
    std::is_move_constructible,
    std::is_trivially_copy_constructible,
    std::is_trivially_move_constructible,
    std::is_trivially_destructible>::value;

template<class T>
constexpr bool CheckExpectedErrorTypeTraits =
    ExpectedTypeTraitsChecker<
        T, int, T,
        std::is_copy_constructible,
        std::is_move_constructible,
        std::is_trivially_copy_constructible,
        std::is_trivially_move_constructible,
        std::is_trivially_destructible>::value &&
    ExpectedTypeTraitsChecker<int, int, T, std::is_default_constructible>::value;

template<class T>
constexpr bool CheckExpectedTypeTraits = CheckExpectedValueTypeTraits<T> && CheckExpectedErrorTypeTraits<T>;

template<class T>
constexpr bool CheckExpectedVoidTypeTraits =
    ExpectedTypeTraitsChecker<
        T, void, T,
        std::is_copy_constructible,
        std::is_move_constructible,
        std::is_trivially_copy_constructible,
        std::is_trivially_move_constructible,
        std::is_trivially_destructible>::value &&
    ExpectedTypeTraitsChecker<int, void, T, std::is_default_constructible>::value;

// ============================================================================
// unexpected
// ============================================================================

TEST(UnexpectedTest, ConstructionAndAccess) {
    unexpected<int> u(1);
    EXPECT_EQ(u.error(), 1);
    unexpected<float> v(2.0f);
    EXPECT_FLOAT_EQ(v.error(), 2.0f);
    unexpected<TestError> w(TestError::Error1);
    EXPECT_EQ(w.error(), TestError::Error1);
    w.error() = TestError::Error2;
    EXPECT_EQ(w.error(), TestError::Error2);
    unexpected<Point> x(Point{1, 2});
    EXPECT_EQ(x.error().mX, 1);
    EXPECT_EQ(x.error().mY, 2);
}

TEST(UnexpectedTest, InPlaceConstruction) {
    unexpected<Point> u(std::in_place, 1, 2);
    EXPECT_EQ(u.error().mX, 1);
    EXPECT_EQ(u.error().mY, 2);
}

TEST(UnexpectedTest, InitializerListConstruction) {
    unexpected<InitListTest<int>> u(std::in_place, {1, 2, 3, 4});
    EXPECT_EQ(u.error().mVec, (std::vector<int>{1, 2, 3, 4}));
}

TEST(UnexpectedTest, Swap) {
    unexpected<int> u(1);
    unexpected<int> v(2);
    swap(u, v);
    EXPECT_EQ(u.error(), 2);
    EXPECT_EQ(v.error(), 1);
}

TEST(UnexpectedTest, Equality) {
    unexpected<int> u(1);
    unexpected<int> v(2);
    unexpected<int> w(2);
    EXPECT_FALSE(u == v);
    EXPECT_TRUE(w == v);
}

TEST(UnexpectedTest, CTAD) {
    unexpected unex(42);
    static_assert(std::is_same_v<decltype(unex), unexpected<int>>);
    EXPECT_EQ(unex.error(), 42);
}

// ============================================================================
// Type traits
// ============================================================================

TEST(ExpectedTypeTraitsTest, ValueAndErrorTraits) {
    static_assert(CheckExpectedTypeTraits<int>);
    static_assert(CheckExpectedTypeTraits<DefaultConstructible>);
    static_assert(CheckExpectedTypeTraits<NoDefaultConstructible>);
    static_assert(CheckExpectedTypeTraits<NoCopyConstructible>);
    static_assert(CheckExpectedTypeTraits<NoTriviallyCopyable>);
    static_assert(CheckExpectedTypeTraits<NoTriviallyDestructible>);
    static_assert(CheckExpectedTypeTraits<NoTriviallyCopyableNoDefaultConstructible>);
    static_assert(CheckExpectedTypeTraits<MoveNoCopy>);
    static_assert(CheckExpectedTypeTraits<std::vector<int>>);
    static_assert(CheckExpectedTypeTraits<std::unique_ptr<int>>);

    static_assert(std::is_copy_constructible_v<expected<CopyNoMove, int>>);
    static_assert(std::is_move_constructible_v<expected<CopyNoMove, int>>);
    static_assert(std::is_copy_constructible_v<expected<int, CopyNoMove>>);
    static_assert(std::is_move_constructible_v<expected<int, CopyNoMove>>);

    static_assert(std::is_copy_assignable_v<expected<CopyAssignableNoMoveAssignable, int>>);
    static_assert(std::is_move_assignable_v<expected<CopyAssignableNoMoveAssignable, int>>);
    static_assert(std::is_copy_assignable_v<expected<int, CopyAssignableNoMoveAssignable>>);
    static_assert(std::is_move_assignable_v<expected<int, CopyAssignableNoMoveAssignable>>);

    static_assert(CheckExpectedVoidTypeTraits<int>);
    static_assert(CheckExpectedVoidTypeTraits<NoDefaultConstructible>);
    static_assert(CheckExpectedVoidTypeTraits<NoCopyConstructible>);
    static_assert(CheckExpectedVoidTypeTraits<NoTriviallyCopyable>);
    static_assert(CheckExpectedVoidTypeTraits<NoTriviallyDestructible>);
    static_assert(CheckExpectedVoidTypeTraits<NoTriviallyCopyableNoDefaultConstructible>);
    static_assert(CheckExpectedVoidTypeTraits<MoveNoCopy>);
    static_assert(CheckExpectedVoidTypeTraits<std::vector<int>>);
    static_assert(CheckExpectedVoidTypeTraits<std::unique_ptr<int>>);
    static_assert(std::is_copy_constructible_v<expected<void, CopyNoMove>>);
    static_assert(std::is_move_constructible_v<expected<void, CopyNoMove>>);
}

// ============================================================================
// expected<T, E> construction / copy / move
// ============================================================================

TEST(ExpectedGenericTest, DefaultConstructionAndCopyAssign) {
    expected<int, TestError> e;
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value(), 0);

    e.value() = 42;
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value(), 42);

    expected<int, TestError> e1{e};
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(e1.value(), 42);

    expected<int, TestError> e2{unexpect, TestError::Error2};
    e1 = e2;
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error(), TestError::Error2);

    e2 = e;
    EXPECT_TRUE(e2.has_value());
    EXPECT_EQ(e2.value(), 42);
}

TEST(ExpectedGenericTest, DefaultConstructibleValue) {
    expected<DefaultConstructible, TestError> e;
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e->mId, DefaultConstructible::DefaultID);
}

TEST(ExpectedGenericTest, NonTrivialCopy) {
    expected<NoTriviallyCopyable, TestError> e{5};
    expected<NoTriviallyCopyable, TestError> e1 = e;
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(e1.value().mId, 5);

    expected<NoTriviallyCopyable, TestError> err{unexpect, TestError::Error3};
    expected<NoTriviallyCopyable, TestError> err1 = err;
    EXPECT_FALSE(err1.has_value());
    EXPECT_EQ(err1.error(), TestError::Error3);
}

TEST(ExpectedGenericTest, NoDefaultConstructibleValue) {
    expected<NoDefaultConstructible, TestError> e{2};
    expected<NoDefaultConstructible, TestError> e1 = e;
    EXPECT_EQ(e1.value().mId, 2);

    expected<NoDefaultConstructible, NoDefaultConstructible> e2{2};
    EXPECT_TRUE(e2.has_value());
    expected<NoDefaultConstructible, NoDefaultConstructible> e3 = e2;
    EXPECT_TRUE(e3.has_value());
    EXPECT_EQ(e3.value().mId, 2);
}

TEST(ExpectedGenericTest, NoTriviallyCopyableNoDefaultConstructible) {
    expected<NoTriviallyCopyableNoDefaultConstructible, NoTriviallyCopyableNoDefaultConstructible> e{2};
    EXPECT_TRUE(e.has_value());
    expected<NoTriviallyCopyableNoDefaultConstructible, NoTriviallyCopyableNoDefaultConstructible> e1 = e;
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(e1.value().mId, 2);

    expected<NoTriviallyCopyableNoDefaultConstructible, TestError> e2{2};
    expected<NoTriviallyCopyableNoDefaultConstructible, TestError> e3 = e2;
    EXPECT_TRUE(e3.has_value());
    EXPECT_EQ(e3.value().mId, 2);
}

TEST(ExpectedGenericTest, NoCopyConstructibleValue) {
    expected<NoCopyConstructible, TestError> e;
    EXPECT_TRUE(e.has_value());
}

TEST(ExpectedGenericTest, VectorCopyAndMove) {
    std::vector<int> v = {1, 2, 3, 4};
    expected<std::vector<int>, TestError> e(v);
    EXPECT_EQ(e.value(), v);
    expected<std::vector<int>, TestError> e1 = e;
    EXPECT_EQ(e1.value(), v);

    expected<std::vector<int>, TestError> e2 = std::move(e);
    EXPECT_EQ(e2.value(), v);

    std::vector<int> v1 = {1, 2, 3, 4, 5, 6};
    expected<std::vector<int>, TestError> e3(v1);
    e2 = e3;
    EXPECT_EQ(e2.value(), v1);

    e2 = std::move(e1);
    EXPECT_EQ(e2.value(), v);
    EXPECT_TRUE(e1.value().empty());
}

TEST(ExpectedGenericTest, ValueConversion) {
    expected<ImplicitIntConversion, int> e = 1;
    EXPECT_TRUE(e.has_value());
    expected<NoImplicitIntConversion, int> e1(1);
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(e1.value().mId, e.value().mId);
}

TEST(ExpectedGenericTest, UniquePtr) {
    expected<std::unique_ptr<int>, TestError> e(std::make_unique<int>(2));
    expected<std::unique_ptr<int>, TestError> e1 = std::move(e);
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(*e1.value(), 2);

    e1 = unexpected<TestError>(TestError::Error2);
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error(), TestError::Error2);

    expected<std::unique_ptr<int>, TestError> e3(std::make_unique<int>(5));
    EXPECT_TRUE(e3.has_value());
    EXPECT_EQ(*e3.value(), 5);
    e3 = std::move(e1);
    EXPECT_FALSE(e3.has_value());
    EXPECT_EQ(e3.error(), TestError::Error2);
}

TEST(ExpectedGenericTest, CrossExpectedConversion) {
    expected<unsigned int, unsigned int> e(1u);
    expected<int, int> e1(e);
    EXPECT_EQ(e1.value(), 1);

    std::vector<int> v = {1, 2, 3, 4};
    expected<std::vector<int>, unsigned int> ev(v);
    expected<std::vector<int>, int> ev1(std::move(ev));
    EXPECT_EQ(ev1.value(), v);
}

TEST(ExpectedGenericTest, UnexpectedConstruction) {
    unexpected<TestError> unex(TestError::Error2);
    expected<int, TestError> e{unex};
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), TestError::Error2);

    std::vector<int> v = {1, 2, 3, 4};
    expected<int, std::vector<int>> e1{unexpected<std::vector<int>>(v)};
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error(), v);

    expected<int, std::vector<int>> e2{std::move(e1)};
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), v);
}

TEST(ExpectedGenericTest, InPlaceConstruction) {
    expected<Point, int> e(std::in_place, 1, 2);
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value().mX, 1);
    EXPECT_EQ(e.value().mY, 2);

    expected<int, Point> err(unexpect, 1, 2);
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(err.error().mX, 1);
    EXPECT_EQ(err.error().mY, 2);

    expected<std::vector<int>, int> ev(std::in_place, {1, 2, 3, 4});
    EXPECT_TRUE(ev.has_value());
    EXPECT_EQ(ev.value(), (std::vector<int>{1, 2, 3, 4}));

    expected<int, std::vector<int>> ev_err(unexpect, {1, 2, 3, 4});
    EXPECT_FALSE(ev_err.has_value());
    EXPECT_EQ(ev_err.error(), (std::vector<int>{1, 2, 3, 4}));
}

TEST(ExpectedGenericTest, AssignValue) {
    expected<std::vector<int>, int> e(std::in_place, {1, 2, 3, 4});
    std::vector<int> v = {1, 2, 3, 5};
    e = v;
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value(), v);

    expected<std::vector<int>, int> e2(unexpect, 1);
    e2 = v;
    EXPECT_TRUE(e2.has_value());
    EXPECT_EQ(e2.value(), v);
}

TEST(ExpectedGenericTest, AssignUnexpected) {
    expected<std::vector<int>, int> e(std::in_place, {1, 2, 3, 4});
    unexpected<int> u{2};
    e = u;
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), u.error());

    expected<std::vector<int>, int> e2(unexpect, 1);
    e2 = u;
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), u.error());
}

TEST(ExpectedGenericTest, AssignScalarAndUnexpectedEnum) {
    expected<int, TestError> e(1);
    e = 3;
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value(), 3);

    expected<unsigned int, TestError> eu(1u);
    eu = 3;
    EXPECT_TRUE(eu.has_value());
    EXPECT_EQ(eu.value(), 3u);

    eu = unexpected<TestError>(TestError::Error3);
    EXPECT_FALSE(eu.has_value());
    EXPECT_EQ(eu.error(), TestError::Error3);

    auto unex = unexpected<TestError>(TestError::Error2);
    eu = unex;
    EXPECT_FALSE(eu.has_value());
    EXPECT_EQ(eu.error(), TestError::Error2);
}

TEST(ExpectedGenericTest, Emplace) {
    expected<Point, TestError> e(std::in_place, 1, 2);
    e.emplace(3, 4);
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value().mX, 3);
    EXPECT_EQ(e.value().mY, 4);
}

TEST(ExpectedGenericTest, InitListInPlace) {
    expected<InitListTest<int>, TestError> e(std::in_place, {1, 2});
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(e.value().mVec, (std::vector<int>{1, 2}));
}

TEST(ExpectedGenericTest, SwapValueValue) {
    expected<int, TestError> e1(1);
    expected<int, TestError> e2(2);
    e1.swap(e2);
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(e1.value(), 2);
    EXPECT_TRUE(e2.has_value());
    EXPECT_EQ(e2.value(), 1);

    using std::swap;
    swap(e1, e2);
    EXPECT_EQ(e1.value(), 1);
    EXPECT_EQ(e2.value(), 2);
}

TEST(ExpectedGenericTest, SwapValueError) {
    expected<int, TestError> e1(1);
    expected<int, TestError> e2{unexpected<TestError>(TestError::Error1)};
    e1.swap(e2);
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error(), TestError::Error1);
    EXPECT_TRUE(e2.has_value());
    EXPECT_EQ(e2.value(), 1);

    using std::swap;
    swap(e1, e2);
    EXPECT_TRUE(e1.has_value());
    EXPECT_EQ(e1.value(), 1);
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), TestError::Error1);
}

TEST(ExpectedGenericTest, SwapVectorMixed) {
    std::vector<int> v = {1, 2, 3, 4, 5};
    expected<int, std::vector<int>> e1{3};
    expected<int, std::vector<int>> e2{unexpect, v};
    using std::swap;
    swap(e1, e2);
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error(), v);
    EXPECT_TRUE(e2.has_value());
    EXPECT_EQ(e2.value(), 3);

    expected<std::vector<int>, int> e3{v};
    expected<std::vector<int>, int> e4{unexpect, 5};
    swap(e3, e4);
    EXPECT_FALSE(e3.has_value());
    EXPECT_EQ(e3.error(), 5);
    EXPECT_TRUE(e4.has_value());
    EXPECT_EQ(e4.value(), v);
}

TEST(ExpectedGenericTest, ValueOrAndErrorOr) {
    expected<float, TestError> e(2.0f);
    EXPECT_FLOAT_EQ(e.value_or(10.f), 2.0f);
    EXPECT_EQ(e.error_or(TestError::Error2), TestError::Error2);
    e = unexpected{TestError::Error3};
    EXPECT_FLOAT_EQ(e.value_or(10.f), 10.0f);
    EXPECT_FLOAT_EQ(e.value_or(1), 1.0f);
    EXPECT_EQ(e.error_or(TestError::Error2), TestError::Error3);
}

TEST(ExpectedGenericTest, Equality) {
    expected<int, int> e1(1);
    expected<int, int> e2(1);
    EXPECT_TRUE(e1 == e2);
    e2 = 5;
    EXPECT_FALSE(e1 == e2);
    e1 = unexpected{5};
    EXPECT_FALSE(e1 == e2);
    e2 = unexpected{5};
    EXPECT_TRUE(e1 == e2);

    expected<int, int> e3(1);
    expected<float, float> e4(1.0f);
    EXPECT_TRUE(e3 == e4);
    e4 = 5.0f;
    EXPECT_FALSE(e3 == e4);
    e3 = unexpected{1};
    EXPECT_FALSE(e3 == e4);
    e4 = unexpected{1.0f};
    EXPECT_TRUE(e3 == e4);
}

TEST(ExpectedGenericTest, CompareWithValueAndUnexpected) {
    expected<int, TestError> e(10);
    EXPECT_TRUE(e == 10);
    EXPECT_FALSE(e == 11);
    EXPECT_FALSE(e == unexpected<TestError>(TestError::Error1));
    expected<int, TestError> err{unexpect, TestError::Error1};
    EXPECT_TRUE(err == unexpected<TestError>(TestError::Error1));
}

TEST(ExpectedGenericTest, MonadicOperations) {
    const auto addHalf = [](int val) -> expected<float, TestError> {
        return static_cast<float>(val) + 0.5f;
    };
    const auto getVector = [](float val) -> expected<std::vector<int>, TestError> {
        if (val > 10.f) {
            return std::vector<int>{1, 2, 3, 4};
        }
        if (val > 0.0f) {
            return std::vector<int>{1};
        }
        return unexpected{TestError::Error2};
    };
    const auto isBigVector = [](std::vector<int> val) -> expected<bool, TestError> {
        return val.size() > 2;
    };

    {
        expected<int, TestError> e(1);
        auto r1 = e.and_then(addHalf).and_then(getVector).and_then(isBigVector);
        EXPECT_TRUE(r1.has_value());
        EXPECT_FALSE(*r1);
    }
    {
        expected<int, TestError> e(10);
        auto r1 = e.and_then(addHalf).and_then(getVector).and_then(isBigVector);
        EXPECT_TRUE(r1.has_value());
        EXPECT_TRUE(*r1);
    }
    {
        expected<int, TestError> e(-5);
        auto r1 = e.and_then(addHalf).and_then(getVector).and_then(isBigVector);
        EXPECT_FALSE(r1.has_value());
        EXPECT_EQ(r1.error(), TestError::Error2);
    }

    const auto getVectorForError = [](TestError err) -> expected<std::vector<int>, TestError> {
        switch (err) {
            case TestError::Error1: return std::vector<int>{1, 1, 1, 1};
            case TestError::Error2: return std::vector<int>{4, 3, 2, 1};
            case TestError::Error3: return std::vector<int>{0, 1, 2};
        }
        return std::vector<int>{};
    };

    {
        expected<float, TestError> e(1.0f);
        auto r1 = e.and_then(getVector).or_else(getVectorForError);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, (std::vector<int>{1}));
    }
    {
        expected<float, TestError> e(-5.0f);
        auto r1 = e.and_then(getVector).or_else(getVectorForError);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, (std::vector<int>{4, 3, 2, 1}));
    }

    const auto pushBackTen = [](auto val) {
        val.push_back(10);
        return val;
    };
    const auto getSize = [](const std::vector<int> &val) { return val.size(); };

    {
        expected<float, TestError> e(1.0f);
        auto r1 = e.and_then(getVector).transform(pushBackTen).transform(getSize);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, 2u);
    }
    {
        expected<float, TestError> e(-5.0f);
        auto r1 = e.and_then(getVector).transform(pushBackTen).transform(getSize);
        EXPECT_FALSE(r1.has_value());
        EXPECT_EQ(r1.error(), TestError::Error2);
    }
    {
        expected<float, TestError> e(-5.0f);
        auto r1 = e.and_then(getVector).or_else(getVectorForError).transform(getSize);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, 4u);
    }
    {
        expected<float, TestError> e(1.0f);
        auto r1 = e.and_then(getVector).or_else(getVectorForError).transform(getSize);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, 1u);
    }

    const auto cycleError = [](TestError err) {
        switch (err) {
            case TestError::Error1: return TestError::Error2;
            case TestError::Error2: return TestError::Error3;
            case TestError::Error3: return TestError::Error1;
        }
        return TestError::Error1;
    };

    {
        expected<float, TestError> e(-5.0f);
        auto r1 = e.and_then(getVector).transform_error(cycleError).or_else(getVectorForError).transform(getSize);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, 3u);
    }
    {
        expected<float, TestError> e(1.0f);
        auto r1 = e.and_then(getVector).transform_error(cycleError).or_else(getVectorForError).transform(getSize);
        EXPECT_TRUE(r1.has_value());
        EXPECT_EQ(*r1, 1u);
    }
}

// ============================================================================
// expected<void, E>
// ============================================================================

TEST(ExpectedVoidTest, DefaultAndCopy) {
    expected<void, TestError> e;
    EXPECT_TRUE(e.has_value());

    expected<void, NoDefaultConstructible> e1;
    EXPECT_TRUE(e1.has_value());

    expected<void, TestError> e2{e};
    EXPECT_TRUE(e2.has_value());
}

TEST(ExpectedVoidTest, UnexpectedConstruction) {
    unexpected<SomeClass> unex(SomeClass{3});
    expected<void, SomeClass> e{unex};
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error().mI, 3);

    expected<void, ConversionTest> e1{e};
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error().mI, 3);

    expected<void, ConversionTest> e2{std::move(e)};
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error().mI, 3);

    unexpected<SomeClass> unex2{SomeClass{4}};
    expected<void, ConversionTest> e3{unex2};
    EXPECT_FALSE(e3.has_value());
    EXPECT_EQ(e3.error().mI, 4);

    expected<void, ConversionTest> e4{std::move(unex2)};
    EXPECT_FALSE(e4.has_value());
    EXPECT_EQ(e4.error().mI, 4);
}

TEST(ExpectedVoidTest, NonTrivialErrorCopy) {
    expected<void, NoTriviallyCopyable> e{unexpect, 4};
    expected<void, NoTriviallyCopyable> e1 = e;
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error().mId, 4);
}

TEST(ExpectedVoidTest, VectorCopyAndMove) {
    std::vector<int> v = {1, 2, 3, 4};
    expected<void, std::vector<int>> e(unexpect, v);
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), v);

    expected<void, std::vector<int>> e1 = e;
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(e1.error(), v);

    expected<void, std::vector<int>> e2 = std::move(e);
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), v);

    std::vector<int> v1 = {1, 2, 3, 4, 5, 6};
    expected<void, std::vector<int>> e3(unexpect, v1);
    e2 = e3;
    EXPECT_EQ(e2.error(), v1);

    e2 = std::move(e1);
    EXPECT_EQ(e2.error(), v);
    EXPECT_TRUE(e1.error().empty());
}

TEST(ExpectedVoidTest, UniquePtrError) {
    expected<void, std::unique_ptr<int>> e(unexpect, std::make_unique<int>(2));
    expected<void, std::unique_ptr<int>> e1 = std::move(e);
    EXPECT_FALSE(e1.has_value());
    EXPECT_EQ(*e1.error(), 2);

    e = std::move(e1);
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(*e.error(), 2);

    expected<void, std::unique_ptr<int>> e2;
    EXPECT_TRUE(e2.has_value());
    e2 = std::move(e);
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(*e2.error(), 2);

    expected<void, std::unique_ptr<int>> e3;
    e2 = std::move(e3);
    EXPECT_TRUE(e2.has_value());
}

TEST(ExpectedVoidTest, InPlaceAndAssign) {
    expected<void, Point> e(unexpect, 1, 2);
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error().mX, 1);
    EXPECT_EQ(e.error().mY, 2);

    expected<void, std::vector<int>> ev(unexpect, {1, 2, 3, 4});
    EXPECT_EQ(ev.error(), (std::vector<int>{1, 2, 3, 4}));

    expected<void, TestError> ok;
    EXPECT_TRUE(ok.has_value());
    ok = unexpected<TestError>(TestError::Error3);
    EXPECT_FALSE(ok.has_value());
    EXPECT_EQ(ok.error(), TestError::Error3);

    auto unex = unexpected<TestError>(TestError::Error2);
    ok = unex;
    EXPECT_FALSE(ok.has_value());
    EXPECT_EQ(ok.error(), TestError::Error2);
}

TEST(ExpectedVoidTest, CrossConversion) {
    expected<void, unsigned int> e(unexpect, 1u);
    expected<void, int> e1(e);
    EXPECT_EQ(e1.error(), 1);

    std::vector<int> v = {1, 2, 3, 4};
    expected<void, std::vector<int>> ev{unexpected<std::vector<int>>(v)};
    expected<void, std::vector<int>> ev1{std::move(ev)};
    EXPECT_EQ(ev1.error(), v);
    EXPECT_TRUE(ev.error().empty());
}

TEST(ExpectedVoidTest, Swap) {
    expected<void, TestError> e1;
    expected<void, TestError> e2;
    e1.swap(e2);
    EXPECT_TRUE(e1.has_value());
    EXPECT_TRUE(e2.has_value());

    expected<void, TestError> e3;
    expected<void, TestError> e4{unexpected<TestError>(TestError::Error1)};
    e3.swap(e4);
    EXPECT_FALSE(e3.has_value());
    EXPECT_EQ(e3.error(), TestError::Error1);
    EXPECT_TRUE(e4.has_value());

    using std::swap;
    expected<void, TestError> e5;
    expected<void, TestError> e6{unexpect, TestError::Error1};
    swap(e5, e6);
    EXPECT_FALSE(e5.has_value());
    EXPECT_EQ(e5.error(), TestError::Error1);
    EXPECT_TRUE(e6.has_value());

    std::vector<int> v = {1, 2, 3, 4, 5};
    expected<void, std::vector<int>> e7;
    expected<void, std::vector<int>> e8{unexpect, v};
    swap(e7, e8);
    EXPECT_FALSE(e7.has_value());
    EXPECT_EQ(e7.error(), v);
    EXPECT_TRUE(e8.has_value());
}

TEST(ExpectedVoidTest, ErrorOrAndEquality) {
    expected<void, TestError> e;
    EXPECT_EQ(e.error_or(TestError::Error2), TestError::Error2);
    e = unexpected<TestError>{TestError::Error3};
    EXPECT_EQ(e.error_or(TestError::Error2), TestError::Error3);

    expected<void, int> e1;
    expected<void, int> e2;
    EXPECT_TRUE(e1 == e2);
    e2 = unexpected<int>{5};
    EXPECT_FALSE(e1 == e2);
    e1 = unexpected<int>{4};
    EXPECT_FALSE(e1 == e2);
    e2 = unexpected<int>{4};
    EXPECT_TRUE(e1 == e2);

    expected<void, int> e3;
    expected<void, float> e4;
    EXPECT_TRUE(e3 == e4);
    e4 = unexpected<float>{5.0f};
    EXPECT_FALSE(e3 == e4);
    e3 = unexpected<int>{1};
    EXPECT_FALSE(e3 == e4);
    e4 = unexpected<float>{1.0f};
    EXPECT_TRUE(e3 == e4);
}

TEST(ExpectedVoidTest, MonadicOperations) {
    int counter = 0;
    const auto foo = [&counter]() -> expected<void, TestError> {
        ++counter;
        return {};
    };

    {
        expected<void, TestError> e;
        auto e1 = e.and_then(foo);
        EXPECT_EQ(counter, 1);
        EXPECT_TRUE(e1.has_value());
        counter = 0;
    }
    {
        expected<void, TestError> e{unexpect, TestError::Error2};
        auto e1 = e.and_then(foo);
        EXPECT_EQ(counter, 0);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(e1.error(), TestError::Error2);
        counter = 0;
    }

    const auto fooError = [&counter](TestError t) -> expected<void, TestError> {
        ++counter;
        switch (t) {
            case TestError::Error1: return unexpected<TestError>(TestError::Error1);
            case TestError::Error2: return {};
            case TestError::Error3: return unexpected<TestError>(TestError::Error1);
        }
        return {};
    };

    {
        expected<void, TestError> e;
        auto e1 = e.or_else(fooError);
        EXPECT_EQ(counter, 0);
        EXPECT_TRUE(e1.has_value());
        counter = 0;
    }
    {
        expected<void, TestError> e{unexpect, TestError::Error3};
        auto e1 = e.or_else(fooError);
        EXPECT_EQ(counter, 1);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(e1.error(), TestError::Error1);
        counter = 0;
    }
    {
        expected<void, TestError> e{unexpect, TestError::Error2};
        auto e1 = e.or_else(fooError);
        EXPECT_EQ(counter, 1);
        EXPECT_TRUE(e1.has_value());
        counter = 0;
    }

    const auto getVector = [&counter]() -> std::vector<int> {
        if (counter > 3) {
            return {1, 2, 3, 4};
        }
        return {1, 2};
    };

    {
        expected<void, TestError> e;
        expected<std::vector<int>, TestError> e1 = e.transform(getVector);
        EXPECT_TRUE(e1.has_value());
        EXPECT_EQ(e1.value().size(), 2u);
        counter = 0;
    }
    {
        counter = 5;
        expected<void, TestError> e;
        expected<std::vector<int>, TestError> e1 = e.transform(getVector);
        EXPECT_TRUE(e1.has_value());
        EXPECT_EQ(e1.value().size(), 4u);
        counter = 0;
    }
    {
        expected<void, TestError> e{unexpect, TestError::Error3};
        expected<std::vector<int>, TestError> e1 = e.transform(getVector);
        EXPECT_EQ(counter, 0);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(e1.error(), TestError::Error3);
        counter = 0;
    }

    const auto setCount = [&counter](int i) { counter = i; };

    {
        expected<int, TestError> e{unexpect, TestError::Error3};
        expected<void, TestError> e1 = e.transform(setCount);
        EXPECT_EQ(counter, 0);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(e1.error(), TestError::Error3);
        counter = 0;
    }
    {
        expected<int, TestError> e{5};
        expected<void, TestError> e1 = e.transform(setCount);
        EXPECT_EQ(counter, 5);
        EXPECT_TRUE(e1.has_value());
        counter = 0;
    }

    const auto numberToError = [&counter](int i) -> TestError {
        counter = i;
        if (i <= 1) return TestError::Error1;
        if (i == 2) return TestError::Error2;
        return TestError::Error3;
    };

    {
        expected<void, int> e{unexpect, 5};
        expected<void, TestError> e1 = e.transform_error(numberToError);
        EXPECT_EQ(counter, 5);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(e1.error(), TestError::Error3);
        counter = 0;
    }
    {
        expected<void, int> e{unexpect, 2};
        expected<void, TestError> e1 = e.transform_error(numberToError);
        EXPECT_EQ(counter, 2);
        EXPECT_FALSE(e1.has_value());
        EXPECT_EQ(e1.error(), TestError::Error2);
        counter = 0;
    }
}

TEST(ExpectedVoidTest, ImplicitErrorConversion) {
    expected<void, ImplicitIntConversion> e{unexpect, 1};
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error().mId, 1);
}

} // namespace
