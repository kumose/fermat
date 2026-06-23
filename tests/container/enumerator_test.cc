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

#include <array>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include <gtest/gtest.h>
#include <fermat/container/enumerate.h>

namespace fermat {
    namespace {
        /// Helper to detect const reference.
        template<class T>
        struct IsConstReference {
            constexpr static bool value = false;
        };

        template<class T>
        struct IsConstReference<const T &> {
            constexpr static bool value = true;
        };

        /// Helper for constexpr sum using iterator style.
        constexpr int BasicSum(const std::array<int, 3> &test) {
            int sum = 0;
            for (auto it: enumerate(test)) {
                sum += *it;
            }
            return sum;
        }

        /// Helper for constexpr sum using structured bindings.
        constexpr int StructuredBindingSum(const std::array<int, 3> &test) {
            int sum = 0;
            for (auto &&[idx, val]: enumerate(test)) {
                (void) idx;
                sum += val;
            }
            return sum;
        }

        /// Basic test with mutable vector.
        TEST(Enumerate, Basic) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (auto it: enumerate(v)) {
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());

                // Test mutability.
                std::string newValue = "x";
                *it = newValue;
                EXPECT_EQ(newValue, v[i]);

                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Basic test with rvalue reference iteration.
        TEST(Enumerate, BasicRRef) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (auto &&it: enumerate(v)) {
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());

                std::string newValue = "x";
                *it = newValue;
                EXPECT_EQ(newValue, v[i]);

                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test const iteration (by value).
        TEST(Enumerate, BasicConst) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (const auto it: enumerate(v)) {
                static_assert(IsConstReference<decltype(*it)>::value,
                              "Const enumeration should yield const references");
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test const iteration (by const lvalue reference).
        TEST(Enumerate, BasicConstRef) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (const auto &it: enumerate(v)) {
                static_assert(IsConstReference<decltype(*it)>::value,
                              "Const enumeration should yield const references");
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test const iteration (by const rvalue reference).
        TEST(Enumerate, BasicConstRRef) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (const auto &&it: enumerate(v)) {
                static_assert(IsConstReference<decltype(*it)>::value,
                              "Const enumeration should yield const references");
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test with std::vector<bool> (proxy iterators).
        TEST(Enumerate, BasicVecBool) {
            std::vector<bool> v = {true, false, false, true};
            size_t i = 0;
            for (auto it: enumerate(v)) {
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test with temporary container (copy).
        TEST(Enumerate, Temporary) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (auto &&it: enumerate(decltype(v)(v))) {
                // Copy v.
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test with const container argument.
        TEST(Enumerate, BasicConstArg) {
            const std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (auto &&it: enumerate(v)) {
                static_assert(IsConstReference<decltype(*it)>::value,
                              "Enumerating a const vector should yield const references");
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test temporary const enumeration.
        TEST(Enumerate, TemporaryConstEnumerate) {
            std::vector<std::string> v = {"abc", "a", "ab"};
            size_t i = 0;
            for (const auto &&it: enumerate(decltype(v)(v))) {
                static_assert(IsConstReference<decltype(*it)>::value,
                              "Const enumeration should yield const references");
                EXPECT_EQ(it.index, i);
                EXPECT_EQ(*it, v[i]);
                EXPECT_EQ(it->size(), v[i].size());
                ++i;
            }
            EXPECT_EQ(i, v.size());
        }

        /// Test with a custom range that defines begin() and a sentinel.
        /// (Assumes fermat::enumerate supports sentinel-based ranges; if not, this test may be skipped.)
        class CStringRange {
            const char *cstr_;

        public:
            struct Sentinel {
            };

            explicit CStringRange(const char *cstr) : cstr_(cstr) {
            }

            const char *begin() const { return cstr_; }
            Sentinel end() const { return Sentinel{}; }
        };

        bool operator==(const char *c, CStringRange::Sentinel) {
            return *c == 0;
        }

        TEST(Enumerate, SentinelSupport) {
            std::array<char, 5> test = {"test"};
            size_t i = 0;
            for (const auto &&it: enumerate(CStringRange{test.data()})) {
                EXPECT_LT(i, test.size());
                EXPECT_EQ(*it, test[i]);
                ++i;
            }
            EXPECT_EQ(i, 4); // "test" has 4 characters
        }

        /// Test empty container.
        TEST(Enumerate, EmptyRange) {
            std::vector<int> empty;
            for (auto &&it: enumerate(empty)) {
                (void) it;
                ADD_FAILURE() << "Loop should not execute";
            }
        }

        /// Test C++17 structured binding with const lvalue reference.
        TEST(Enumerate, Cpp17StructuredBindingConstRef) {
            std::vector<std::string> test = {"abc", "a", "ab"};
            for (const auto &[idx, str]: enumerate(test)) {
                ASSERT_LT(idx, test.size());
                EXPECT_EQ(str, test[idx]);
            }
        }

        /// Test C++17 structured binding with const rvalue reference.
        TEST(Enumerate, Cpp17StructuredBindingConstRRef) {
            std::vector<std::string> test = {"abc", "a", "ab"};
            for (const auto &&[idx, str]: enumerate(test)) {
                ASSERT_LT(idx, test.size());
                EXPECT_EQ(str, test[idx]);
            }
        }

        /// Test C++17 structured binding with const vector.
        TEST(Enumerate, Cpp17StructuredBindingConstVector) {
            const std::vector<std::string> test = {"abc", "a", "ab"};
            for (auto &&[idx, str]: enumerate(test)) {
                static_assert(IsConstReference<decltype(str)>::value,
                              "Enumerating const vector should yield const references");
                ASSERT_LT(idx, test.size());
                EXPECT_EQ(str, test[idx]);
            }
        }

        /// Test C++17 structured binding with modification.
        TEST(Enumerate, Cpp17StructuredBindingModify) {
            std::vector<int> test = {1, 2, 3, 4, 5};
            for (auto &&[idx, val]: enumerate(test)) {
                (void) idx;
                val = 0;
            }
            for (const auto &val: test) {
                EXPECT_EQ(val, 0);
            }
        }

        /// Test constexpr iterator-style enumerate.
        TEST(Enumerate, BasicConstexpr) {
            constexpr std::array<int, 3> test = {1, 2, 3};
            static_assert(BasicSum(test) == 6, "Iterator-style enumerate not constexpr");
            EXPECT_EQ(BasicSum(test), 6);
        }

        /// Test constexpr structured binding enumerate.
        TEST(Enumerate, Cpp17StructuredBindingConstexpr) {
            constexpr std::array<int, 3> test = {1, 2, 3};
            static_assert(StructuredBindingSum(test) == 6,
                          "Structured binding enumerate not constexpr");
            EXPECT_EQ(StructuredBindingSum(test), 6);
        }
    } // namespace
} // namespace fermat
