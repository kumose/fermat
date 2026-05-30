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

#include <fermat/container/bitset.h>
#include <gtest/gtest.h>

#include <climits>
#include <cstdint>

namespace {

using fermat::Bitset;
using fermat::GetFirstBit;
using fermat::GetLastBit;

template<typename UInt>
void TestGetFirstBitImpl() {
    EXPECT_EQ(GetFirstBit(static_cast<UInt>(0)), sizeof(UInt) * CHAR_BIT);

    for (uint32_t i = 0; i < sizeof(UInt) * CHAR_BIT; ++i) {
        UInt x = (static_cast<UInt>(1) << i) |
                 (static_cast<UInt>(1) << (sizeof(UInt) * CHAR_BIT - 1));
        EXPECT_EQ(GetFirstBit(x), i);
    }
}

template<typename UInt>
void TestGetLastBitImpl() {
    EXPECT_EQ(GetLastBit(static_cast<UInt>(0)), sizeof(UInt) * CHAR_BIT);

    for (uint32_t i = 0; i < sizeof(UInt) * CHAR_BIT; ++i) {
        UInt x = (static_cast<UInt>(1) << i) | static_cast<UInt>(1);
        EXPECT_EQ(GetLastBit(x), i);
    }
}

template<size_t N, typename WordType, typename UInt>
void VerifyBitsetConversionTruncated(const Bitset<N, WordType> &bs,
                                     UInt (*convert)(const Bitset<N, WordType> &),
                                     UInt truncatedValue) {
    EXPECT_EQ(convert(bs), truncatedValue);
}

template<typename WordType>
void TestBitsetWithWord() {
    auto verifyToUint32Truncated = [](const auto &bs, uint32_t truncatedValue) {
        uint32_t (*convert)(const decltype(bs) &) =
                [](const auto &b) { return b.to_uint32_no_assert_convertible(); };
        VerifyBitsetConversionTruncated(bs, convert, truncatedValue);
    };

    auto verifyToUint64Truncated = [](const auto &bs, uint64_t truncatedValue) {
        uint64_t (*convert)(const decltype(bs) &) =
                [](const auto &b) { return b.to_uint64_no_assert_convertible(); };
        VerifyBitsetConversionTruncated(bs, convert, truncatedValue);
    };

    auto verifyToUlongTruncatedIf32bit = [](const auto &bs, unsigned long truncatedValue) {
        if constexpr (sizeof(unsigned long) < 8) {
            unsigned long (*convert)(const decltype(bs) &) =
                    [](const auto &b) { return b.to_ulong_no_assert_convertible(); };
            VerifyBitsetConversionTruncated(bs, convert, truncatedValue);
        } else {
            EXPECT_EQ(bs.to_ulong_no_assert_convertible(), truncatedValue);
        }
    };

    {
        Bitset<0, WordType> b0(0x10101010);
        EXPECT_EQ(b0.count(), 0u);
        EXPECT_EQ(b0.to_ulong_assert_convertible(), 0x00000000ul);
        EXPECT_EQ(b0.to_uint32_assert_convertible(), 0x00000000u);
        EXPECT_EQ(b0.to_uint64_assert_convertible(), 0x00000000ull);
        EXPECT_EQ(b0.as_uint64(), 0x00000000ull);
        EXPECT_EQ(b0.template as_uint<uint64_t>(), 0x00000000ull);

        b0.flip();
        EXPECT_EQ(b0.count(), 0u);
        EXPECT_EQ(b0.to_ulong_assert_convertible(), 0x00000000ul);
        EXPECT_EQ(b0.to_uint32_assert_convertible(), 0x00000000u);
        EXPECT_EQ(b0.to_uint64_assert_convertible(), 0x00000000ull);
        EXPECT_EQ(b0.as_uint64(), 0x00000000ull);
        EXPECT_EQ(b0.template as_uint<uint64_t>(), 0x00000000ull);

        b0 <<= 1;
        EXPECT_EQ(b0.count(), 0u);
        EXPECT_EQ(b0.to_ulong_assert_convertible(), 0x00000000ul);
        EXPECT_EQ(b0.to_uint32_assert_convertible(), 0x00000000u);
        EXPECT_EQ(b0.to_uint64_assert_convertible(), 0x00000000ull);
        EXPECT_EQ(b0.as_uint64(), 0x00000000ull);
        EXPECT_EQ(b0.template as_uint<uint64_t>(), 0x00000000ull);

        b0 = Bitset<0, WordType>(0x10101010);
        EXPECT_EQ(b0.to_uint32_assert_convertible(), 0x00000000u);
        b0 = Bitset<0, WordType>(UINT64_C(0x1010101010101010));
        EXPECT_EQ(b0.to_uint64_assert_convertible(), UINT64_C(0x0000000000000000));

        Bitset<8, WordType> b8(0x10101010);
        EXPECT_EQ(b8.count(), 1u);
        EXPECT_EQ(b8.to_ulong_assert_convertible(), 0x00000010ul);
        EXPECT_EQ(b8.to_uint32_assert_convertible(), 0x00000010u);
        EXPECT_EQ(b8.to_uint64_assert_convertible(), 0x00000010ull);
        EXPECT_EQ(b8.as_uint64(), 0x00000010ull);
        EXPECT_EQ(b8.template as_uint<uint64_t>(), 0x00000010ull);

        b8.flip();
        EXPECT_EQ(b8.count(), 7u);
        EXPECT_EQ(b8.to_ulong_assert_convertible(), 0x000000eful);
        EXPECT_EQ(b8.to_uint32_assert_convertible(), 0x000000efu);
        EXPECT_EQ(b8.to_uint64_assert_convertible(), 0x000000efull);
        EXPECT_EQ(b8.as_uint64(), 0x000000efull);
        EXPECT_EQ(b8.template as_uint<uint64_t>(), 0x000000efull);

        b8 <<= 1;
        EXPECT_EQ(b8.count(), 6u);
        EXPECT_EQ(b8.to_ulong_assert_convertible(), 0x000000deul);
        EXPECT_EQ(b8.to_uint32_assert_convertible(), 0x000000deu);
        EXPECT_EQ(b8.to_uint64_assert_convertible(), 0x000000deull);
        EXPECT_EQ(b8.as_uint64(), 0x000000deull);
        EXPECT_EQ(b8.template as_uint<uint64_t>(), 0x000000deull);

        b8.reset();
        b8.flip();
        b8 >>= 33;
        EXPECT_EQ(b8.count(), 0u);

        b8.reset();
        b8.flip();
        b8 >>= 65;
        EXPECT_EQ(b8.count(), 0u);

        b8 = Bitset<8, WordType>(0x10101010);
        EXPECT_EQ(b8.to_uint32_assert_convertible(), 0x00000010u);
        b8 = Bitset<8, WordType>(UINT64_C(0x0000000000000010));
        EXPECT_EQ(b8.to_uint64_assert_convertible(), UINT64_C(0x0000000000000010));

        Bitset<16, WordType> b16(0x10101010);
        EXPECT_EQ(b16.count(), 2u);
        EXPECT_EQ(b16.to_ulong_assert_convertible(), 0x00001010ul);
        EXPECT_EQ(b16.to_uint32_assert_convertible(), 0x00001010u);
        EXPECT_EQ(b16.to_uint64_assert_convertible(), 0x00001010ull);
        EXPECT_EQ(b16.as_uint64(), 0x00001010ull);
        EXPECT_EQ(b16.template as_uint<uint64_t>(), 0x00001010ull);

        b16.flip();
        EXPECT_EQ(b16.count(), 14u);
        EXPECT_EQ(b16.to_ulong_assert_convertible(), 0x0000efeful);
        EXPECT_EQ(b16.to_uint32_assert_convertible(), 0x0000efefu);
        EXPECT_EQ(b16.to_uint64_assert_convertible(), 0x0000efefull);
        EXPECT_EQ(b16.as_uint64(), 0x0000efefull);
        EXPECT_EQ(b16.template as_uint<uint64_t>(), 0x0000efefull);

        b16 <<= 1;
        EXPECT_EQ(b16.count(), 13u);
        EXPECT_EQ(b16.to_ulong_assert_convertible(), 0x0000dfdeul);
        EXPECT_EQ(b16.to_uint32_assert_convertible(), 0x0000dfdeu);
        EXPECT_EQ(b16.to_uint64_assert_convertible(), 0x0000dfdeull);
        EXPECT_EQ(b16.as_uint64(), 0x0000dfdeull);
        EXPECT_EQ(b16.template as_uint<uint64_t>(), 0x0000dfdeull);

        b16.reset();
        b16.flip();
        b16 >>= 33;
        EXPECT_EQ(b16.count(), 0u);

        b16.reset();
        b16.flip();
        b16 >>= 65;
        EXPECT_EQ(b16.count(), 0u);

        b16 = Bitset<16, WordType>(0x10101010);
        EXPECT_EQ(b16.to_uint32_assert_convertible(), 0x00001010u);
        b16 = Bitset<16, WordType>(UINT64_C(0x0000000000001010));
        EXPECT_EQ(b16.to_uint64_assert_convertible(), UINT64_C(0x0000000000001010));

        Bitset<32, WordType> b32(0x10101010);
        EXPECT_EQ(b32.count(), 4u);
        EXPECT_EQ(b32.to_ulong_assert_convertible(), 0x10101010ul);
        EXPECT_EQ(b32.to_uint32_assert_convertible(), 0x10101010u);
        EXPECT_EQ(b32.to_uint64_assert_convertible(), 0x10101010ull);
        EXPECT_EQ(b32.as_uint64(), 0x10101010ull);
        EXPECT_EQ(b32.template as_uint<uint64_t>(), 0x10101010ull);

        b32.flip();
        EXPECT_EQ(b32.count(), 28u);
        EXPECT_EQ(b32.to_ulong_assert_convertible(), 0xefefefeful);
        EXPECT_EQ(b32.to_uint32_assert_convertible(), 0xefefefefu);
        EXPECT_EQ(b32.to_uint64_assert_convertible(), 0xefefefefull);
        EXPECT_EQ(b32.as_uint64(), 0xefefefefull);
        EXPECT_EQ(b32.template as_uint<uint64_t>(), 0xefefefefull);

        b32 <<= 1;
        EXPECT_EQ(b32.count(), 27u);
        EXPECT_EQ(b32.to_ulong_assert_convertible(), 0xdfdfdfdeul);
        EXPECT_EQ(b32.to_uint32_assert_convertible(), 0xdfdfdfdeu);
        EXPECT_EQ(b32.to_uint64_assert_convertible(), 0xdfdfdfdeull);
        EXPECT_EQ(b32.as_uint64(), 0xdfdfdfdeull);
        EXPECT_EQ(b32.template as_uint<uint64_t>(), 0xdfdfdfdeull);

        b32.reset();
        b32.flip();
        b32 >>= 33;
        EXPECT_EQ(b32.count(), 0u);

        b32.reset();
        b32.flip();
        b32 >>= 65;
        EXPECT_EQ(b32.count(), 0u);

        b32 = Bitset<32, WordType>(0x10101010);
        EXPECT_EQ(b32.to_uint32_assert_convertible(), 0x10101010u);
        b32 = Bitset<32, WordType>(UINT64_C(0x0000000010101010));
        EXPECT_EQ(b32.to_uint64_assert_convertible(), UINT64_C(0x0000000010101010));

        Bitset<64, WordType> b64(0x10101010);
        EXPECT_EQ(b64.count(), 4u);
        EXPECT_EQ(b64.to_ulong_assert_convertible(), 0x10101010ul);
        EXPECT_EQ(b64.to_uint32_assert_convertible(), 0x10101010u);
        EXPECT_EQ(b64.to_uint64_assert_convertible(), 0x10101010ull);
        EXPECT_EQ(b64.as_uint64(), 0x10101010ull);
        EXPECT_EQ(b64.template as_uint<uint64_t>(), 0x10101010ull);

        b64.flip();
        EXPECT_EQ(b64.count(), 60u);
        verifyToUlongTruncatedIf32bit(b64, static_cast<unsigned long>(0xffffffffefefefefull));
        verifyToUint32Truncated(b64, 0xefefefefu);
        EXPECT_EQ(b64.to_uint64_assert_convertible(), 0xffffffffefefefefull);
        EXPECT_EQ(b64.as_uint64(), 0xffffffffefefefefull);
        EXPECT_EQ(b64.template as_uint<uint64_t>(), 0xffffffffefefefefull);

        b64 <<= 1;
        EXPECT_EQ(b64.count(), 59u);
        verifyToUlongTruncatedIf32bit(b64, static_cast<unsigned long>(0xffffffffdfdfdfdeull));
        verifyToUint32Truncated(b64, 0xdfdfdfdeu);
        EXPECT_EQ(b64.to_uint64_assert_convertible(), 0xffffffffdfdfdfdeull);
        EXPECT_EQ(b64.as_uint64(), 0xffffffffdfdfdfdeull);
        EXPECT_EQ(b64.template as_uint<uint64_t>(), 0xffffffffdfdfdfdeull);

        b64.reset();
        EXPECT_EQ(b64.count(), 0u);
        EXPECT_EQ(b64.to_ulong_assert_convertible(), 0ul);

        b64 <<= 1;
        EXPECT_EQ(b64.count(), 0u);
        EXPECT_EQ(b64.to_ulong_assert_convertible(), 0ul);

        b64.flip();
        EXPECT_EQ(b64.count(), 64u);
        verifyToUlongTruncatedIf32bit(b64, static_cast<unsigned long>(0xffffffffffffffffull));

        b64 <<= 1;
        EXPECT_EQ(b64.count(), 63u);
        verifyToUlongTruncatedIf32bit(b64, static_cast<unsigned long>(0xfffffffffffffffeull));

        b64.reset();
        b64.flip();
        b64 >>= 33;
        EXPECT_EQ(b64.count(), 31u);

        b64.reset();
        b64.flip();
        b64 >>= 65;
        EXPECT_EQ(b64.count(), 0u);

        b64 = Bitset<64, WordType>(0x10101010);
        EXPECT_EQ(b64.to_uint32_assert_convertible(), 0x10101010u);
        b64 = Bitset<64, WordType>(UINT64_C(0x1010101010101010));
        EXPECT_EQ(b64.to_uint64_assert_convertible(), UINT64_C(0x1010101010101010));
    }

    {
        Bitset<1, WordType> b1;
        Bitset<1, WordType> b1A(1);

        EXPECT_EQ(b1.size(), 1u);
        EXPECT_FALSE(b1.any());
        EXPECT_FALSE(b1.all());
        EXPECT_TRUE(b1.none());
        EXPECT_EQ(b1.to_ulong_assert_convertible(), 0ul);
        EXPECT_TRUE(b1A.any());
        EXPECT_TRUE(b1A.all());
        EXPECT_FALSE(b1A.none());
        EXPECT_EQ(b1A.to_ulong_assert_convertible(), 1ul);
        EXPECT_EQ(b1A.to_uint32_assert_convertible(), 1u);
        EXPECT_EQ(b1A.to_uint64_assert_convertible(), 1ull);

        Bitset<33, WordType> b33;
        Bitset<33, WordType> b33A(1);

        EXPECT_EQ(b33.size(), 33u);
        EXPECT_FALSE(b33.any());
        EXPECT_FALSE(b33.all());
        EXPECT_TRUE(b33.none());
        EXPECT_EQ(b33.to_ulong_assert_convertible(), 0ul);
        EXPECT_TRUE(b33A.any());
        EXPECT_FALSE(b33A.all());
        EXPECT_FALSE(b33A.none());
        EXPECT_EQ(b33A.to_ulong_assert_convertible(), 1ul);

        Bitset<65, WordType> b65;
        Bitset<65, WordType> b65A(1);

        EXPECT_EQ(b65.size(), 65u);
        EXPECT_FALSE(b65.any());
        EXPECT_FALSE(b65.all());
        EXPECT_TRUE(b65.none());
        EXPECT_EQ(b65.to_ulong_assert_convertible(), 0ul);
        EXPECT_TRUE(b65A.any());
        EXPECT_FALSE(b65A.all());
        EXPECT_FALSE(b65A.none());
        EXPECT_EQ(b65A.to_ulong_assert_convertible(), 1ul);

        Bitset<129, WordType> b129;
        Bitset<129, WordType> b129A(1);

        EXPECT_EQ(b129.size(), 129u);
        EXPECT_FALSE(b129.any());
        EXPECT_FALSE(b129.all());
        EXPECT_TRUE(b129.none());
        EXPECT_EQ(b129.to_ulong_assert_convertible(), 0ul);
        EXPECT_TRUE(b129A.any());
        EXPECT_FALSE(b129A.all());
        EXPECT_FALSE(b129A.none());
        EXPECT_EQ(b129A.to_ulong_assert_convertible(), 1ul);

        b1[0] = true;
        EXPECT_TRUE(b1.test(0));
        EXPECT_EQ(b1.count(), 1u);

        b33[0] = true;
        b33[32] = true;
        EXPECT_TRUE(b33.test(0));
        EXPECT_FALSE(b33.test(15));
        EXPECT_TRUE(b33.test(32));
        EXPECT_EQ(b33.count(), 2u);

        b65[0] = true;
        b65[32] = true;
        b65[64] = true;
        EXPECT_TRUE(b65.test(0));
        EXPECT_FALSE(b65.test(15));
        EXPECT_TRUE(b65.test(32));
        EXPECT_FALSE(b65.test(47));
        EXPECT_TRUE(b65.test(64));
        EXPECT_EQ(b65.count(), 3u);

        b129[0] = true;
        b129[32] = true;
        b129[64] = true;
        b129[128] = true;
        EXPECT_TRUE(b129.test(0));
        EXPECT_FALSE(b129.test(15));
        EXPECT_TRUE(b129.test(32));
        EXPECT_FALSE(b129.test(47));
        EXPECT_TRUE(b129.test(64));
        EXPECT_FALSE(b129.test(91));
        EXPECT_TRUE(b129.test(128));
        EXPECT_EQ(b129.count(), 4u);

        EXPECT_NE(b1.data(), nullptr);
        EXPECT_NE(b33.data(), nullptr);
        EXPECT_NE(b65.data(), nullptr);
        EXPECT_NE(b129.data(), nullptr);

        b1.reset();
        EXPECT_EQ(b1.count(), 0u);

        b1.set();
        EXPECT_EQ(b1.count(), b1.size());
        EXPECT_TRUE(b1.all());

        b1.flip();
        EXPECT_EQ(b1.count(), 0u);
        EXPECT_FALSE(b1.all());
        EXPECT_TRUE(b1.none());

        b1.set(0, true);
        EXPECT_TRUE(b1[0]);

        b1.reset(0);
        EXPECT_FALSE(b1[0]);

        b1.flip(0);
        EXPECT_TRUE(b1[0]);

        Bitset<1, WordType> b1Not = ~b1;
        EXPECT_TRUE(b1[0]);
        EXPECT_FALSE(b1Not[0]);

        b33.reset();
        EXPECT_EQ(b33.count(), 0u);

        b33.set();
        EXPECT_EQ(b33.count(), b33.size());
        EXPECT_TRUE(b33.all());

        b33.flip();
        EXPECT_EQ(b33.count(), 0u);
        EXPECT_FALSE(b33.all());

        b33.set(0, true);
        b33.set(32, true);
        EXPECT_TRUE(b33[0]);
        EXPECT_FALSE(b33[15]);
        EXPECT_TRUE(b33[32]);

        b33.reset(0);
        b33.reset(32);
        EXPECT_FALSE(b33[0]);
        EXPECT_FALSE(b33[32]);

        b33.flip(0);
        b33.flip(32);
        EXPECT_TRUE(b33[0]);
        EXPECT_TRUE(b33[32]);

        Bitset<33, WordType> b33Not(~b33);
        EXPECT_TRUE(b33[0]);
        EXPECT_TRUE(b33[32]);
        EXPECT_FALSE(b33Not[0]);
        EXPECT_FALSE(b33Not[32]);

        b65.reset();
        EXPECT_EQ(b65.count(), 0u);
        EXPECT_FALSE(b65.all());
        EXPECT_TRUE(b65.none());

        b65.set();
        EXPECT_EQ(b65.count(), b65.size());
        EXPECT_TRUE(b65.all());
        EXPECT_FALSE(b65.none());

        b65.flip();
        EXPECT_EQ(b65.count(), 0u);
        EXPECT_FALSE(b65.all());
        EXPECT_TRUE(b65.none());

        b65.set(0, true);
        b65.set(32, true);
        b65.set(64, true);
        EXPECT_TRUE(b65[0]);
        EXPECT_FALSE(b65[15]);
        EXPECT_TRUE(b65[32]);
        EXPECT_FALSE(b65[50]);
        EXPECT_TRUE(b65[64]);

        b65.reset(0);
        b65.reset(32);
        b65.reset(64);
        EXPECT_FALSE(b65[0]);
        EXPECT_FALSE(b65[32]);
        EXPECT_FALSE(b65[64]);

        b65.flip(0);
        b65.flip(32);
        b65.flip(64);
        EXPECT_TRUE(b65[0]);
        EXPECT_TRUE(b65[32]);
        EXPECT_TRUE(b65[64]);

        Bitset<65, WordType> b65Not(~b65);
        EXPECT_TRUE(b65[0]);
        EXPECT_TRUE(b65[32]);
        EXPECT_TRUE(b65[64]);
        EXPECT_FALSE(b65Not[0]);
        EXPECT_FALSE(b65Not[32]);
        EXPECT_FALSE(b65Not[64]);

        b129.reset();
        EXPECT_EQ(b129.count(), 0u);

        b129.set();
        EXPECT_EQ(b129.count(), b129.size());
        EXPECT_TRUE(b129.all());

        b129.flip();
        EXPECT_EQ(b129.count(), 0u);
        EXPECT_FALSE(b129.all());
        EXPECT_TRUE(b129.none());

        b129.set(0, true);
        b129.set(32, true);
        b129.set(64, true);
        b129.set(128, true);
        EXPECT_TRUE(b129[0]);
        EXPECT_FALSE(b129[15]);
        EXPECT_TRUE(b129[32]);
        EXPECT_FALSE(b129[50]);
        EXPECT_TRUE(b129[64]);
        EXPECT_FALSE(b129[90]);
        EXPECT_TRUE(b129[128]);

        b129.reset(0);
        b129.reset(32);
        b129.reset(64);
        b129.reset(128);
        EXPECT_FALSE(b129[0]);
        EXPECT_FALSE(b129[32]);
        EXPECT_FALSE(b129[64]);
        EXPECT_FALSE(b129[128]);

        b129.flip(0);
        b129.flip(32);
        b129.flip(64);
        b129.flip(128);
        EXPECT_TRUE(b129[0]);
        EXPECT_TRUE(b129[32]);
        EXPECT_TRUE(b129[64]);
        EXPECT_TRUE(b129[128]);

        Bitset<129, WordType> b129Not(~b129);
        EXPECT_TRUE(b129[0]);
        EXPECT_TRUE(b129[32]);
        EXPECT_TRUE(b129[64]);
        EXPECT_TRUE(b129[128]);
        EXPECT_FALSE(b129Not[0]);
        EXPECT_FALSE(b129Not[32]);
        EXPECT_FALSE(b129Not[64]);
        EXPECT_FALSE(b129Not[128]);

        Bitset<1, WordType> b1Equal(b1);
        EXPECT_EQ(b1Equal, b1);
        EXPECT_NE(b1Equal, b1Not);

        Bitset<33, WordType> b33Equal(b33);
        EXPECT_EQ(b33Equal, b33);
        EXPECT_NE(b33Equal, b33Not);

        Bitset<65, WordType> b65Equal(b65);
        EXPECT_EQ(b65Equal, b65);
        EXPECT_NE(b65Equal, b65Not);

        Bitset<129, WordType> b129Equal(b129);
        EXPECT_EQ(b129Equal, b129);
        EXPECT_NE(b129Equal, b129Not);

        b1.reset();
        b1[0] = true;
        b1 >>= 0;
        EXPECT_TRUE(b1[0]);
        b1 >>= 1;
        EXPECT_FALSE(b1[0]);

        b1[0] = true;
        b1 <<= 0;
        EXPECT_TRUE(b1[0]);
        b1 <<= 1;
        EXPECT_FALSE(b1[0]);

        b1[0] = true;
        b1Equal = b1 >> 0;
        EXPECT_EQ(b1Equal, b1);
        b1Equal = b1 >> 1;
        EXPECT_FALSE(b1Equal[0]);

        b1[0] = true;
        b1Equal = b1 << 0;
        EXPECT_TRUE(b1Equal[0]);
        b1Equal = b1 << 1;
        EXPECT_FALSE(b1Equal[0]);

        b1.reset();
        b1.flip();
        b1 >>= 33;
        EXPECT_EQ(b1.count(), 0u);
        EXPECT_FALSE(b1.all());
        EXPECT_TRUE(b1.none());

        b1.reset();
        b1.flip();
        b1 <<= 33;
        EXPECT_EQ(b1.count(), 0u);
        EXPECT_FALSE(b1.all());
        EXPECT_TRUE(b1.none());

        b1.reset();
        b1.flip();
        b1 >>= 65;
        EXPECT_EQ(b1.count(), 0u);
        EXPECT_FALSE(b1.all());
        EXPECT_TRUE(b1.none());

        b1.reset();
        b1.flip();
        b1 <<= 65;
        EXPECT_EQ(b1.count(), 0u);
        EXPECT_FALSE(b1.all());
        EXPECT_TRUE(b1.none());

        b33.reset();
        b33[0] = true;
        b33[32] = true;
        b33 >>= 0;
        EXPECT_TRUE(b33[0]);
        EXPECT_TRUE(b33[32]);
        b33 >>= 10;
        EXPECT_TRUE(b33[22]);

        b33.reset();
        b33[0] = true;
        b33[32] = true;
        b33 <<= 0;
        EXPECT_TRUE(b33[0]);
        EXPECT_TRUE(b33[32]);
        b33 <<= 10;
        EXPECT_TRUE(b33[10]);

        b33.reset();
        b33[0] = true;
        b33[32] = true;
        b33Equal = b33 >> 0;
        EXPECT_EQ(b33Equal, b33);
        b33Equal = b33 >> 10;
        EXPECT_TRUE(b33Equal[22]);

        b33.reset();
        b33[0] = true;
        b33[32] = true;
        b33Equal = b33 << 10;
        EXPECT_TRUE(b33Equal[10]);

        b33.reset();
        b33.flip();
        b33 >>= 33;
        EXPECT_EQ(b33.count(), 0u);
        EXPECT_FALSE(b33.all());
        EXPECT_TRUE(b33.none());

        b33.reset();
        b33.flip();
        b33 <<= 33;
        EXPECT_EQ(b33.count(), 0u);
        EXPECT_FALSE(b33.all());
        EXPECT_TRUE(b33.none());

        b33.reset();
        b33.flip();
        b33 >>= 65;
        EXPECT_EQ(b33.count(), 0u);
        EXPECT_FALSE(b33.all());
        EXPECT_TRUE(b33.none());

        b33.reset();
        b33.flip();
        b33 <<= 65;
        EXPECT_EQ(b33.count(), 0u);
        EXPECT_FALSE(b33.all());
        EXPECT_TRUE(b33.none());

        b65.reset();
        b65[0] = true;
        b65[32] = true;
        b65[64] = true;
        b65 >>= 0;
        EXPECT_TRUE(b65[0]);
        EXPECT_TRUE(b65[32]);
        EXPECT_TRUE(b65[64]);
        b65 >>= 10;
        EXPECT_TRUE(b65[22]);
        EXPECT_TRUE(b65[54]);

        b65.reset();
        b65[0] = true;
        b65[32] = true;
        b65[64] = true;
        b65 <<= 0;
        EXPECT_TRUE(b65[0]);
        EXPECT_TRUE(b65[32]);
        EXPECT_TRUE(b65[64]);
        b65 <<= 10;
        EXPECT_TRUE(b65[10]);
        EXPECT_TRUE(b65[42]);

        b65.reset();
        b65[0] = true;
        b65[32] = true;
        b65[64] = true;
        b65Equal = b65 >> 0;
        EXPECT_EQ(b65Equal, b65);
        b65Equal = b65 >> 10;
        EXPECT_TRUE(b65Equal[22]);
        EXPECT_TRUE(b65Equal[54]);

        b65.reset();
        b65[0] = true;
        b65[32] = true;
        b65[64] = true;
        b65Equal = b65 << 10;
        EXPECT_TRUE(b65Equal[10]);
        EXPECT_TRUE(b65Equal[42]);

        b65.reset();
        b65.flip();
        b65 >>= 33;
        EXPECT_EQ(b65.count(), 32u);

        b65.reset();
        b65.flip();
        b65 <<= 33;
        EXPECT_EQ(b65.count(), 32u);

        b65.reset();
        b65.flip();
        b65 >>= 65;
        EXPECT_EQ(b65.count(), 0u);

        b65.reset();
        b65.flip();
        b65 <<= 65;
        EXPECT_EQ(b65.count(), 0u);

        b129.reset();
        b129[0] = true;
        b129[32] = true;
        b129[64] = true;
        b129[128] = true;
        b129 >>= 0;
        EXPECT_TRUE(b129[0]);
        EXPECT_TRUE(b129[32]);
        EXPECT_TRUE(b129[64]);
        EXPECT_TRUE(b129[128]);
        b129 >>= 10;
        EXPECT_TRUE(b129[22]);
        EXPECT_TRUE(b129[54]);
        EXPECT_TRUE(b129[118]);

        b129.reset();
        b129[0] = true;
        b129[32] = true;
        b129[64] = true;
        b129[128] = true;
        b129 <<= 0;
        EXPECT_TRUE(b129[0]);
        EXPECT_TRUE(b129[32]);
        EXPECT_TRUE(b129[64]);
        EXPECT_TRUE(b129[128]);
        b129 <<= 10;
        EXPECT_TRUE(b129[10]);
        EXPECT_TRUE(b129[42]);
        EXPECT_TRUE(b129[74]);

        b129.reset();
        b129[0] = true;
        b129[32] = true;
        b129[64] = true;
        b129[128] = true;
        b129Equal = b129 >> 0;
        EXPECT_EQ(b129Equal, b129);
        b129Equal = b129 >> 10;
        EXPECT_TRUE(b129Equal[22]);
        EXPECT_TRUE(b129Equal[54]);
        EXPECT_TRUE(b129Equal[118]);

        b129.reset();
        b129[0] = true;
        b129[32] = true;
        b129[64] = true;
        b129[128] = true;
        b129Equal = b129 << 10;
        EXPECT_TRUE(b129Equal[10]);
        EXPECT_TRUE(b129Equal[42]);
        EXPECT_TRUE(b129Equal[74]);

        b129.reset();
        b129.flip();
        b129 >>= 33;
        EXPECT_EQ(b129.count(), 96u);

        b129.reset();
        b129.flip();
        b129 <<= 33;
        EXPECT_EQ(b129.count(), 96u);

        b129.reset();
        b129.flip();
        b129 >>= 65;
        EXPECT_EQ(b129.count(), 64u);

        b129.reset();
        b129.flip();
        b129 <<= 65;
        EXPECT_EQ(b129.count(), 64u);

        b1.set();
        b1[0] = false;
        b1A[0] = true;
        b1 &= b1A;
        EXPECT_FALSE(b1[0]);
        b1 |= b1A;
        EXPECT_TRUE(b1[0]);
        b1 ^= b1A;
        EXPECT_FALSE(b1[0]);
        b1 |= b1A;
        EXPECT_TRUE(b1[0]);

        b33.set();
        b33[0] = false;
        b33[32] = false;
        b33A[0] = true;
        b33A[32] = true;
        b33 &= b33A;
        EXPECT_FALSE(b33[0]);
        EXPECT_FALSE(b33[32]);
        b33 |= b33A;
        EXPECT_TRUE(b33[0]);
        EXPECT_TRUE(b33[32]);
        b33 ^= b33A;
        EXPECT_FALSE(b33[0]);
        EXPECT_FALSE(b33[32]);
        b33 |= b33A;
        EXPECT_TRUE(b33[0]);
        EXPECT_TRUE(b33[32]);

        b65.set();
        b65[0] = false;
        b65[32] = false;
        b65[64] = false;
        b65A[0] = true;
        b65A[32] = true;
        b65A[64] = true;
        b65 &= b65A;
        EXPECT_FALSE(b65[0]);
        EXPECT_FALSE(b65[32]);
        EXPECT_FALSE(b65[64]);
        b65 |= b65A;
        EXPECT_TRUE(b65[0]);
        EXPECT_TRUE(b65[32]);
        EXPECT_TRUE(b65[64]);
        b65 ^= b65A;
        EXPECT_FALSE(b65[0]);
        EXPECT_FALSE(b65[32]);
        EXPECT_FALSE(b65[64]);
        b65 |= b65A;
        EXPECT_TRUE(b65[0]);
        EXPECT_TRUE(b65[32]);
        EXPECT_TRUE(b65[64]);

        b129.set();
        b129[0] = false;
        b129[32] = false;
        b129[64] = false;
        b129[128] = false;
        b129A[0] = true;
        b129A[32] = true;
        b129A[64] = true;
        b129A[128] = true;
        b129 &= b129A;
        EXPECT_FALSE(b129[0]);
        EXPECT_FALSE(b129[32]);
        EXPECT_FALSE(b129[64]);
        EXPECT_FALSE(b129[128]);
        b129 |= b129A;
        EXPECT_TRUE(b129[0]);
        EXPECT_TRUE(b129[32]);
        EXPECT_TRUE(b129[64]);
        EXPECT_TRUE(b129[128]);
        b129 ^= b129A;
        EXPECT_FALSE(b129[0]);
        EXPECT_FALSE(b129[32]);
        EXPECT_FALSE(b129[64]);
        EXPECT_FALSE(b129[128]);
        b129 |= b129A;
        EXPECT_TRUE(b129[0]);
        EXPECT_TRUE(b129[32]);
        EXPECT_TRUE(b129[64]);
        EXPECT_TRUE(b129[128]);
    }

    {
        Bitset<65, WordType> b65;
        typename Bitset<65, WordType>::reference r = b65[33];

        r = true;
        EXPECT_TRUE(r);
    }

    {
        size_t i = 0;
        size_t j = 0;

        Bitset<1, WordType> b1;
        i = b1.find_first();
        EXPECT_EQ(i, b1.kSize);
        b1.set(0, true);
        i = b1.find_first();
        EXPECT_EQ(i, 0u);
        i = b1.find_next(i);
        EXPECT_EQ(i, b1.kSize);

        b1.set();
        for (i = 0, j = b1.find_first(); j != b1.kSize; j = b1.find_next(j))
            ++i;
        EXPECT_EQ(i, 1u);

        Bitset<7, WordType> b7;
        i = b7.find_first();
        EXPECT_EQ(i, b7.kSize);
        b7.set(0, true);
        b7.set(5, true);
        i = b7.find_first();
        EXPECT_EQ(i, 0u);
        i = b7.find_next(i);
        EXPECT_EQ(i, 5u);
        i = b7.find_next(i);
        EXPECT_EQ(i, b7.kSize);

        b7.set();
        for (i = 0, j = b7.find_first(); j != b7.kSize; j = b7.find_next(j))
            ++i;
        EXPECT_EQ(i, 7u);

        Bitset<32, WordType> b32;
        i = b32.find_first();
        EXPECT_EQ(i, b32.kSize);
        b32.set(0, true);
        b32.set(27, true);
        i = b32.find_first();
        EXPECT_EQ(i, 0u);
        i = b32.find_next(i);
        EXPECT_EQ(i, 27u);
        i = b32.find_next(i);
        EXPECT_EQ(i, b32.kSize);

        b32.set();
        for (i = 0, j = b32.find_first(); j != b32.kSize; j = b32.find_next(j))
            ++i;
        EXPECT_EQ(i, 32u);

        Bitset<41, WordType> b41;
        i = b41.find_first();
        EXPECT_EQ(i, b41.kSize);
        b41.set(0, true);
        b41.set(27, true);
        b41.set(37, true);
        i = b41.find_first();
        EXPECT_EQ(i, 0u);
        i = b41.find_next(i);
        EXPECT_EQ(i, 27u);
        i = b41.find_next(i);
        EXPECT_EQ(i, 37u);
        i = b41.find_next(i);
        EXPECT_EQ(i, b41.kSize);

        b41.set();
        for (i = 0, j = b41.find_first(); j != b41.kSize; j = b41.find_next(j))
            ++i;
        EXPECT_EQ(i, 41u);

        Bitset<64, WordType> b64;
        i = b64.find_first();
        EXPECT_EQ(i, b64.kSize);
        b64.set(0, true);
        b64.set(27, true);
        b64.set(37, true);
        i = b64.find_first();
        EXPECT_EQ(i, 0u);
        i = b64.find_next(i);
        EXPECT_EQ(i, 27u);
        i = b64.find_next(i);
        EXPECT_EQ(i, 37u);
        i = b64.find_next(i);
        EXPECT_EQ(i, b64.kSize);

        b64.set();
        for (i = 0, j = b64.find_first(); j != b64.kSize; j = b64.find_next(j))
            ++i;
        EXPECT_EQ(i, 64u);

        Bitset<79, WordType> b79;
        i = b79.find_first();
        EXPECT_EQ(i, b79.kSize);
        b79.set(0, true);
        b79.set(27, true);
        b79.set(37, true);
        i = b79.find_first();
        EXPECT_EQ(i, 0u);
        i = b79.find_next(i);
        EXPECT_EQ(i, 27u);
        i = b79.find_next(i);
        EXPECT_EQ(i, 37u);
        i = b79.find_next(i);
        EXPECT_EQ(i, b79.kSize);

        b79.set();
        for (i = 0, j = b79.find_first(); j != b79.kSize; j = b79.find_next(j))
            ++i;
        EXPECT_EQ(i, 79u);

        Bitset<128, WordType> b128;
        i = b128.find_first();
        EXPECT_EQ(i, b128.kSize);
        b128.set(0, true);
        b128.set(27, true);
        b128.set(37, true);
        b128.set(77, true);
        i = b128.find_first();
        EXPECT_EQ(i, 0u);
        i = b128.find_next(i);
        EXPECT_EQ(i, 27u);
        i = b128.find_next(i);
        EXPECT_EQ(i, 37u);
        i = b128.find_next(i);
        EXPECT_EQ(i, 77u);
        i = b128.find_next(i);
        EXPECT_EQ(i, b128.kSize);

        b128.set();
        for (i = 0, j = b128.find_first(); j != b128.kSize; j = b128.find_next(j))
            ++i;
        EXPECT_EQ(i, 128u);

        Bitset<137, WordType> b137;
        i = b137.find_first();
        EXPECT_EQ(i, b137.kSize);
        b137.set(0, true);
        b137.set(27, true);
        b137.set(37, true);
        b137.set(77, true);
        b137.set(99, true);
        b137.set(136, true);
        i = b137.find_first();
        EXPECT_EQ(i, 0u);
        i = b137.find_next(i);
        EXPECT_EQ(i, 27u);
        i = b137.find_next(i);
        EXPECT_EQ(i, 37u);
        i = b137.find_next(i);
        EXPECT_EQ(i, 77u);
        i = b137.find_next(i);
        EXPECT_EQ(i, 99u);
        i = b137.find_next(i);
        EXPECT_EQ(i, 136u);
        i = b137.find_next(i);
        EXPECT_EQ(i, b137.kSize);

        b137.set();
        for (i = 0, j = b137.find_first(); j != b137.kSize; j = b137.find_next(j))
            ++i;
        EXPECT_EQ(i, 137u);
    }

    {
        size_t i = 0;
        size_t j = 0;

        Bitset<1, WordType> b1;
        i = b1.find_last();
        EXPECT_EQ(i, b1.kSize);
        b1.set(0, true);
        i = b1.find_last();
        EXPECT_EQ(i, 0u);
        i = b1.find_prev(i);
        EXPECT_EQ(i, b1.kSize);

        b1.set();
        for (i = 0, j = b1.find_last(); j != b1.kSize; j = b1.find_prev(j))
            ++i;
        EXPECT_EQ(i, 1u);

        Bitset<7, WordType> b7;
        i = b7.find_last();
        EXPECT_EQ(i, b7.kSize);
        b7.set(0, true);
        b7.set(5, true);
        i = b7.find_last();
        EXPECT_EQ(i, 5u);
        i = b7.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b7.find_prev(i);
        EXPECT_EQ(i, b7.kSize);

        b7.set();
        for (i = 0, j = b7.find_last(); j != b7.kSize; j = b7.find_prev(j))
            ++i;
        EXPECT_EQ(i, 7u);

        Bitset<32, WordType> b32;
        i = b32.find_last();
        EXPECT_EQ(i, b32.kSize);
        b32.set(0, true);
        b32.set(27, true);
        i = b32.find_last();
        EXPECT_EQ(i, 27u);
        i = b32.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b32.find_prev(i);
        EXPECT_EQ(i, b32.kSize);

        b32.set();
        for (i = 0, j = b32.find_last(); j != b32.kSize; j = b32.find_prev(j))
            ++i;
        EXPECT_EQ(i, 32u);

        Bitset<41, WordType> b41;
        i = b41.find_last();
        EXPECT_EQ(i, b41.kSize);
        b41.set(0, true);
        b41.set(27, true);
        b41.set(37, true);
        i = b41.find_last();
        EXPECT_EQ(i, 37u);
        i = b41.find_prev(i);
        EXPECT_EQ(i, 27u);
        i = b41.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b41.find_prev(i);
        EXPECT_EQ(i, b41.kSize);

        b41.set();
        for (i = 0, j = b41.find_last(); j != b41.kSize; j = b41.find_prev(j))
            ++i;
        EXPECT_EQ(i, 41u);

        Bitset<64, WordType> b64;
        i = b64.find_last();
        EXPECT_EQ(i, b64.kSize);
        b64.set(0, true);
        b64.set(27, true);
        b64.set(37, true);
        i = b64.find_last();
        EXPECT_EQ(i, 37u);
        i = b64.find_prev(i);
        EXPECT_EQ(i, 27u);
        i = b64.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b64.find_prev(i);
        EXPECT_EQ(i, b64.kSize);

        verifyToUint32Truncated(b64, 0x08000001u);

        b64.set();
        for (i = 0, j = b64.find_last(); j != b64.kSize; j = b64.find_prev(j))
            ++i;
        EXPECT_EQ(i, 64u);

        Bitset<79, WordType> b79;
        i = b79.find_last();
        EXPECT_EQ(i, b79.kSize);
        b79.set(0, true);
        b79.set(27, true);
        b79.set(37, true);
        i = b79.find_last();
        EXPECT_EQ(i, 37u);
        i = b79.find_prev(i);
        EXPECT_EQ(i, 27u);
        i = b79.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b79.find_prev(i);
        EXPECT_EQ(i, b79.kSize);

        EXPECT_EQ(b79.to_uint64_assert_convertible(), 0x0000002008000001ull);

        b79.set();
        for (i = 0, j = b79.find_last(); j != b79.kSize; j = b79.find_prev(j))
            ++i;
        EXPECT_EQ(i, 79u);

        verifyToUint64Truncated(b79, 0xffffffffffffffffull);

        Bitset<128, WordType> b128;
        i = b128.find_last();
        EXPECT_EQ(i, b128.kSize);
        b128.set(0, true);
        b128.set(27, true);
        b128.set(37, true);
        b128.set(77, true);
        i = b128.find_last();
        EXPECT_EQ(i, 77u);
        i = b128.find_prev(i);
        EXPECT_EQ(i, 37u);
        i = b128.find_prev(i);
        EXPECT_EQ(i, 27u);
        i = b128.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b128.find_prev(i);
        EXPECT_EQ(i, b128.kSize);

        verifyToUint64Truncated(b128, 0x0000002008000001ull);

        b128.set();
        for (i = 0, j = b128.find_last(); j != b128.kSize; j = b128.find_prev(j))
            ++i;
        EXPECT_EQ(i, 128u);

        Bitset<137, WordType> b137;
        i = b137.find_last();
        EXPECT_EQ(i, b137.kSize);
        b137.set(0, true);
        b137.set(27, true);
        b137.set(37, true);
        b137.set(77, true);
        b137.set(99, true);
        b137.set(136, true);
        i = b137.find_last();
        EXPECT_EQ(i, 136u);
        i = b137.find_prev(i);
        EXPECT_EQ(i, 99u);
        i = b137.find_prev(i);
        EXPECT_EQ(i, 77u);
        i = b137.find_prev(i);
        EXPECT_EQ(i, 37u);
        i = b137.find_prev(i);
        EXPECT_EQ(i, 27u);
        i = b137.find_prev(i);
        EXPECT_EQ(i, 0u);
        i = b137.find_prev(i);
        EXPECT_EQ(i, b137.kSize);

        b137.set();
        for (i = 0, j = b137.find_last(); j != b137.kSize; j = b137.find_prev(j))
            ++i;
        EXPECT_EQ(i, 137u);

        Bitset<99, WordType> b99;
        b99.set(63);
        verifyToUint32Truncated(b99, 0x0u);
        EXPECT_EQ(b99.to_uint64_assert_convertible(), 0x8000000000000000ull);
        verifyToUlongTruncatedIf32bit(b99, static_cast<unsigned long>(0x8000000000000000ull));
    }

    TestGetFirstBitImpl<WordType>();
    TestGetLastBitImpl<WordType>();
}

} // namespace

TEST(BitsetTest, Uint64Word) {
    TestBitsetWithWord<uint64_t>();
}

TEST(BitsetTest, Uint32Word) {
    TestBitsetWithWord<uint32_t>();
}

TEST(BitsetTest, Uint16Word) {
    TestBitsetWithWord<uint16_t>();
}

TEST(BitsetTest, Uint8Word) {
    TestBitsetWithWord<uint8_t>();
}

TEST(BitsetTest, WordCountMacro) {
    {
        using bitset_t = Bitset<32, unsigned char>;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        using bitset_t = Bitset<32, unsigned int>;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        using bitset_t = Bitset<32, uint16_t>;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        using bitset_t = Bitset<32, uint32_t>;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        using bitset_t = Bitset<128, uint64_t>;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        using bitset_t = Bitset<256, uint64_t>;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
}
