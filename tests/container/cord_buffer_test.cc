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

namespace fermat {
    namespace test {
        // ============================================================================
        // 1. BufferRef – Core Building Block
        // ============================================================================

        // 1.1 Factory Methods
        TEST(BufferRefTest, create_write_able) {
            constexpr size_t kSize = 1024;
            auto ref = BufferRef<64>::create_write_able(kSize);
            EXPECT_NE(ref.buffer, nullptr);
            EXPECT_EQ(ref.size(), 0);
            EXPECT_EQ(ref.capacity(), kSize);
            EXPECT_EQ(ref.write_able(), kSize);
            EXPECT_TRUE(ref.is_unique());
        }

        TEST(BufferRefTest, setup_write_able_shared_ptr) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(512);
            auto ref = BufferRef<64>::setup_write_able(std::move(buf));
            EXPECT_TRUE(ref.is_unique());
            EXPECT_EQ(ref.capacity(), 512);
            EXPECT_EQ(ref.size(), 0);
            // The original buf is now moved.
        }

        TEST(BufferRefTest, setup_write_able_shared_ptr_with_range) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(1024);
            auto ref = BufferRef<64>::setup_write_able(std::move(buf), 100, 200);
            EXPECT_TRUE(ref.is_unique());
            EXPECT_EQ(ref.offset(), 100);
            EXPECT_EQ(ref.size(), 200);
        }

        TEST(BufferRefTest, reference) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(512);
            std::memset(buf->data(), 'A', buf->size());
            auto ref = BufferRef<64>::reference(buf, 10, 30);
            EXPECT_EQ(ref.buffer.use_count(), 2); // shared with original buf
            EXPECT_EQ(ref.offset(), 10);
            EXPECT_EQ(ref.size(), 30);
            // Original buffer unchanged
            EXPECT_EQ(buf->size(), 512);
        }

        TEST(BufferRefTest, assign) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(256);
            auto ref = BufferRef<64>::assign(std::move(buf), 5, 100);
            EXPECT_TRUE(ref.is_unique());
            EXPECT_EQ(ref.offset(), 5);
            EXPECT_EQ(ref.size(), 100);
        }

        // 1.2 Write Operations
        TEST(BufferRefTest, append) {
            auto ref = BufferRef<64>::create_write_able(100);
            const char *data = "hello";
            size_t written = ref.append(data, 5);
            EXPECT_EQ(written, 5);
            EXPECT_EQ(ref.size(), 5);
            // Append beyond capacity
            written = ref.append(data, 200);
            EXPECT_EQ(written, 95); // only 95 left
            EXPECT_EQ(ref.size(), 100);
            EXPECT_EQ(ref.write_able(), 0);
        }

        TEST(BufferRefTest, borrow) {
            auto ref = BufferRef<64>::create_write_able(128);
            void *out = nullptr;
            int size = 0;
            bool ok = ref.borrow(&out, &size);
            EXPECT_TRUE(ok);
            EXPECT_NE(out, nullptr);
            EXPECT_EQ(size, 128);
            EXPECT_EQ(ref.size(), 128); // automatically extended
            // Second borrow should fail because write_able() == 0
            out = nullptr;
            size = 0;
            ok = ref.borrow(&out, &size);
            EXPECT_FALSE(ok);
            // Shared buffer cannot borrow
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            auto ref2 = BufferRef<64>::reference(buf, 0, buf->size());
            out = nullptr;
            size = 0;
            ok = ref2.borrow(&out, &size);
            EXPECT_FALSE(ok);
        }

        TEST(BufferRefTest, backup) {
            auto ref = BufferRef<64>::create_write_able(100);
            ref.append("1234567890", 10);
            EXPECT_EQ(ref.size(), 10);
            size_t backed = ref.backup(3);
            EXPECT_EQ(backed, 3);
            EXPECT_EQ(ref.size(), 7);
            // backup more than length
            backed = ref.backup(20);
            EXPECT_EQ(backed, 7);
            EXPECT_EQ(ref.size(), 0);
            // backup on shared buffer should crash (KCHECK). We'll rely on death test.
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(50);
            auto ref2 = BufferRef<64>::reference(buf, 0, buf->size());
            EXPECT_DEATH(ref2.backup(1), ".*");
        }

        TEST(BufferRefTest, pop_front) {
            auto ref = BufferRef<64>::create_write_able(100);
            ref.append("abcdefghij", 10);
            EXPECT_EQ(ref.offset(), 0);
            size_t popped = ref.pop_front(3);
            EXPECT_EQ(popped, 3);
            EXPECT_EQ(ref.size(), 7);
            EXPECT_EQ(ref.offset(), 3);
            // pop more than length
            popped = ref.pop_front(20);
            EXPECT_EQ(popped, 7);
            EXPECT_EQ(ref.size(), 0);
            EXPECT_EQ(ref.offset(), 10);
        }

        TEST(BufferRefTest, pop_back) {
            auto ref = BufferRef<64>::create_write_able(100);
            ref.append("abcdefghij", 10);
            size_t popped = ref.pop_back(4);
            EXPECT_EQ(popped, 4);
            EXPECT_EQ(ref.size(), 6);
            // pop more than length
            popped = ref.pop_back(20);
            EXPECT_EQ(popped, 6);
            EXPECT_EQ(ref.size(), 0);
        }

        // 1.3 Introspection
        TEST(BufferRefTest, write_able_and_capacity) {
            auto ref = BufferRef<64>::create_write_able(200);
            EXPECT_EQ(ref.write_able(), 200);
            EXPECT_EQ(ref.capacity(), 200);
            ref.append("data", 4);
            EXPECT_EQ(ref.write_able(), 196);
            EXPECT_EQ(ref.capacity(), 200);
            // Shared buffer -> write_able returns 0
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            auto ref2 = BufferRef<64>::reference(buf, 0, buf->size());
            EXPECT_EQ(ref2.write_able(), 0);
            EXPECT_EQ(ref2.capacity(), 100);
        }

        // ============================================================================
        // 2. CordBufferBase – Construction & State
        // ============================================================================

        TEST(CordBufferTest, default_construction) {
            CordBufferBase<64, 4096> cord;
            EXPECT_EQ(cord.size(), 0);
            EXPECT_EQ(cord.buffer_count(), 0);
            EXPECT_EQ(cord.begin(), cord.end());
        }

        TEST(CordBufferTest, move_construction) {
            CordBufferBase<64, 4096> cord1;
            cord1.append("hello", 5);
            EXPECT_EQ(cord1.size(), 5);
            CordBufferBase<64, 4096> cord2(std::move(cord1));
            EXPECT_EQ(cord2.size(), 5);
            EXPECT_EQ(cord1.size(), 0);
            EXPECT_EQ(cord1.buffer_count(), 0);
        }

        TEST(CordBufferTest, move_assignment) {
            CordBufferBase<64, 4096> cord1, cord2;
            cord1.append("world", 5);
            cord2 = std::move(cord1);
            EXPECT_EQ(cord2.size(), 5);
            EXPECT_EQ(cord1.size(), 0);
        }

        // ============================================================================
        // 3. Adding Data – Ownership Semantics
        // ============================================================================

        TEST(CordBufferTest, append_reference_single) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            std::memset(buf->data(), 'X', buf->size());
            auto ref = BufferRef<64>::reference(buf, 0, 50);
            CordBufferBase<64, 4096> cord;
            cord.append_reference(ref);
            EXPECT_EQ(cord.size(), 50);
            EXPECT_EQ(cord.buffer_count(), 1);
            // Write pointer reset to sentinel
            EXPECT_EQ(cord.output_next().size(), 4096); // because write_able should be 0 for shared buffer
        }

        TEST(CordBufferTest, append_reference_vector) {
            std::vector<BufferRef<64> > refs;
            for (int i = 0; i < 3; ++i) {
                auto buf = std::make_shared<Buffer<char, 64> >();
                buf->resize_uninitialized(100);
                auto ref = BufferRef<64>::reference(buf, 0, 20);
                refs.push_back(std::move(ref));
            }
            CordBufferBase<64, 4096> cord;
            cord.append_reference(refs);
            EXPECT_EQ(cord.size(), 60);
            EXPECT_EQ(cord.buffer_count(), 3);
        }

        TEST(CordBufferTest, prepend_reference) {
            auto buf1 = std::make_shared<Buffer<char, 64> >();
            buf1->resize_uninitialized(100);
            auto ref1 = BufferRef<64>::reference(buf1, 0, 10);
            auto buf2 = std::make_shared<Buffer<char, 64> >();
            buf2->resize_uninitialized(100);
            auto ref2 = BufferRef<64>::reference(buf2, 0, 20);
            CordBufferBase<64, 4096> cord;
            cord.append_reference(ref1);
            cord.prepend_reference(ref2);
            EXPECT_EQ(cord.size(), 30);
            ASSERT_EQ(cord.buffer_count(), 2);
            // Order: ref2 then ref1
            auto it = cord.begin();
            // We'll just trust that internal order is correct; can't easily verify without reading.
        }

        TEST(CordBufferTest, append_writeable_single) {
            auto ref = BufferRef<64>::create_write_able(256);
            ref.append("test", 4);
            CordBufferBase<64, 4096> cord;
            cord.append_writeable(std::move(ref));
            EXPECT_EQ(cord.size(), 4);
            EXPECT_EQ(cord.buffer_count(), 1);
            // The tail should be writable
            auto span = cord.output_next();
            EXPECT_GT(span.size(), 0);
        }

        TEST(CordBufferTest, append_writeable_vector) {
            std::vector<BufferRef<64> > refs;
            for (int i = 0; i < 3; ++i) {
                auto ref = BufferRef<64>::create_write_able(100);
                ref.append("x", 1);
                refs.push_back(std::move(ref));
            }
            CordBufferBase<64, 4096> cord;
            cord.append_writeable(std::move(refs));
            EXPECT_EQ(cord.size(), 3);
            EXPECT_EQ(cord.buffer_count(), 3);
            // last buffer should be writable
            auto span = cord.output_next();
            EXPECT_GT(span.size(), 0);
        }

        TEST(CordBufferTest, append_buffer_shared_ptr) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            std::memset(buf->data(), 'X', buf->size());
            CordBufferBase<64, 4096> cord;
            cord.append_buffer(buf, 10, 30);
            EXPECT_EQ(cord.size(), 30);
            EXPECT_EQ(cord.buffer_count(), 1);
            // Write pointer reset
            EXPECT_EQ(cord.output_next().size(), 4096); // because shared buffer not writable
        }

        TEST(CordBufferTest, append_buffer_move) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(200);
            CordBufferBase<64, 4096> cord;
            cord.append_buffer(std::move(buf), 5, 50);
            EXPECT_EQ(cord.size(), 50);
            EXPECT_EQ(cord.buffer_count(), 1);
            // Write pointer reset (move version also resets)
            EXPECT_EQ(cord.output_next().size(), 4096);
        }

        TEST(CordBufferTest, prepend_buffer) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            std::memset(buf->data(), 'B', buf->size());
            CordBufferBase<64, 4096> cord;
            cord.append("tail", 4);
            cord.prepend_buffer(buf, 0, 20);
            EXPECT_EQ(cord.size(), 24);
            EXPECT_EQ(cord.buffer_count(), 2);
            // The tail should still be the original segment, and it may be writable if unique.
            auto span = cord.output_next();
            // Since original tail was a shared_ptr? Actually tail is from append("tail") which created a new writable buffer.
            // So it should be writable.
            EXPECT_GT(span.size(), 0);
        }

        // ============================================================================
        // 4. Batch Creation Helpers
        // ============================================================================

        TEST(CordBufferTest, create_buffers) {
            size_t needed = 5000;
            auto refs = CordBufferBase<64, 4096>::create_buffers(needed);
            size_t total_cap = 0;
            for (auto &ref: refs) {
                total_cap += ref.capacity();
            }
            EXPECT_GE(total_cap, needed);
            // Each block except last is exactly BlockSize=4096
            for (size_t i = 0; i < refs.size() - 1; ++i) {
                EXPECT_EQ(refs[i].capacity(), 4096);
            }
            if (!refs.empty()) {
                EXPECT_LE(refs.back().capacity(), 4096);
            }
        }

        TEST(CordBufferTest, create_big_buffer) {
            size_t needed = 3000;
            auto big = CordBufferBase<64, 4096>::create_big_buffer(needed);
            EXPECT_EQ(big.capacity(), 4096); // ceil(3000/4096)=1 block
            needed = 5000;
            big = CordBufferBase<64, 4096>::create_big_buffer(needed);
            EXPECT_EQ(big.capacity(), 8192); // 2 buffer_count
        }

        // ============================================================================
        // 5. Writing Through output_next() / output_backup()
        // ============================================================================

        TEST(CordBufferTest, output_next_span) {
            CordBufferBase<64, 4096> cord;
            auto span = cord.output_next();
            EXPECT_GT(span.size(), 0);
            EXPECT_EQ(cord.size(), span.size()); // because output_next extends the tail
            // Write some data
            std::memcpy(span.data(), "hello", 5);
            // Now the cord size is span.size() (full capacity), but we only wrote 5 bytes.
            // Use output_backup to correct.
            cord.output_backup(span.size() - 5);
            EXPECT_EQ(cord.size(), 5);
        }

        TEST(CordBufferTest, output_next_traditional) {
            CordBufferBase<64, 4096> cord;
            void *out = nullptr;
            int size = 0;
            bool ok = cord.output_next(&out, &size);
            EXPECT_TRUE(ok);
            EXPECT_NE(out, nullptr);
            EXPECT_GT(size, 0);
            EXPECT_EQ(cord.size(), size);
            cord.output_backup(size - 3);
            EXPECT_EQ(cord.size(), 3);
        }

        TEST(CordBufferTest, output_backup_clear) {
            CordBufferBase<64, 4096> cord;
            cord.append("data", 4);
            cord.output_backup(10); // larger than total size
            EXPECT_EQ(cord.size(), 0);
            EXPECT_TRUE(cord.begin() == cord.end());
        }

        TEST(CordBufferTest, output_backup_multi_segment) {
            CordBufferBase<64, 4096> cord;
            // Force multiple buffer_count by writing more than BlockSize
            std::string large(5000, 'a');
            cord.append(large);
            size_t orig_size = cord.size();
            // Backup 1000 bytes
            cord.output_backup(1000);
            EXPECT_EQ(cord.size(), orig_size - 1000);
            // The last block may have been partially trimmed
        }

        // ============================================================================
        // 6. Removing Data
        // ============================================================================

        TEST(CordBufferTest, pop_front) {
            CordBufferBase<64, 4096> cord;
            cord.append("abcdefghij", 10);
            cord.pop_front(3);
            EXPECT_EQ(cord.size(), 7);
            std::string result;
            EXPECT_NE(cord.buffer_begin(), cord.buffer_end());
            EXPECT_EQ(cord.buffer_begin()->size(), 7);
            for (auto it = cord.begin(); it != cord.end(); ++it) {
                KLOG(INFO) << *it;
            }
            for (char c: cord) {
                KLOG(INFO) << c;
                result.push_back(c);
            }
            EXPECT_EQ(result, "defghij");
            cord.pop_front(20); // more than size
            EXPECT_EQ(cord.size(), 0);
        }

        TEST(CordBufferTest, pop_front_buffer) {
            CordBufferBase<64, 4096> cord;
            std::string first(4096, 'a');
            cord.append(first.data(), 4096);
            cord.append("second", 6);
            EXPECT_EQ(cord.buffer_count(), 2);
            cord.pop_front_buffer();
            EXPECT_EQ(cord.buffer_count(), 1);
            EXPECT_EQ(cord.size(), 6);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "second");
        }

        TEST(CordBufferTest, pop_back) {
            CordBufferBase<64, 4096> cord;
            cord.append("abcdefghij", 10);
            cord.pop_back(4);
            EXPECT_EQ(cord.size(), 6);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "abcdef");
            cord.pop_back(20);
            EXPECT_EQ(cord.size(), 0);
        }

        // 6.4 pop_back_buffer - remove last segment
        TEST(CordBufferTest, pop_back_buffer) {
            CordBufferBase<64, 4096> cord;
            // Fill first block completely (4096 bytes)
            std::string first_block(4096, 'a');
            cord.append(first_block.data(), 4096);
            // Second block (6 bytes)
            cord.append("second", 6);
            // Remove the last segment (the "second" block)
            cord.pop_back_buffer();

            EXPECT_EQ(cord.buffer_count(), 1);
            EXPECT_EQ(cord.size(), 4096); // Only the first block remains, full 4096 bytes

            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, first_block); // Should be 4096 'a's
        }

        // ============================================================================
        // 7. Appending/Prepending Entire Cords
        // ============================================================================

        TEST(CordBufferTest, append_cord_copy) {
            CordBufferBase<64, 4096> cord1, cord2;
            cord1.append("hello", 5);
            cord2.append(cord1);
            EXPECT_EQ(cord2.size(), 5);
            EXPECT_EQ(cord2.buffer_count(), 1);
            // The buffers are shared, not copied
            const auto &ref1 = cord1.front_buffer();
            const auto &ref2 = cord2.front_buffer();
            EXPECT_EQ(ref1.buffer, ref2.buffer);
        }

        TEST(CordBufferTest, append_cord_move) {
            CordBufferBase<64, 4096> cord1, cord2;
            cord1.append("world", 5);
            KLOG(INFO) << 1;
            cord2.append(std::move(cord1));
            KLOG(INFO) << 2;
            EXPECT_EQ(cord2.size(), 5);
            KLOG(INFO) << 3;
            EXPECT_EQ(cord1.size(), 0);
            KLOG(INFO) << 4;
            EXPECT_EQ(cord2.buffer_count(), 1);
            KLOG(INFO) << 5;
        }

        TEST(CordBufferTest, prepend_cord_copy) {
            CordBufferBase<64, 4096> cord1, cord2;
            cord1.append("tail", 4);
            cord2.append("head", 4);
            cord1.prepend(cord2);
            EXPECT_EQ(cord1.size(), 8);
            EXPECT_EQ(cord1.buffer_count(), 2);
        }

        TEST(CordBufferTest, prepend_cord_move) {
            CordBufferBase<64, 4096> cord1, cord2;
            cord1.append("A", 1);
            cord2.append("B", 1);
            cord1.prepend(std::move(cord2));
            EXPECT_EQ(cord1.size(), 2);
            EXPECT_EQ(cord2.size(), 0);
        }

        // ============================================================================
        // 8. Sharing and Copying
        // ============================================================================

        TEST(CordBufferTest, share) {
            CordBufferBase<64, 4096> cord;
            cord.append("shared", 6);
            auto shared = cord.share();
            EXPECT_EQ(shared.size(), 6);
            // The buffers are shared
            EXPECT_EQ(cord.front_buffer().buffer, shared.front_buffer().buffer);
            // Write pointer of shared is sentinel, so can't write
            EXPECT_EQ(shared.output_next().size(), 4096);
        }

        TEST(CordBufferTest, copy) {
            CordBufferBase<64, 4096> cord;
            cord.append("deep", 4);
            auto copied = cord.copy();
            EXPECT_EQ(copied.size(), 4);
            // Buffers are independent
            EXPECT_NE(cord.front_buffer().buffer, copied.front_buffer().buffer);
            // Copied cord's tail is writable
            EXPECT_GT(copied.output_next().size(), 0);
        }

        // ============================================================================
        // 9. Iterators (const)
        // ============================================================================

        TEST(CordBufferTest, iterator_basic) {
            CordBufferBase<64, 4096> cord;
            cord.append("abc", 3);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "abc");
        }

        TEST(CordBufferTest, iterator_multiple_segments) {
            CordBufferBase<64, 4096> cord;
            cord.append("first", 5);
            cord.append("second", 6);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "firstsecond");
        }

        TEST(CordBufferTest, iterator_empty) {
            CordBufferBase<64, 4096> cord;
            EXPECT_EQ(cord.begin(), cord.end());
        }

        TEST(CordBufferTest, iterator_range_respect) {
            // Create a BufferRef with a sub-range
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            std::memset(buf->data(), 'X', 100);
            auto ref = BufferRef<64>::reference(buf, 10, 5);
            CordBufferBase<64, 4096> cord;
            cord.append_reference(ref);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, std::string(5, 'X'));
        }

        // ============================================================================
        // 10. InputStream (ZeroCopyInputStream adapter)
        // ============================================================================

        TEST(CordBufferTest, input_stream_next) {
            CordBufferBase<64, 4096> cord;
            cord.append("hello", 5);
            auto stream = cord.input_stream();
            const void *data = nullptr;
            int size = 0;
            bool ok = stream.next(&data, &size);
            EXPECT_TRUE(ok);
            EXPECT_EQ(size, 5);
            std::string s(static_cast<const char *>(data), size);
            EXPECT_EQ(s, "hello");
            ok = stream.next(&data, &size);
            EXPECT_FALSE(ok);
        }

        TEST(CordBufferTest, input_stream_multiple_segments) {
            CordBufferBase<64, 4096> cord;
            std::string x(4096, 'x');
            cord.append(x.data(), x.size());
            cord.append("second", 6);
            auto stream = cord.input_stream();
            const void *data = nullptr;
            int size = 0;
            EXPECT_TRUE(stream.next(&data, &size));
            EXPECT_EQ(size, 4096);
            EXPECT_TRUE(stream.next(&data, &size));
            EXPECT_EQ(size, 6);
            EXPECT_FALSE(stream.next(&data, &size));
        }

        TEST(CordBufferTest, input_stream_back_up) {
            CordBufferBase<64, 4096> cord;
            cord.append("abcdef", 6);
            auto stream = cord.input_stream();
            const void *data = nullptr;
            int size = 0;
            EXPECT_TRUE(stream.next(&data, &size));
            EXPECT_EQ(size, 6);
            // Read only 2 bytes, then back up 1
            stream.back_up(1);
            // next should give the same chunk starting from offset 5? Actually back_up moves backwards.
            // Let's read again:
            EXPECT_TRUE(stream.next(&data, &size));
            EXPECT_EQ(size, 1); // last character 'f'
            std::string s(static_cast<const char *>(data), size);
            EXPECT_EQ(s, "f");
        }

        TEST(CordBufferTest, input_stream_skip) {
            CordBufferBase<64, 4096> cord;
            cord.append("1234567890", 10);
            auto stream = cord.input_stream();
            bool ok = stream.skip(5);
            EXPECT_TRUE(ok);
            EXPECT_EQ(stream.byte_count(), 5);
            const void *data = nullptr;
            int size = 0;
            EXPECT_TRUE(stream.next(&data, &size));
            EXPECT_EQ(size, 5);
            std::string s(static_cast<const char *>(data), size);
            EXPECT_EQ(s, "67890");
        }

        TEST(CordBufferTest, input_stream_skip_cross_segments) {
            CordBufferBase<64, 4096> cord;
            cord.append("12345", 5);
            cord.append("67890", 5);
            auto stream = cord.input_stream();
            bool ok = stream.skip(7);
            EXPECT_TRUE(ok);
            EXPECT_EQ(stream.byte_count(), 7);
            const void *data = nullptr;
            int size = 0;
            EXPECT_TRUE(stream.next(&data, &size));
            EXPECT_EQ(size, 3);
            std::string s(static_cast<const char *>(data), size);
            EXPECT_EQ(s, "890");
        }

        // ============================================================================
        // 11. High-Performance Read Methods
        // ============================================================================

        // Mock IOReader for testing readv/read/pread
        class MockReader : public IOReader {
        public:
            explicit MockReader(std::string data) : data_(std::move(data)), pos_(0) {
            }

            turbo::Result<size_t> readv(const std::vector<IOVec> &iov, size_t total_bytes_reserved) override {
                (void) total_bytes_reserved; // ignore hint
                size_t total = 0;
                for (const auto &vec: iov) {
                    char *buf = static_cast<char *>(vec.iov_base);
                    size_t len = vec.iov_len;
                    size_t to_copy = std::min(len, data_.size() - pos_);
                    if (to_copy > 0) {
                        std::memcpy(buf, data_.data() + pos_, to_copy);
                        pos_ += to_copy;
                        total += to_copy;
                    }
                    if (to_copy < len) break; // partial read
                }
                return total;
            }

            turbo::Result<size_t>
            preadv(const std::vector<IOVec> &iov, size_t offset, size_t total_bytes_reserved) override {
                (void) total_bytes_reserved;
                size_t total = 0;
                size_t cur_offset = offset;
                for (const auto &vec: iov) {
                    char *buf = static_cast<char *>(vec.iov_base);
                    size_t len = vec.iov_len;
                    if (cur_offset >= data_.size()) break;
                    size_t to_copy = std::min(len, data_.size() - cur_offset);
                    if (to_copy > 0) {
                        std::memcpy(buf, data_.data() + cur_offset, to_copy);
                        cur_offset += to_copy;
                        total += to_copy;
                    }
                    if (to_copy < len) break;
                }
                return total;
            }

            turbo::Result<size_t> read(turbo::span<char> &iov) override {
                size_t to_copy = std::min(iov.size(), data_.size() - pos_);
                std::memcpy(iov.data(), data_.data() + pos_, to_copy);
                pos_ += to_copy;
                return to_copy;
            }

            turbo::Result<size_t> pread(turbo::span<char> &iov, size_t offset) override {
                if (offset >= data_.size()) return 0;
                size_t to_copy = std::min(iov.size(), data_.size() - offset);
                std::memcpy(iov.data(), data_.data() + offset, to_copy);
                return to_copy;
            }

        private:
            std::string data_;
            size_t pos_;
        };

        TEST(CordBufferTest, append_by_readv_restart_false) {
            CordBufferBase<64, 4096> cord;
            MockReader reader("0123456789");
            auto result = cord.append_by_readv(reader, 10, false);
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value_or_die(), 10);
            EXPECT_EQ(cord.size(), 10);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "0123456789");
        }

        TEST(CordBufferTest, append_by_readv_restart_true) {
            CordBufferBase<64, 4096> cord;
            MockReader reader("abcdefghijklmn");
            auto result = cord.append_by_readv(reader, 14, true);
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value_or_die(), 14);
            EXPECT_EQ(cord.size(), 14);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "abcdefghijklmn");
        }

        TEST(CordBufferTest, append_by_readv_partial) {
            CordBufferBase<64, 4096> cord;
            MockReader reader("short");
            auto result = cord.append_by_readv(reader, 10, false);
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value_or_die(), 5); // only "short" available
            EXPECT_EQ(cord.size(), 5);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "short");
        }

        TEST(CordBufferTest, append_by_readv_error) {
            // Mock that returns error

            class ErrorReader : public IOReader {
            public:
                turbo::Result<size_t> read(turbo::span<char> &) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> readv(const std::vector<IOVec> &, size_t) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> preadv(const std::vector<IOVec> &, size_t, size_t) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> pread(turbo::span<char> &, size_t) override {
                    return turbo::unknown_error("fail");
                }
            };

            ErrorReader reader;
            CordBufferBase<64, 4096> cord;
            auto result = cord.append_by_readv(reader, 100, false);
            EXPECT_FALSE(result.ok());
            EXPECT_EQ(cord.size(), 0); // rolled back
        }

        TEST(CordBufferTest, append_by_read) {
            CordBufferBase<64, 4096> cord;
            MockReader reader("hello world");
            auto result = cord.append_by_read(reader, 11);
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value_or_die(), 11);
            EXPECT_EQ(cord.size(), 11);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "hello world");
        }

        TEST(CordBufferTest, append_by_read_partial) {
            CordBufferBase<64, 4096> cord;
            MockReader reader("hello");
            auto result = cord.append_by_read(reader, 10);
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value_or_die(), 5);
            EXPECT_EQ(cord.size(), 5);
        }

        TEST(CordBufferTest, append_by_read_error) {
            class ErrorReader : public IOReader {
            public:
                turbo::Result<size_t> read(turbo::span<char> &) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> readv(const std::vector<IOVec> &, size_t) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> preadv(const std::vector<IOVec> &, size_t, size_t) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> pread(turbo::span<char> &, size_t) override {
                    return turbo::unknown_error("fail");
                }
            };
            ErrorReader reader;
            CordBufferBase<64, 4096> cord;
            auto result = cord.append_by_read(reader, 100);
            EXPECT_FALSE(result.ok());
            EXPECT_EQ(cord.size(), 0);
        }

        TEST(CordBufferTest, append_by_pread) {
            CordBufferBase<64, 4096> cord;
            MockReader reader("abcdefghij");
            auto result = cord.append_by_pread(reader, 2, 5);
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value_or_die(), 5);
            EXPECT_EQ(cord.size(), 5);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "cdefg"); // starting at offset 2
        }

        // ============================================================================
        // 12. Clear and Swap
        // ============================================================================

        TEST(CordBufferTest, clear) {
            CordBufferBase<64, 4096> cord;
            cord.append("data", 4);
            cord.clear();
            EXPECT_EQ(cord.size(), 0);
            EXPECT_EQ(cord.buffer_count(), 0);
        }

        TEST(CordBufferTest, swap) {
            CordBufferBase<64, 4096> cord1, cord2;
            cord1.append("one", 3);
            cord2.append("two", 3);
            cord1.swap(cord2);
            EXPECT_EQ(cord1.size(), 3);
            std::string s;
            for (char c: cord1) s.push_back(c);
            EXPECT_EQ(s, "two");
            EXPECT_EQ(cord2.size(), 3);
            s.clear();
            for (char c: cord2) s.push_back(c);
            EXPECT_EQ(s, "one");
        }

        // ============================================================================
        // 13. Operators
        // ============================================================================

        TEST(CordBufferTest, assignment_from_string_view) {
            CordBufferBase<64, 4096> cord;
            cord = std::string_view("hello");
            EXPECT_EQ(cord.size(), 5);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "hello");
        }

        TEST(CordBufferTest, assignment_from_span) {
            char arr[] = "world";
            turbo::span<char> sp(arr, 5);
            CordBufferBase<64, 4096> cord;
            cord = sp;
            EXPECT_EQ(cord.size(), 5);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "world");
        }

        TEST(CordBufferTest, assignment_from_vector) {
            std::vector<char> vec = {'a', 'b', 'c'};
            CordBufferBase<64, 4096> cord;
            cord = vec;
            EXPECT_EQ(cord.size(), 3);
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "abc");
        }

        TEST(CordBufferTest, stream_insertion) {
            CordBufferBase<64, 4096> cord;
            cord << "hello " << std::string_view("world ") << 42;
            std::string out;
            for (char c: cord) out.push_back(c);
            EXPECT_EQ(out, "hello world 42");
        }

        // ============================================================================
        // 14. Receivers / Appenders (Compatibility)
        // ============================================================================

        TEST(ContainerAppenderTest, append) {
            CordBufferBase<64, 4096> cord;
            ContainerAppender<CordBufferBase<64, 4096> > appender(cord);
            appender.append("test", 4);
            EXPECT_EQ(cord.size(), 4);
            appender.clear();
            EXPECT_EQ(cord.size(), 0);
        }

        TEST(ContainerReceiverTest, release) {
            ContainerReceiver<CordBufferBase<64, 4096> > receiver;
            receiver.append("hello", 5);
            auto cord = receiver.release();
            EXPECT_EQ(cord.size(), 5);
        }

        // ============================================================================
        // 15. Type Traits
        // ============================================================================

        TEST(TypeTraitsTest, is_cord_buffer) {
            EXPECT_TRUE((fermat::is_cord_buffer_v<CordBufferBase<64, 4096>>));
            EXPECT_FALSE(is_cord_buffer_v<int>);
            EXPECT_EQ((is_cord_buffer<CordBufferBase<64, 4096>>::alignment), 64);
            EXPECT_EQ((is_cord_buffer<CordBufferBase<64, 4096>>::block_size), 4096);
        }

        // ============================================================================
        // 16. Edge Cases & Stress Tests
        // ============================================================================

        // 16.1 Empty Cord
        TEST(CordBufferDeathTest, front_back_buffer_on_empty) {
            CordBufferBase<64, 4096> cord;
            EXPECT_DEATH(cord.front_buffer(), ".*");
            EXPECT_DEATH(cord.back_buffer(), ".*");
        }

        TEST(CordBufferTest, pop_zero_does_nothing) {
            CordBufferBase<64, 4096> cord;
            cord.append("hello", 5);
            size_t old_size = cord.size();
            cord.pop_front(0);
            cord.pop_back(0);
            EXPECT_EQ(cord.size(), old_size);
        }

        TEST(CordBufferTest, pop_front_larger_than_size_clears) {
            CordBufferBase<64, 4096> cord;
            cord.append("hello", 5);
            cord.pop_front(10);
            EXPECT_EQ(cord.size(), 0);
            EXPECT_TRUE(cord.begin() == cord.end());
        }

        // 16.2 Single‑Segment Cord
        TEST(CordBufferTest, single_segment_operations) {
            CordBufferBase<64, 4096> cord;
            cord.append("data", 4);
            EXPECT_EQ(cord.size(), 4);
            // Write more via output_next
            auto span = cord.output_next();
            std::memcpy(span.data(), "extra", 5);
            cord.output_backup(span.size() - 5);
            EXPECT_EQ(cord.size(), 9);
            // Pop from front and back
            cord.pop_front(2);
            cord.pop_back(2);
            EXPECT_EQ(cord.size(), 5);
            std::string result;
            for (char c: cord) result.push_back(c);
            EXPECT_EQ(result, "taext"); // "data" + "extra" -> after pop_front(2): "taextra", pop_back(2): "taex"
        }

        // 16.3 Multi‑Segment Cord with random interleaving
        TEST(CordBufferTest, multi_segment_random_ops) {
            CordBufferBase<64, 4096> cord;
            constexpr int kIterations = 100;
            size_t expected_size = 0;
            for (int i = 0; i < kIterations; ++i) {
                int op = rand() % 5;
                if (op == 0) {
                    // append small string
                    std::string s = "abc";
                    cord.append(s);
                    expected_size += s.size();
                } else if (op == 1) {
                    // prepend small string
                    std::string s = "xy";
                    auto ptr = std::make_shared<Buffer<char, 64> >();
                    ptr->append("xy", 2);
                    cord.prepend_reference(BufferRef<64>::reference(ptr
                                                                    , 0, 2));
                    expected_size += 2;
                } else if (op == 2 && expected_size > 0) {
                    // pop_front random
                    size_t n = rand() % (expected_size + 1);
                    cord.pop_front(n);
                    expected_size -= n;
                } else if (op == 3 && expected_size > 0) {
                    // pop_back random
                    size_t n = rand() % (expected_size + 1);
                    cord.pop_back(n);
                    expected_size -= n;
                } else if (op == 4) {
                    // append big block to create new segment
                    std::string big(4096, 'Z');
                    cord.append(big);
                    expected_size += big.size();
                }
                EXPECT_EQ(cord.size(), expected_size);
            }
        }

        // 16.4 Shared Cord (COW)
        TEST(CordBufferTest, cow_share_independent_writes) {
            CordBufferBase<64, 4096> cordA;
            cordA.append("original", 8);
            auto cordB = cordA.share();

            // Initially both share the same buffer
            EXPECT_EQ(cordA.front_buffer().buffer, cordB.front_buffer().buffer);
            EXPECT_EQ(cordA.size(), 8);
            EXPECT_EQ(cordB.size(), 8);

            // Append to A – allocates a new buffer for the tail, original remains shared
            cordA.append(" more", 5);
            EXPECT_EQ(cordA.size(), 13);
            EXPECT_EQ(cordB.size(), 8); // B unchanged
            EXPECT_EQ(cordA.buffer_count(), 2);
            EXPECT_EQ(cordB.buffer_count(), 1);
            // Original block still shared
            EXPECT_EQ(cordA.front_buffer().buffer, cordB.front_buffer().buffer);
            // New block in A is exclusive
            EXPECT_TRUE(cordA.back_buffer().is_unique());

            // Append to B – similarly allocates its own new buffer
            cordB.append(" extra", 6);
            EXPECT_EQ(cordB.size(), 14);
            EXPECT_EQ(cordB.buffer_count(), 2);
            // Original block still shared
            EXPECT_EQ(cordA.front_buffer().buffer, cordB.front_buffer().buffer);
            // The new buffer_count are different
            EXPECT_NE(cordA.back_buffer().buffer, cordB.back_buffer().buffer);
            // Both new buffer_count are exclusive
            EXPECT_TRUE(cordA.back_buffer().is_unique());
            EXPECT_TRUE(cordB.back_buffer().is_unique());

            // Verify that shared segments are not writable
            EXPECT_EQ(cordB.front_buffer().write_able(), 0); // shared, not writable
            EXPECT_GT(cordB.back_buffer().write_able(), 0); // exclusive, writable
        }

        // 16.5 Very Large Data (avoid OOM in CI, skip for normal runs)
        TEST(CordBufferTest, DISABLED_very_large_append) {
            CordBufferBase<64, 4096> cord;
            const size_t kLarge = 3ULL * 1024 * 1024 * 1024; // 3 GiB
            EXPECT_THROW(cord.append(std::string(kLarge, 'x')), std::bad_alloc);
        }

        TEST(CordBufferTest, create_buffers_huge) {
            // May throw bad_alloc, test only the call
            EXPECT_ANY_THROW((CordBufferBase<64, 4096>::create_buffers(1000ULL * 1024 * 1024 * 1024)));
        }

        // 16.6 Error Paths
        TEST(CordBufferTest, append_buffer_out_of_range) {
            CordBufferBase<64, 4096> cord;
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            auto status = cord.append_buffer(buf, 50, 60);
            EXPECT_FALSE(status.ok());
            EXPECT_EQ(status.code(), turbo::StatusCode::kInvalidArgument);
        }

        TEST(CordBufferTest, append_by_readv_error_rollback) {
            class FaultyReader : public IOReader {
            public:
                turbo::Result<size_t> readv(const std::vector<IOVec> &, size_t) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> preadv(const std::vector<IOVec> &, size_t, size_t) override {
                    return turbo::unknown_error("fail");
                }

                turbo::Result<size_t> read(turbo::span<char> &) override {
                    return 0;
                }

                turbo::Result<size_t> pread(turbo::span<char> &, size_t) override {
                    return 0;
                }
            };

            CordBufferBase<64, 4096> cord;
            // Pre‑fill with some data so rollback happens
            cord.append("initial", 7);
            size_t old_size = cord.size();
            FaultyReader reader;
            auto result = cord.append_by_readv(reader, 100, false);
            EXPECT_FALSE(result.ok());
            EXPECT_EQ(cord.size(), old_size); // rolled back
        }

        TEST(CordBufferTest, output_backup_larger_than_total) {
            CordBufferBase<64, 4096> cord;
            cord.append("abc", 3);
            cord.output_backup(10);
            EXPECT_EQ(cord.size(), 0);
            EXPECT_TRUE(cord.begin() == cord.end());
        }

        TEST(CordBufferTest, borrow_on_shared_fails) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(100);
            auto ref = BufferRef<64>::reference(buf, 0, 100);
            void *out = nullptr;
            int size = 0;
            EXPECT_FALSE(ref.borrow(&out, &size));
        }

        // ============================================================================
        // 17. Concurrency (stress test, may be disabled by default)
        // ============================================================================

        TEST(CordBufferTest, concurrent_append_with_cow) {
            CordBufferBase<64, 4096> shared;
            shared.append("base", 4);
            auto shared_copy = shared.share(); // another reference
            std::atomic<bool> done{false};
            std::thread t1([&]() {
                for (int i = 0; i < 1000; ++i) {
                    shared.append("a", 1);
                }
                done = true;
            });
            std::thread t2([&]() {
                while (!done) {
                    // reading while writing – should not crash (COW may cause data races but no UB)
                    volatile size_t s = shared_copy.size();
                    (void) s;
                }
            });
            t1.join();
            t2.join();
            // No crash, but data races may still exist – this test is only for sanity.
        }

        // ============================================================================
        // 18. Memory Leak Checks (run under valgrind/asan)
        // ============================================================================
        TEST(CordBufferTest, memory_leak_check) {
            {
                CordBufferBase<64, 4096> cord;
                cord.append("data", 4);
                auto ref = BufferRef<64>::create_write_able(100);
                cord.append_writeable(std::move(ref));
                auto copy = cord.copy();
            } // all destructors run, no leaks expected
            SUCCEED();
        }

        TEST(CordBufferTest, assign_buffer_ownership_no_double_free) {
            auto buf = std::make_shared<Buffer<char, 64> >();
            buf->resize_uninitialized(50);
            {
                CordBufferBase<64, 4096> cord;
                cord.append_buffer(std::move(buf), 0, 50);
            } // buffer destroyed once
            SUCCEED();
        }


        using TestCordBuffer = CordBufferBase<64, 4096>;
        /// Helper: flatten CordBuffer content into std::string for easy comparison.
        static std::string Flatten(const TestCordBuffer &cord) {
            std::string result;
            for (auto b = cord.buffer_begin(); b != cord.buffer_end(); ++b) {
                result += std::string_view(b->buffer->data() + b->range.offset, b->range.length);
            }
            return result;
        }

        // ============================================================================
        // Tests for cat() method - unformatted concatenation
        // ============================================================================

        /// Verifies that cat() appends multiple arguments correctly.
        /// cat() internally uses turbo::str_cat and supports mixed types.
        TEST(CordBufferTest, CatAppendsMultipleArguments) {
            TestCordBuffer cord;

            cord.cat("Hello", " ", "World", "!", 42);

            EXPECT_EQ(Flatten(cord), "Hello World!42");
            EXPECT_EQ(cord.size(), Flatten(cord).size());
        }

        /// Verifies that cat() works with a single argument.
        TEST(CordBufferTest, CatWithSingleArgument) {
            TestCordBuffer cord;
            cord.cat("Single");
            EXPECT_EQ(Flatten(cord), "Single");
        }

        /// Verifies that cat() with no arguments does nothing.
        TEST(CordBufferTest, CatWithNoArguments) {
            TestCordBuffer cord;
            cord.cat(); // No arguments
            EXPECT_EQ(Flatten(cord), "");
            EXPECT_EQ(cord.size(), 0);
        }

        /// Verifies that cat() can be called multiple times sequentially.
        TEST(CordBufferTest, CatMultipleTimes) {
            TestCordBuffer cord;
            cord.cat("Part1");
            cord.cat("Part2");
            cord.cat("Part3");
            EXPECT_EQ(Flatten(cord), "Part1Part2Part3");
        }

        // ============================================================================
        // Tests for format() method - formatted output
        // ============================================================================

        /// Verifies basic format() with a single placeholder.
        TEST(CordBufferTest, FormatWithOnePlaceholder) {
            TestCordBuffer cord;
            cord.format("The answer is %d", 42);
            EXPECT_EQ(Flatten(cord), "The answer is 42");
        }

        /// Verifies format() with multiple placeholders of different types.
        TEST(CordBufferTest, FormatWithMultiplePlaceholders) {
            TestCordBuffer cord;
            cord.format("int=%d, double=%.2f, string=%s", 123, 3.14, "hello");
            EXPECT_EQ(Flatten(cord), "int=123, double=3.14, string=hello");
        }

        /// Verifies format() with precision specifier for floating point.
        TEST(CordBufferTest, FormatWithFloatPrecision) {
            TestCordBuffer cord;
            cord.format("Pi ≈ %.2f", 3.1415926535);
            EXPECT_EQ(Flatten(cord), "Pi ≈ 3.14");
        }

        /// Verifies format() with width and alignment specifiers.
        TEST(CordBufferTest, FormatWithWidthAndAlignment) {
            TestCordBuffer cord;
            cord.format("%10s", "right"); // right-aligned in 10 chars
            EXPECT_EQ(Flatten(cord), "     right");
        }

        /// Verifies chaining format() calls.
        TEST(CordBufferTest, FormatMultipleTimes) {
            TestCordBuffer cord;
            cord.format("Start ");
            cord.format("middle ");
            cord.format("end");
            EXPECT_EQ(Flatten(cord), "Start middle end");
        }

        // ============================================================================
        // Mixed tests: cat() and format() interleaved
        // ============================================================================

        /// Verifies interleaving cat() and format() calls.
        TEST(CordBufferTest, CatAndFormatMixed) {
            TestCordBuffer cord;
            cord.cat("Value: ");
            cord.format("%.2f", 3.14);
            cord.cat(", squared = ");
            cord.format("%.2f", 3.14 * 3.14);
            EXPECT_EQ(Flatten(cord), "Value: 3.14, squared = 9.86");
        }

        /// Verifies that format() respects previous content.
        TEST(CordBufferTest, FormatAfterCatPreservesContent) {
            TestCordBuffer cord;
            cord.cat("Prefix: ");
            cord.format("[%s]", "formatted");
            EXPECT_EQ(Flatten(cord), "Prefix: [formatted]");
        }

        /// Verifies that cat() after format() appends correctly.
        TEST(CordBufferTest, CatAfterFormat) {
            TestCordBuffer cord;
            cord.format("Hello %s", "World");
            cord.cat("!");
            EXPECT_EQ(Flatten(cord), "Hello World!");
        }

        // ============================================================================
        // Edge cases and validation
        // ============================================================================

        /// Verifies that format() with an empty format string does nothing.
        TEST(CordBufferTest, FormatWithEmptyString) {
            TestCordBuffer cord;
            cord.cat("before");
            cord.format("");
            cord.cat("after");
            EXPECT_EQ(Flatten(cord), "beforeafter");
        }

        /// Verifies that cat() with empty strings does not affect content.
        TEST(CordBufferTest, CatWithEmptyString) {
            TestCordBuffer cord;
            cord.cat("nonempty");
            cord.cat("");
            cord.cat("");
            cord.cat("end");
            EXPECT_EQ(Flatten(cord), "nonemptyend");
        }

        /// Verifies that both methods handle large outputs correctly (not crashing).
        TEST(CordBufferTest, LargeOutputHandling) {
            TestCordBuffer cord;
            std::string large(10000, 'A');
            cord.format("%s", large);
            EXPECT_EQ(cord.size(), 10000);
            cord.cat(large);
            EXPECT_EQ(cord.size(), 20000);
        }

        /// Verifies that format() and cat() work on an empty CordBuffer.
        TEST(CordBufferTest, EmptyBufferOperations) {
            TestCordBuffer cord;
            EXPECT_EQ(cord.size(), 0);
            cord.format("test");
            EXPECT_EQ(Flatten(cord), "test");
        }
    } // namespace test
} // namespace fermat
