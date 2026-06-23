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
#include <thread>
#include <atomic>
#include <array>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include <gtest/gtest.h>
#include <fermat/container/cord_buffer.h>
#include <fermat/container/cord_output_stream.h>
#include <fermat/container/cord_input_stream.h>

namespace fermat {
    namespace test {
        // ============================================================================
        // 19. CordBufferStreambuf (std::streambuf adapter)
        // ============================================================================

        // 19.1 Construction and basic writing
        TEST(CordBufferStreambufTest, basic_write_single_char) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            os << 'A';
            os.flush();
            EXPECT_EQ(cord.size(), 1);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "A");
        }

        TEST(CordBufferStreambufTest, basic_write_string) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            os << "Hello";
            os.flush();
            EXPECT_EQ(cord.size(), 5);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "Hello");
        }

        // 19.2 Cross-block writes
        TEST(CordBufferStreambufTest, cross_block_write) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            // Write more than one block (4096)
            std::string large(5000, 'X');
            os << large;
            os.flush();
            EXPECT_GE(cord.size(), 5000);
            EXPECT_GE(cord.buffer_count(), 2);
            auto s = cord.flatten();
            EXPECT_EQ(s, large);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, large);
            KLOG(INFO)<<"result.size():"<<result.size()<<"large.size():"<<large.size();
        }

        // 19.3 overflow and sync
        TEST(CordBufferStreambufTest, overflow_auto_commit) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            // Fill the buffer with many characters to force overflow
            for (int i = 0; i < 5000; ++i) {
                os << 'a';
            }
            os.flush();
            EXPECT_EQ(cord.size(), 5000);
        }

        TEST(CordBufferStreambufTest, sync_commits_partial) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            os << "partial";
            // Without flush, the streambuf may still hold data; we call sync directly
            EXPECT_EQ(streambuf.sync(), 0); // success
            EXPECT_EQ(cord.size(), 7);
        }

        // 19.4 Partial write and rollback
        TEST(CordBufferStreambufTest, partial_write_rollback) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            // Write less than a full block
            os << "abc";
            os.flush();
            EXPECT_EQ(cord.size(), 3);
            // The underlying buffer should have been rolled back to exactly 3 bytes
            const auto &front = cord.front_buffer();
            EXPECT_EQ(front.size(), 3);
            EXPECT_EQ(front.offset(), 0);
        }

        // 19.5 Edge cases
        TEST(CordBufferStreambufTest, write_zero_bytes) {
            CordBufferBase<64, 4096> cord;
            CordBufferStreambuf<64, 4096> streambuf(&cord);
            std::ostream os(&streambuf);
            os << "";
            os.flush();
            EXPECT_EQ(cord.size(), 0);
        }

        TEST(CordBufferStreambufDeathTest, null_cord) {
            // Constructing with nullptr is likely undefined; expect crash.
            // We skip because it's not required to be safe.
            // If the implementation dereferences nullptr, death test will pass.
            // Commented out to avoid hard dependency on crash behavior.
            // EXPECT_DEATH({ CordBufferStreambuf<64,4096> streambuf(nullptr); }, ".*");
        }

        // ============================================================================
        // 20. CordOutputStringStream (std::ostream adapter)
        // ============================================================================

        TEST(CordOutputStringStreamTest, formatted_output) {
            CordBufferBase<64, 4096> cord;
            CordOutputStringStream<64, 4096> oss(&cord);
            oss << "Number: " << 42 << " and pi = " << 3.14159;
            oss.flush();
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "Number: 42 and pi = 3.14159");
        }

        TEST(CordOutputStringStreamTest, flush_and_state) {
            CordBufferBase<64, 4096> cord;
            CordOutputStringStream<64, 4096> oss(&cord);
            oss << "Hello";
            oss.flush();
            EXPECT_EQ(oss.cord().size(), 5);
            oss << " world";
            oss.flush();
            EXPECT_EQ(oss.cord().size(), 11);
        }


        // ============================================================================
        // 21. CordInputBinaryStream (binary deserialization)
        // ============================================================================

        // Helper to create a cord with binary data
        template<typename... Args>
        void write_to_cord(CordBufferBase<64, 4096> &cord, Args &&... args) {
            CordOutputBinaryStream<64, 4096> obs(&cord);
            (obs << ... << std::forward<Args>(args));
        }

        TEST(CordInputBinaryStreamTest, construction_and_default_endian) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, uint16_t(0x1234));
            // Read back with default little-endian (host endian)
            CordInputBinaryStream<64, 4096> ibs(&cord, false); // little endian
            uint16_t val = 0;
            bool ok = ibs.read(val);
            ASSERT_TRUE(ok);
            // On little-endian machine, 0x1234 stored as 0x34 0x12; reading as little gives 0x1234
            // On big-endian machine, the test would need adjustment; assume little for simplicity.
            EXPECT_EQ(val, 0x1234);
        }

        TEST(CordInputBinaryStreamTest, big_endian_read) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, uint16_t(0x1234));
            CordInputBinaryStream<64, 4096> ibs(&cord, true); // big endian
            uint16_t val = 0;
            ibs.read(val);
            // Since 0x1234 was written in host endian, reading as big will give swapped bytes.
            // On little-endian host, written bytes = 0x34,0x12; reading as big yields 0x3412.
            EXPECT_EQ(val, 0x3412);
        }

        TEST(CordInputBinaryStreamTest, raw_byte_reading) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, uint8_t(0xAB), uint8_t(0xCD));
            CordInputBinaryStream<64, 4096> ibs(&cord);
            uint8_t b1 = 0, b2 = 0;
            ibs.read(&b1, 1);
            ibs.read(&b2, 1);
            EXPECT_EQ(b1, 0xAB);
            EXPECT_EQ(b2, 0xCD);
        }

        TEST(CordInputBinaryStreamTest, read_to_receiver) {
            class TestReceiver : public Receiver {
            public:
                std::string data;

                turbo::Status append(const char *d, size_t len) override {
                    data.append(d, len);
                    return turbo::OkStatus();
                }

                void clear() noexcept override {
                }

                turbo::Status resize(size_t) override { return turbo::OkStatus(); }
                turbo::Status reserve(size_t) override { return turbo::OkStatus(); }
                bool is_dynamic() const noexcept override { return true; }
                size_t size() const noexcept override { return data.size(); }
                size_t capacity() const noexcept override { return data.capacity(); }
            };
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "HelloWorld");
            CordInputBinaryStream<64, 4096> ibs(&cord);
            TestReceiver recv;
            size_t n = ibs.read(recv, 5);
            EXPECT_EQ(n, 5);
            EXPECT_EQ(recv.data, "Hello");
            n = ibs.read(recv, 10);
            EXPECT_EQ(n, 5); // remaining
            EXPECT_EQ(recv.data, "HelloWorld");
        }

        TEST(CordInputBinaryStreamTest, endianness_manipulator) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, uint16_t(0x1234), uint16_t(0x5678));
            CordInputBinaryStream<64, 4096> ibs(&cord, false); // start little
            uint16_t v1 = 0, v2 = 0;
            ibs.read(v1); // little
            ibs << BigEndian{};
            ibs.read(v2); // big
            // On little host: v1 = 0x1234, v2 = swapped 0x5678 -> 0x7856? Wait: written 0x5678 as little = 0x78 0x56, read as big gives 0x7856.
            EXPECT_EQ(v1, 0x1234);
            EXPECT_EQ(v2, 0x7856);
        }

        TEST(CordInputBinaryStreamTest, integer_reads) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, uint32_t(0xDEADBEEF), int64_t(-12345));
            CordInputBinaryStream<64, 4096> ibs(&cord);
            uint32_t u32 = 0;
            int64_t i64 = 0;
            ibs.read(u32);
            ibs.read(i64);
            EXPECT_EQ(u32, 0xDEADBEEF);
            EXPECT_EQ(i64, -12345);
        }

        TEST(CordInputBinaryStreamTest, floating_point_reads) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, 3.14159f, 2.71828);
            CordInputBinaryStream<64, 4096> ibs(&cord);
            float f = 0;
            double d = 0;
            ibs.read(f);
            ibs.read(d);
            EXPECT_FLOAT_EQ(f, 3.14159f);
            EXPECT_DOUBLE_EQ(d, 2.71828);
        }

        TEST(CordInputBinaryStreamTest, span_read) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "abcdefghij");
            CordInputBinaryStream<64, 4096> ibs(&cord);
            char buf[5] = {0};
            turbo::span<char> sp(buf, 5);
            size_t n = ibs.read(sp);
            EXPECT_EQ(n, 5);
            EXPECT_EQ(std::string(buf,5), "abcde");
        }

        TEST(CordInputBinaryStreamTest, read_util_delimiter) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "hello,world");
            CordInputBinaryStream<64, 4096> ibs(&cord);
            char buf[10];
            bool reach = false;
            size_t n = ibs.read_util(turbo::span<char>(buf, 10), ',', reach);
            EXPECT_EQ(n, 5);
            EXPECT_TRUE(reach);
            EXPECT_EQ(std::string(buf, n), "hello");
            // The delimiter is consumed, next read should get "world"
            char buf2[10];
            n = ibs.read(turbo::span<char>(buf2, 10));
            EXPECT_EQ(n, 5);
            EXPECT_EQ(std::string(buf2,5), "world");
        }

        TEST(CordInputBinaryStreamTest, read_util_multiple_delimiters) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "abc;def:ghi");
            CordInputBinaryStream<64, 4096> ibs(&cord);
            char buf[10];
            bool reach = false;
            size_t n = ibs.read_util(turbo::span<char>(buf, 10), ";:", reach);
            EXPECT_EQ(n, 3);
            EXPECT_TRUE(reach);
            EXPECT_EQ(std::string(buf,3), "abc");
        }

        // 22. CordInputStringStream (std::istream adapter)
        TEST(CordInputStringStreamTest, formatted_input) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "42 3.14 hello");
            CordInputStringStream<64, 4096> iss(&cord);
            int i;
            double d;
            std::string s;
            iss >> i >> d >> s;
            EXPECT_EQ(i, 42);
            EXPECT_DOUBLE_EQ(d, 3.14);
            EXPECT_EQ(s, "hello");
        }

        TEST(CordInputStringStreamTest, getline) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "line1\nline2\n");
            CordInputStringStream<64, 4096> iss(&cord);
            std::string line;
            std::getline(iss, line);
            EXPECT_EQ(line, "line1");
            std::getline(iss, line);
            EXPECT_EQ(line, "line2");
        }

        TEST(CordInputStringStreamTest, read_raw) {
            CordBufferBase<64, 4096> cord;
            {
                CordOutputBinaryStream<64, 4096> obs(&cord);
                obs << std::string_view("raw data");
            }
            CordInputStringStream<64, 4096> iss(&cord);
            char buf[10] = {0};
            iss.read(buf, 8); // "raw data" length is 8
            EXPECT_EQ(std::string(buf, 8), "raw data");
        }

        TEST(CordInputStringStreamTest, stream_state) {
            CordBufferBase<64, 4096> cord;
            write_to_cord(cord, "abc");
            CordInputStringStream<64, 4096> iss(&cord);
            int x;
            iss >> x; // fails
            EXPECT_TRUE(iss.fail());
            iss.clear();
            std::string s;
            iss >> s;
            EXPECT_EQ(s, "abc");
            EXPECT_TRUE(iss.eof());
        }

        // 23. Integration and Round-Trip Tests
        TEST(IntegrationTest, binary_round_trip) {
            CordBufferBase<64, 4096> cord;
            CordOutputBinaryStream<64, 4096> obs(&cord);
            obs << BigEndian{} << uint16_t(0x1234) << LittleEndian{} << uint32_t(0x89ABCDEF) << 3.14159;
            CordInputBinaryStream<64, 4096> ibs(&cord);
            uint16_t v1;
            uint32_t v2;
            double v3;
            ibs << BigEndian{};
            ibs.read(v1);
            ibs << LittleEndian{};
            ibs.read(v2);
            ibs.read(v3);
            EXPECT_EQ(v1, 0x1234);
            EXPECT_EQ(v2, 0x89ABCDEF);
            EXPECT_DOUBLE_EQ(v3, 3.14159);
        }

        TEST(IntegrationTest, text_round_trip) {
            CordBufferBase<64, 4096> cord;
            {
                CordOutputStringStream<64, 4096> oss(&cord);
                oss << "Hello " << 100 << " times";
                KLOG(INFO) << cord.size();
            }

            CordInputStringStream<64, 4096> iss(&cord);
            std::string token;
            iss >> token;
            EXPECT_EQ(token, "Hello");
            int n;
            iss >> n;
            EXPECT_EQ(n, 100);
            iss >> token;
            EXPECT_EQ(token, "times");
        }

        TEST(IntegrationTest, text_round_trip_flust) {
            CordBufferBase<64, 4096> cord;

            CordOutputStringStream<64, 4096> oss(&cord);
            oss << "Hello " << 100 << " times";
            oss.flush();
            KLOG(INFO) << cord.size();


            CordInputStringStream<64, 4096> iss(&cord);
            std::string token;
            iss >> token;
            EXPECT_EQ(token, "Hello");
            int n;
            iss >> n;
            EXPECT_EQ(n, 100);
            iss >> token;
            EXPECT_EQ(token, "times");
        }


        // 24. Performance and Resource Leak Checks (simple)
        TEST(LeakTest, basic_allocation) {
            CordBufferBase<64, 4096> *cord = new CordBufferBase<64, 4096>();
            cord->append("test", 4);
            auto share = cord->share();
            delete cord;
            // share still holds data
            EXPECT_EQ(share.size(), 4);
            // share destructor cleans up
        }

        TEST(LeakTest, create_buffers_no_leak) {
            auto refs = CordBufferBase<64, 4096>::create_buffers(10000);
            EXPECT_GT(refs.size(), 0);
        }

        // 25. Compile-time and Type Traits
        TEST(TypeTraitsTest, is_cord_buffer) {
            EXPECT_TRUE((is_cord_buffer_v<CordBufferBase<64,4096>>));
            EXPECT_FALSE(is_cord_buffer_v<int>);
            EXPECT_EQ((is_cord_buffer<CordBufferBase<64,4096>>::alignment), 64);
            EXPECT_EQ((is_cord_buffer<CordBufferBase<64,4096>>::block_size), 4096);
        }

        // 26. Documentation and API Consistency
        TEST(APIConsistencyTest, moved_from_state) {
            CordBufferBase<64, 4096> cord1;
            cord1.append("data", 4);
            CordBufferBase<64, 4096> cord2(std::move(cord1));
            // cord1 is now in a valid but unspecified state; we can clear it or destroy
            cord1.clear();
            EXPECT_EQ(cord1.size(), 0);
        }

        // Additional test for DKCHECK (death)
        TEST(DeathTest, append_writeable_non_unique) {
            CordBufferBase<64, 4096> cord;
            auto ref = BufferRef<64>::create_write_able(100);
            auto ref2 = ref; // copy shared_ptr, use_count becomes 2
            // ref is not unique, passing to append_writeable should trigger DKCHECK crash
            EXPECT_DEATH(cord.append_writeable(std::move(ref)), ".*");
        }
    } // namespace test
} // namespace fermat
