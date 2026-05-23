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
#include <fermat/container/peeker.h>
#include <fermat/io/iobuf.h>
#include <fermat/container/buffer.h>
#include <fermat/container/vector.h>
#include <string>
#include <string_view>
#include <vector>

namespace fermat {
    using TestIOBuf = CordBufferBase<64, 4096>;
    using TestPeeker = Peeker<TestIOBuf>;
    using StringViewPeeker = Peeker<std::string_view>;
    using StringPeeker = Peeker<std::string>;
    using BufferPeeker = Peeker<Buffer<char, 64> >;
    using VectorPeeker = Peeker<std::vector<char> >;
    using AlignedVectorPeeker = Peeker<Vector<char, 64> >;

    // -----------------------------------------------------------------------------
    // Tests for Peeker<IOBuf>
    // -----------------------------------------------------------------------------
    class PeekerIOBufTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Fill three blocks each with a distinct pattern.
            // Block0: 4000 bytes of 'A' (not full)
            // Block1: 4000 bytes of 'B' (not full)
            // Block2: 4000 bytes of 'C' (not full)
            // Using 4000 < 4096 ensures each block gets its own allocation because
            // the first block after 4000 bytes still has 96 bytes free, but borrowing
            // with exactly 4000 will allocate a new block if the current block's
            // remaining is insufficient? Actually borrow(4000) will check write_able_size.
            // To force new blocks, we can write 4096 bytes to exactly fill each block.
            // Using exactly block size ensures a new block each time.
            const size_t block_size = TestIOBuf::kBlockSize;
            std::string data0(block_size, 'A');
            std::string data1(block_size, 'B');
            std::string data2(block_size, 'C');

            auto l1 = _buf.borrow(block_size).value_or_die();
            l1->write(data0.data(), block_size);
            _buf.commit(l1);

            auto l2 = _buf.borrow(block_size).value_or_die();
            l2->write(data1.data(), block_size);
            _buf.commit(l2);

