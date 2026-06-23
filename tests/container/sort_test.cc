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
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <cstdint>
#include <cstring>
#include <fermat/container/sort.h>
#include <turbo/synchronization/mutex.h>
namespace fermat {
    namespace {
        // ----------------------------------------------------------------------------
        // Test structure with mKey member for default extract_radix_key.
        // ----------------------------------------------------------------------------
        struct SimpleKey {
            using radix_type = uint32_t;
            radix_type mKey;
            int dummy; ///< Extra data to ensure stability (not used in key).

            bool operator==(const SimpleKey &other) const {
                return mKey == other.mKey && dummy == other.dummy;
            }
        };

        /// Helper to create a vector of SimpleKey with given keys.
        std::vector<SimpleKey> MakeSimpleKeys(const std::vector<uint32_t> &keys) {
            std::vector<SimpleKey> result;
            result.reserve(keys.size());
            for (size_t i = 0; i < keys.size(); ++i) {
                result.push_back({keys[i], static_cast<int>(i)});
            }
            return result;
        }

        // ----------------------------------------------------------------------------
        // Test structure with custom radix type (uint16_t).
        // ----------------------------------------------------------------------------
        struct SmallKey {
            using radix_type = uint16_t;
            radix_type mKey;
            char tag; ///< Additional data.

            bool operator==(const SmallKey &other) const {
                return mKey == other.mKey && tag == other.tag;
            }
        };

        // ----------------------------------------------------------------------------
        // Custom extractor example (for types that don't have mKey).
        // ----------------------------------------------------------------------------
        struct Person {
            uint32_t id{0};
            std::string name;

            bool operator==(const Person& other) const {
                return id == other.id && name == other.name;
            }
        };

        struct ExtractPersonId {
            using radix_type = uint32_t;
            radix_type operator()(const Person &p) const { return p.id; }
        };

        // ----------------------------------------------------------------------------
        // Test fixture: generate random data for reuse.
        // ----------------------------------------------------------------------------
        class RadixSortTest : public ::testing::Test {
        protected:
            void SetUp() override {
                std::mt19937 rng(42); // Fixed seed for reproducibility.
                std::uniform_int_distribution<uint32_t> dist(0, 1000000);
                for (int i = 0; i < 10000; ++i) {
                    random_keys_.push_back(dist(rng));
                }
            }

            std::vector<uint32_t> random_keys_;
        };

        // ----------------------------------------------------------------------------
        /// Basic test: sort vector of SimpleKey using default extractor.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, SortSimpleKeys) {
            std::vector<uint32_t> keys = {5, 1, 4, 2, 8, 0, 3, 7, 6};
            auto vec = MakeSimpleKeys(keys);
            std::vector<SimpleKey> buffer(vec.size());

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());

