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

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <fermat/container/window_cord.h>

namespace fermat {
    namespace test {

        using TestChunk = CordBufferBase<64, 4096>;
        using TestWindow = WindowCord<64, 4096>;

        static std::string FlattenChunk(const TestChunk &cord) {
            return cord.flatten<std::string>();
        }

        static TestChunk MakeChunk(std::string_view data) {
            TestChunk chunk;
            chunk.append(data).ignore_error();
            return chunk;
        }

        static std::vector<TestChunk> MakeFileLikeChunks(const std::string &data, size_t block_size) {
            std::vector<TestChunk> blocks;
            for (size_t i = 0; i < data.size(); i += block_size) {
                blocks.push_back(MakeChunk(data.substr(i, block_size)));
            }
            return blocks;
        }

        TEST(WindowCordTest, push_move_and_metrics) {
            TestWindow window;
            EXPECT_TRUE(window.empty());
            EXPECT_EQ(window.chunk_count(), 0);
            EXPECT_EQ(window.byte_size(), 0);

            window.push(MakeChunk("abc"));
            window.push(MakeChunk("defgh"));

            EXPECT_FALSE(window.empty());
            EXPECT_EQ(window.chunk_count(), 2);
            EXPECT_EQ(window.byte_size(), 8);
            EXPECT_EQ(FlattenChunk(window[0]), "abc");
            EXPECT_EQ(FlattenChunk(window[1]), "defgh");
        }

        TEST(WindowCordTest, push_const_shares_buffers) {
            TestChunk original = MakeChunk("shared");
            TestWindow window;
            window.push(original);

            EXPECT_EQ(window.chunk_count(), 1);
            EXPECT_EQ(window.front().buffer_count(), 1);
            EXPECT_EQ(original.front_buffer().buffer, window.front().front_buffer().buffer);
        }

        TEST(WindowCordTest, ten_file_like_blocks) {
            const std::string payload = "0123456789abcdef";
            auto blocks = MakeFileLikeChunks(payload, 1);

            TestWindow window;
            for (auto &block: blocks) {
                window.push(std::move(block));
            }

            EXPECT_EQ(window.chunk_count(), 16);
            EXPECT_EQ(window.byte_size(), payload.size());

            std::string merged;
            for (size_t i = 0; i < window.chunk_count(); ++i) {
                merged += FlattenChunk(window[i]);
            }
            EXPECT_EQ(merged, payload);
        }

        TEST(WindowCordTest, mutate_chunk_in_place) {
            TestWindow window;
            window.push(MakeChunk("hello"));
            window.push(MakeChunk("world"));

            window[0].append("!").ignore_error();
            ASSERT_EQ(window[0].size(), 6);
            ASSERT_EQ(window[1].size(), 5);
            EXPECT_EQ(FlattenChunk(window[0]), "hello!");
            EXPECT_EQ(window.byte_size(), 11);
            EXPECT_EQ(FlattenChunk(window[1]), "world");
        }

        TEST(WindowCordTest, share_at_is_non_destructive) {
            TestWindow window;
            window.push(MakeChunk("one"));
            window.push(MakeChunk("two"));

            auto view = window.share_at(1);
            EXPECT_EQ(window.chunk_count(), 2);
            EXPECT_EQ(FlattenChunk(view), "two");
            EXPECT_EQ(FlattenChunk(window[1]), "two");
            EXPECT_EQ(view.front_buffer().buffer, window[1].front_buffer().buffer);
        }

        TEST(WindowCordTest, share_front) {
            TestWindow window;
            window.push(MakeChunk("head"));
            window.push(MakeChunk("tail"));

            auto view = window.share_front();
            EXPECT_EQ(FlattenChunk(view), "head");
            EXPECT_EQ(window.chunk_count(), 2);
        }

        TEST(WindowCordTest, take_front_fifo) {
            TestWindow window;
            window.push(MakeChunk("first"));
            window.push(MakeChunk("second"));
            window.push(MakeChunk("third"));

            auto a = window.take_front();
            auto b = window.take_front();
            EXPECT_EQ(FlattenChunk(a), "first");
            EXPECT_EQ(FlattenChunk(b), "second");
            EXPECT_EQ(window.chunk_count(), 1);
            EXPECT_EQ(FlattenChunk(window.front()), "third");
        }

        TEST(WindowCordTest, pop_front_discards_without_return) {
            TestWindow window;
            window.push(MakeChunk("drop"));
            window.push(MakeChunk("keep"));

            window.pop_front();
            EXPECT_EQ(window.chunk_count(), 1);
            EXPECT_EQ(FlattenChunk(window.front()), "keep");
        }

        TEST(WindowCordTest, iterate_with_range_for) {
            TestWindow window;
            window.push(MakeChunk("a"));
            window.push(MakeChunk("bb"));
            window.push(MakeChunk("ccc"));

            std::string walked;
            for (const auto &chunk: window) {
                walked += FlattenChunk(chunk);
            }
            EXPECT_EQ(walked, "abbccc");
        }

        TEST(WindowCordTest, clear) {
            TestWindow window;
            window.push(MakeChunk("x"));
            window.push(MakeChunk("y"));
            window.clear();
            EXPECT_TRUE(window.empty());
            EXPECT_EQ(window.chunk_count(), 0);
            EXPECT_EQ(window.byte_size(), 0);
        }

        TEST(WindowCordTest, share_at_snapshot_unchanged_by_later_append) {
            TestWindow window;
            window.push(MakeChunk("data"));

            auto view = window.share_at(0);
            window[0].append("_more").ignore_error();

            EXPECT_EQ(FlattenChunk(view), "data");
            EXPECT_EQ(FlattenChunk(window[0]), "data_more");
        }

        TEST(WindowCordDeathTest, take_front_on_empty) {
            TestWindow window;
            EXPECT_DEATH(window.take_front(), ".*");
        }

        TEST(WindowCordDeathTest, pop_front_on_empty) {
            TestWindow window;
            EXPECT_DEATH(window.pop_front(), ".*");
        }

        TEST(WindowCordDeathTest, operator_index_out_of_range) {
            TestWindow window;
            window.push(MakeChunk("x"));
            EXPECT_DEATH((void) window[1], ".*");
        }

    } // namespace test
} // namespace fermat