            auto l3 = _buf.borrow(block_size).value_or_die();
            l3->write(data2.data(), block_size);
            _buf.commit(l3);
        }

        TestIOBuf _buf;
    };

    TEST_F(PeekerIOBufTest, ConstructionAndReset) {
        TestPeeker peeker(&_buf);
        EXPECT_EQ(peeker.tellg(), Position(0, 0));
        EXPECT_FALSE(peeker.eof());

        peeker.reset();
        EXPECT_EQ(peeker.tellg(), Position(0, 0));

        TestPeeker default_peeker;
        default_peeker.set_buffer(&_buf);
        EXPECT_EQ(default_peeker.tellg(), Position(0, 0));
    }

    TEST_F(PeekerIOBufTest, TellgAndEnd) {
        TestPeeker peeker(&_buf);
        EXPECT_EQ(peeker.tellg(), Position(0, 0));
        EXPECT_EQ(peeker.end(), TestPeeker::npos);
    }

    TEST_F(PeekerIOBufTest, SeekToByteOffset) {
        TestPeeker peeker(&_buf);
        // Seek to logical offset block_size + 1, which should be in second block, offset 1
        size_t block_size = TestIOBuf::kBlockSize;
        auto pos = peeker.seek_to(block_size + 1);
        EXPECT_EQ(pos, Position(1, 1));
        EXPECT_EQ(peeker.tellg(), Position(1, 1));
        EXPECT_EQ(*peeker, 'B'); // second block's character

        // Seek to end (beyond total size)
        pos = peeker.seek_to(3 * block_size);
        EXPECT_EQ(pos, TestPeeker::npos);
        EXPECT_TRUE(peeker.eof());

        // Seek to invalid offset
        pos = peeker.seek_to(3 * block_size + 100);
        EXPECT_EQ(pos, TestPeeker::npos);
        EXPECT_TRUE(peeker.eof());
    }

    TEST_F(PeekerIOBufTest, SeekToPosition) {
        TestPeeker peeker(&_buf);
        size_t block_size = TestIOBuf::kBlockSize;
        // Directly set to second block, offset 2
        peeker.seek_to(Position(1, 2));
        EXPECT_EQ(peeker.tellg(), Position(1, 2));
        EXPECT_EQ(*peeker, 'B'); // second block's character

        // Invalid index (beyond blocks)
        peeker.seek_to(Position(5, 0));
        EXPECT_TRUE(peeker.eof());

        // Invalid offset inside block (assume block_size is 4096, offset 5000 is invalid)
        peeker.seek_to(Position(1, block_size + 10));
        EXPECT_TRUE(peeker.eof());

        // Empty buffer case
        TestIOBuf empty;
        TestPeeker empty_peeker(&empty);
        empty_peeker.seek_to(Position(0, 0));
        EXPECT_TRUE(empty_peeker.eof());
    }

    TEST_F(PeekerIOBufTest, SeekEndAndStart) {
        TestPeeker peeker(&_buf);
        size_t block_size = TestIOBuf::kBlockSize;
        // Move 2 bytes forward from start
        peeker.seek_end(2);
        EXPECT_EQ(peeker.tellg(), Position(0, 2));
        EXPECT_EQ(*peeker, 'A');

        // Move across block boundary: from offset 2 in block0, move to start of block1
        peeker.seek_end(block_size - 2); // total moved = block_size
        EXPECT_EQ(peeker.tellg(), Position(1, 0));
        EXPECT_EQ(*peeker, 'B');

        // Move beyond end -> eof
        peeker.seek_end(2 * block_size + 10);
        EXPECT_TRUE(peeker.eof());

        // Reset and test seek_start
        peeker.reset(); // now at (0,0)
        peeker.seek_end(block_size + 5); // to (1,5)
        EXPECT_EQ(*peeker, 'B');
        peeker.seek_start(3); // back 3 bytes -> (1,2)
        EXPECT_EQ(peeker.tellg(), Position(1, 2));
        EXPECT_EQ(*peeker, 'B');

        // seek_start more than consumed -> reset to beginning
        peeker.seek_start(block_size + 10);
        EXPECT_EQ(peeker.tellg(), Position(0, 0));
        EXPECT_EQ(*peeker, 'A');
    }

    TEST_F(PeekerIOBufTest, OperatorStarAndIncrementDecrement) {
        TestPeeker peeker(&_buf);
        size_t block_size = TestIOBuf::kBlockSize;
        EXPECT_EQ(*peeker, 'A');

        ++peeker;
        EXPECT_EQ(*peeker, 'A'); // offset 1, still 'A'

        peeker += block_size - 2; // move to end of block0
        EXPECT_EQ(peeker.tellg(), Position(0, block_size - 1)) << peeker.tellg();
        EXPECT_EQ(*peeker, 'A');
        ++peeker; // now enter block1, offset 0
        EXPECT_EQ(peeker.tellg(), Position(1, 0));
        EXPECT_EQ(*peeker, 'B');

        --peeker; // back to block0 last char
        EXPECT_EQ(peeker.tellg(), Position(0, block_size - 1));
        peeker--; // same
        EXPECT_EQ(peeker.tellg(), Position(0, block_size - 2));

        peeker -= (block_size - 2); // back to start
        EXPECT_EQ(peeker.tellg(), Position(0, 0));

        auto p2 = peeker + block_size;
        EXPECT_EQ(p2.tellg(), Position(1, 0));
        auto p3 = peeker - 1u; // negative move should clamp to 0
        EXPECT_EQ(p3.tellg(), Position(0, 0));
    }

    TEST_F(PeekerIOBufTest, FindFirstPositionAndOffset) {
        TestPeeker peeker(&_buf);
        // Find first 'B' after start
        auto pos = peeker.find_first_position("B");
        size_t block_size = TestIOBuf::kBlockSize;
        EXPECT_EQ(pos, Position(1, 0));
        size_t off = peeker.find_first_offset("B");
        EXPECT_EQ(off, block_size); // logical offset of first 'B'

        // Move cursor to after that 'B' and search for 'C'
        peeker.seek_to(block_size + 10);
        pos = peeker.find_first_position("C");
        EXPECT_EQ(pos, Position(2, 0));
        off = peeker.find_first_offset("C");
        EXPECT_EQ(off, 2 * block_size);

        // Search for non-existing character
        pos = peeker.find_first_position("X");
        EXPECT_EQ(pos, TestPeeker::npos);
        off = peeker.find_first_offset("X");
        EXPECT_EQ(off, TestPeeker::kNPos);
    }

    TEST_F(PeekerIOBufTest, Readn) {
        TestPeeker peeker(&_buf);
        size_t block_size = TestIOBuf::kBlockSize;

        // Read 2 bytes from start
        auto chunk = peeker.readn(2);
        ASSERT_TRUE(chunk.has_value());
        EXPECT_EQ(*chunk, "AA");
        EXPECT_EQ(peeker.tellg(), Position(0, 2));

        // Read remaining of first block (block_size - 2 bytes)
        chunk = peeker.readn(block_size - 2);
        ASSERT_TRUE(chunk.has_value());
        EXPECT_EQ(chunk->size(), block_size - 2);
        EXPECT_EQ(chunk->back(), 'A');
        EXPECT_EQ(peeker.tellg(), Position(1, 0));

        // Read 5 bytes from second block
        chunk = peeker.readn(5);
        ASSERT_TRUE(chunk.has_value());
        EXPECT_EQ(chunk->size(), 5);
        EXPECT_EQ(chunk->back(), 'B');
        EXPECT_EQ(peeker.tellg(), Position(1, 5));

        // Move to near end of second block (3 bytes before its end)
        peeker.seek_to(2 * block_size - 3);
        // Read 10 bytes: should get the remaining 3 bytes of second block,
        // and then cursor moves to start of third block (since seek_end crosses)
        chunk = peeker.readn(10);
        ASSERT_TRUE(chunk.has_value());
        EXPECT_EQ(chunk->size(), 3);
        EXPECT_EQ(chunk->back(), 'B');
        EXPECT_EQ(peeker.tellg(), Position(2, 0));  // now at third block start

        // Read entire third block (should get all block_size bytes)
        chunk = peeker.readn(block_size);
        ASSERT_TRUE(chunk.has_value());
        EXPECT_EQ(chunk->size(), block_size);
        EXPECT_EQ(chunk->back(), 'C');
        EXPECT_TRUE(peeker.eof());

        // Read after EOF
        chunk = peeker.readn(1);
        EXPECT_FALSE(chunk.has_value());
    }

    TEST_F(PeekerIOBufTest, ConversionToStringView) {
        TestPeeker peeker(&_buf);
        size_t block_size = TestIOBuf::kBlockSize;
        peeker.seek_to(block_size + 2); // within second block
        std::string_view sv = peeker;
        EXPECT_EQ(sv.size(), block_size - 2); // from that offset to end
        EXPECT_EQ(sv[0], 'B');
        peeker.seek_end(block_size); // to third block start
        sv = peeker;
        EXPECT_EQ(sv, std::string(block_size -2, 'C'));
    }

    TEST_F(PeekerIOBufTest, BoolAndEof) {
        TestPeeker peeker(&_buf);
        EXPECT_TRUE(peeker);
        EXPECT_FALSE(peeker.eof());
        peeker.seek_to(3 * TestIOBuf::kBlockSize);
        EXPECT_FALSE(peeker);
        EXPECT_TRUE(peeker.eof());
    }

    TEST_F(PeekerIOBufTest, HasRead) {
        TestPeeker peeker(&_buf);
        EXPECT_EQ(peeker.has_read(), 0);
        peeker.seek_to(500);
        EXPECT_EQ(peeker.has_read(), 500);
        peeker.readn(100);
        EXPECT_EQ(peeker.has_read(), 600);
        peeker.seek_start(50);
        EXPECT_EQ(peeker.has_read(), 550);
        peeker.reset();
        EXPECT_EQ(peeker.has_read(), 0);
    }

    // -----------------------------------------------------------------------------
    // Tests for Peeker<std::string_view> (and similar)
    // -----------------------------------------------------------------------------
    class PeekerStringViewTest : public ::testing::Test {
    protected:
        void SetUp() override {
            _data = "abcdefghijklmnopqrstuvwxyz";
        }

        std::string _data;
    };

    TEST_F(PeekerStringViewTest, ConstructionAndBasicAccess) {
        StringViewPeeker peeker(_data);
        EXPECT_EQ(peeker.tellg(), Position(0, 0));
        EXPECT_EQ(*peeker, 'a');

        peeker.set_buffer("hello");
        EXPECT_EQ(*peeker, 'h');
        peeker.reset();
        EXPECT_EQ(*peeker, 'h');
    }

    TEST_F(PeekerStringViewTest, SeekAndCursor) {
        StringViewPeeker peeker(_data);
        peeker.seek_to(10);
        EXPECT_EQ(peeker.tellg(), Position(0, 10));
        EXPECT_EQ(*peeker, 'k');
        peeker.seek_end(5);
        EXPECT_EQ(peeker.tellg(), Position(0, 15));
        EXPECT_EQ(*peeker, 'p');
        peeker.seek_start(3);
        EXPECT_EQ(peeker.tellg(), Position(0, 12));
        EXPECT_EQ(*peeker, 'm');
    }

    TEST_F(PeekerStringViewTest, FindFirst) {
        StringViewPeeker peeker(_data);
        auto pos = peeker.find_first_position("xyz");
        EXPECT_EQ(pos, Position(0, 23));
        size_t off = peeker.find_first_offset("xyz");
        EXPECT_EQ(off, 23);

        peeker.seek_to(20);
        pos = peeker.find_first_position("abc");
        EXPECT_EQ(pos, TestPeeker::npos);
        off = peeker.find_first_offset("abc");
        EXPECT_EQ(off, StringViewPeeker::kNPos);
    }

    TEST_F(PeekerStringViewTest, Readn) {
        StringViewPeeker peeker(_data);
        auto chunk = peeker.readn(5);
        ASSERT_TRUE(chunk);
        EXPECT_EQ(*chunk, "abcde");
        EXPECT_EQ(peeker.tellg(), Position(0, 5));
        chunk = peeker.readn(100);
        ASSERT_TRUE(chunk);
        EXPECT_EQ(chunk->size(), 21);
        EXPECT_TRUE(peeker.eof());
        chunk = peeker.readn(1);
        EXPECT_FALSE(chunk);
    }

    TEST_F(PeekerStringViewTest, ConversionAndBool) {
        StringViewPeeker peeker(_data);
        peeker.seek_to(5);
        std::string_view sv = peeker;
        EXPECT_EQ(sv, "fghijklmnopqrstuvwxyz");
        EXPECT_TRUE(peeker);
        peeker.seek_to(100);
        EXPECT_FALSE(peeker);
    }

    // -----------------------------------------------------------------------------
    // Tests for Peeker<std::string>
    // -----------------------------------------------------------------------------
    TEST(PeekerStringTest, Basic) {
        std::string str = "The quick brown fox";
        StringPeeker peeker(str);
        EXPECT_EQ(*peeker, 'T');
        peeker.seek_to(4);
        EXPECT_EQ(*peeker, 'q');
        auto chunk = peeker.readn(5);
        ASSERT_TRUE(chunk);
        EXPECT_EQ(*chunk, "quick");
        EXPECT_EQ(peeker.tellg(), Position(0, 9));
    }

    // -----------------------------------------------------------------------------
    // Tests for Peeker<Buffer<char, N>> and Peeker<Vector<char, N>>
    // -----------------------------------------------------------------------------
    TEST(PeekerBufferTest, Basic) {
        Buffer<char, 64> buf;
        std::string_view str = "hello world";
        buf.append(str.data(), str.size());
        BufferPeeker peeker(buf);
        EXPECT_EQ(*peeker, 'h');
        peeker.seek_to(6);
        EXPECT_EQ(*peeker, 'w');
        EXPECT_EQ(peeker.readn(3).value(), "wor");
    }

    TEST(PeekerVectorTest, Basic) {
        std::vector<char> vec = {'a', 'b', 'c', 'd', 'e'};
        VectorPeeker peeker(vec);
        EXPECT_EQ(*peeker, 'a');
        peeker.seek_to(2);
        EXPECT_EQ(*peeker, 'c');
        auto chunk = peeker.readn(2);
        ASSERT_TRUE(chunk);
        EXPECT_EQ(*chunk, "cd");
    }

    TEST(PeekerAlignedVectorTest, Basic) {
        Vector<char, 64> vec;
        vec.push_back('x');
        vec.push_back('y');
        vec.push_back('z');
        AlignedVectorPeeker peeker(vec);
        EXPECT_EQ(*peeker, 'x');
        peeker.seek_to(2);
        EXPECT_EQ(*peeker, 'z');
    }
} // namespace fermat