            std::vector<uint32_t> expected = keys;
            std::sort(expected.begin(), expected.end());
            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, expected[i]);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test stability: elements with same key retain original order.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, Stability) {
            std::vector<SimpleKey> vec;
            for (int i = 0; i < 3; ++i) vec.push_back({1, i});
            for (int i = 3; i < 5; ++i) vec.push_back({2, i});
            auto original = vec;
            std::vector<SimpleKey> buffer(vec.size());

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());

            size_t idx = 0;
            for (int i = 0; i < 3; ++i, ++idx) {
                EXPECT_EQ(vec[idx].mKey, 1);
                EXPECT_EQ(vec[idx].dummy, original[i].dummy);
            }
            for (int i = 3; i < 5; ++i, ++idx) {
                EXPECT_EQ(vec[idx].mKey, 2);
                EXPECT_EQ(vec[idx].dummy, original[i].dummy);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test with custom DigitBits (4 bits per pass).
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, DigitBits4) {
            std::vector<uint32_t> keys = {0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x0000, 0xFFFF};
            auto vec = MakeSimpleKeys(keys);
            std::vector<SimpleKey> buffer(vec.size());

            using Extractor = extract_radix_key<SimpleKey>;
            fermat::radix_sort<decltype(vec.begin()), Extractor, 4>(
                vec.begin(), vec.end(), buffer.begin());

            std::vector<uint32_t> expected = keys;
            std::sort(expected.begin(), expected.end());
            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, expected[i]);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test with 8-bit digit (default) and random data.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, RandomDataDefaultDigitBits) {
            auto vec = MakeSimpleKeys(random_keys_);
            std::vector<SimpleKey> buffer(vec.size());

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());

            auto expected = random_keys_;
            std::sort(expected.begin(), expected.end());
            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, expected[i]);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test with 16-bit digit.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, DigitBits16) {
            auto vec = MakeSimpleKeys(random_keys_);
            std::vector<SimpleKey> buffer(vec.size());

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey>, 16>(
                vec.begin(), vec.end(), buffer.begin());

            auto expected = random_keys_;
            std::sort(expected.begin(), expected.end());
            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, expected[i]);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test with custom key extractor (Person.id).
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, CustomExtractor) {
            std::vector<Person> persons = {
                {42, "Alice"}, {17, "Bob"}, {99, "Charlie"}, {5, "David"}, {17, "Eve"}
            };
            std::vector<Person> buffer(persons.size());
            using Extractor = ExtractPersonId;
            fermat::radix_sort<decltype(persons.begin()), Extractor>(
                persons.begin(), persons.end(), buffer.begin());

            std::vector<Person> expected = {
                {5, "David"}, {17, "Bob"}, {17, "Eve"}, {42, "Alice"}, {99, "Charlie"}
            };
            for (size_t i = 0; i < persons.size(); ++i) {
                EXPECT_EQ(persons[i].id, expected[i].id);
                EXPECT_EQ(persons[i].name, expected[i].name);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test with small integer type (uint16_t).
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, SmallKeyType) {
            std::vector<uint16_t> keys = {5, 1, 4, 2, 8, 0, 3, 7, 6};
            std::vector<SmallKey> vec;
            for (size_t i = 0; i < keys.size(); ++i) {
                vec.push_back({keys[i], static_cast<char>('a' + i)});
            }
            std::vector<SmallKey> buffer(vec.size());
            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SmallKey> >(
                vec.begin(), vec.end(), buffer.begin());

            std::vector<uint16_t> expected = keys;
            std::sort(expected.begin(), expected.end());
            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, expected[i]);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test empty container.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, Empty) {
            std::vector<SimpleKey> vec;
            std::vector<SimpleKey> buffer;
            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());
            SUCCEED();
        }

        // ----------------------------------------------------------------------------
        /// Test single element.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, SingleElement) {
            std::vector<SimpleKey> vec = {{42, 0}};
            std::vector<SimpleKey> buffer(1);
            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());
            EXPECT_EQ(vec[0].mKey, 42);
            EXPECT_EQ(vec[0].dummy, 0);
        }

        // ----------------------------------------------------------------------------
        /// Test already sorted input.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, AlreadySorted) {
            std::vector<uint32_t> keys = {1, 2, 3, 4, 5};
            auto vec = MakeSimpleKeys(keys);
            std::vector<SimpleKey> buffer(vec.size());
            auto original = vec;

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());

            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, original[i].mKey);
                EXPECT_EQ(vec[i].dummy, original[i].dummy);
            }
        }

        // ----------------------------------------------------------------------------
        /// Test reverse sorted input.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, ReverseSorted) {
            std::vector<uint32_t> keys = {5, 4, 3, 2, 1};
            auto vec = MakeSimpleKeys(keys);
            std::vector<SimpleKey> buffer(vec.size());

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());

            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, static_cast<uint32_t>(i + 1));
            }
        }

        // ----------------------------------------------------------------------------
        /// Test all equal keys.
        // ----------------------------------------------------------------------------
        TEST_F(RadixSortTest, AllEqual) {
            std::vector<uint32_t> keys = {42, 42, 42, 42};
            auto vec = MakeSimpleKeys(keys);
            std::vector<SimpleKey> buffer(vec.size());
            auto original = vec;

            fermat::radix_sort<decltype(vec.begin()), extract_radix_key<SimpleKey> >(
                vec.begin(), vec.end(), buffer.begin());

            for (size_t i = 0; i < vec.size(); ++i) {
                EXPECT_EQ(vec[i].mKey, 42);
                EXPECT_EQ(vec[i].dummy, original[i].dummy);
            }
        }

        /// @brief Test integer version (no extractor, no buffer).
        TEST_F(RadixSortTest, IntegerVersion) {
            std::vector<uint32_t> data = {5, 1, 4, 2, 8, 0, 3, 7, 6};
            auto expected = data;
            std::sort(expected.begin(), expected.end());
            fermat::radix_sort(data.begin(), data.end()); // simplified API
            EXPECT_EQ(data, expected);
        }

        /// @brief Test extractor version with lambda.
        TEST_F(RadixSortTest, ExtractorLambda) {
            std::vector<SimpleKey> vec = {{5, 0}, {1, 1}, {4, 2}, {2, 3}, {8, 4}};
            auto expected = vec;
            std::sort(expected.begin(), expected.end(),
                      [](const SimpleKey &a, const SimpleKey &b) { return a.mKey < b.mKey; });
            fermat::radix_sort(vec.begin(), vec.end(), [](const SimpleKey &x) { return x.mKey; });
            EXPECT_EQ(vec, expected);
        }

        /// @brief Test extractor version with function object.
        TEST_F(RadixSortTest, ExtractorFunctor) {
            std::vector<Person> persons = {{42, "Alice"}, {17, "Bob"}, {99, "Charlie"}};
            auto expected = persons;
            std::sort(expected.begin(), expected.end(),
                      [](const Person &a, const Person &b) { return a.id < b.id; });
            fermat::radix_sort(persons.begin(), persons.end(), ExtractPersonId{});
            EXPECT_EQ(persons, expected);
        }
        /// @brief Test extractor version with function object.
        TEST_F(RadixSortTest, ExtractorFunctorLambda) {
            std::vector<Person> persons = {{42, "Alice"}, {17, "Bob"}, {99, "Charlie"}};
            auto expected = persons;
            std::sort(expected.begin(), expected.end(),
                      [](const Person &a, const Person &b) { return a.id < b.id; });
            fermat::radix_sort(persons.begin(), persons.end(), [](const Person &p) {return p.id;});
            EXPECT_EQ(persons, expected);
        }
    } // namespace
} // namespace fermat
