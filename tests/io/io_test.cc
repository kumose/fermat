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

#include <fermat/container/receiver.h>
#include <fermat/io/fd_guard.h>
#include <fermat/io/reader.h>
#include <fermat/io/writer.h>
#include <gtest/gtest.h>

#include <turbo/files/filesystem.h>

#include <unistd.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

std::atomic<int> gTempFileCounter{0};

class TempFile {
public:
    explicit TempFile(const std::string &name) {
        const auto dir = std::filesystem::temp_directory_path();
        const auto filename = "fermat_io_test_" + std::to_string(::getpid()) + "_" +
                              std::to_string(gTempFileCounter.fetch_add(1)) + "_" + name;
        _path = turbo::FilePath((dir / filename).string());
    }

    const turbo::FilePath &path() const { return _path; }

    ~TempFile() { std::filesystem::remove(_path.string()); }

private:
    turbo::FilePath _path;
};

std::string ReadAll(fermat::SequenceReadFile &reader) {
    std::string out;
    std::vector<char> buf(4096);
    while (!reader.is_eof()) {
        turbo::span<char> span(buf.data(), buf.size());
        auto result = reader.read(span);
        if (!result.ok()) {
            ADD_FAILURE() << result.status().to_string();
            break;
        }
        const size_t n = result.value_or_die();
        if (n == 0) {
            break;
        }
        out.append(buf.data(), n);
    }
    return out;
}

TEST(HandleGuardTest, MoveAndReset) {
    fermat::HandleGuard guard;
    EXPECT_FALSE(guard.is_valid());

    fermat::HandleGuard moved(std::move(guard));
    EXPECT_FALSE(moved.is_valid());

    fermat::HandleGuard other;
    other = std::move(moved);
    EXPECT_FALSE(other.is_valid());

    other.reset();
    EXPECT_FALSE(other.is_valid());
}

TEST(IOSmallFileTest, WriteAndReadRoundTrip) {
    TempFile temp("small.bin");
    const std::string payload = "fermat io small file payload";

    {
        turbo::span<const char> span(payload.data(), payload.size());
        turbo::span<char> write_span(const_cast<char *>(span.data()), span.size());
        ASSERT_TRUE(fermat::write_small_file(write_span, temp.path()).ok());
    }

    fermat::ContainerReceiver<std::string> receiver;
    ASSERT_TRUE(fermat::read_small_file(receiver, temp.path()).ok());
    EXPECT_EQ(receiver.storage, payload);
}

TEST(SequenceIOTest, WriteReadAndWritev) {
    TempFile temp("seq.bin");

    fermat::SequenceWriteFile writer;
    ASSERT_TRUE(writer.open(temp.path()).ok());

    const std::string part1 = "hello";
    const std::string part2 = " world";
    turbo::span<const char> span1(part1.data(), part1.size());
    turbo::span<char> write1(const_cast<char *>(span1.data()), span1.size());
    auto w1 = writer.write(write1);
    ASSERT_TRUE(w1.ok());
    EXPECT_EQ(w1.value_or_die(), part1.size());

    std::vector<fermat::IOVec> iov;
    iov.emplace_back(const_cast<char *>(part2.data()), part2.size());
    auto w2 = writer.writev(iov, part2.size());
    ASSERT_TRUE(w2.ok());
    EXPECT_EQ(w2.value_or_die(), part2.size());
    ASSERT_TRUE(writer.flush().ok());

    fermat::SequenceReadFile reader;
    ASSERT_TRUE(reader.open(temp.path()).ok());
    EXPECT_EQ(reader.size(), part1.size() + part2.size());
    EXPECT_EQ(ReadAll(reader), part1 + part2);
    EXPECT_TRUE(reader.is_eof());
}

TEST(SequenceReadFileTest, PreadNotSupported) {
    TempFile temp("pread.bin");
    {
        const std::string payload = "abc";
        turbo::span<const char> span(payload.data(), payload.size());
        turbo::span<char> write_span(const_cast<char *>(span.data()), span.size());
        ASSERT_TRUE(fermat::write_small_file(write_span, temp.path()).ok());
    }

    fermat::SequenceReadFile reader;
    ASSERT_TRUE(reader.open(temp.path()).ok());

    std::vector<char> buf(3);
    turbo::span<char> span(buf.data(), buf.size());
    auto result = reader.pread(span, 0);
    EXPECT_FALSE(result.ok());
}

TEST(RandomIOTest, PreadPwriteSeek) {
    TempFile temp("random.bin");

    fermat::RandomWriteFile writer;
    ASSERT_TRUE(writer.open(temp.path()).ok());

    const std::string payload = "0123456789";
    turbo::span<const char> span(payload.data(), payload.size());
    turbo::span<char> write_span(const_cast<char *>(span.data()), span.size());
    auto written = writer.write(write_span);
    ASSERT_TRUE(written.ok());
    EXPECT_EQ(written.value_or_die(), payload.size());

    const std::string patch = "ZZZ";
    turbo::span<const char> patch_span(patch.data(), patch.size());
    turbo::span<char> patch_write(const_cast<char *>(patch_span.data()), patch_span.size());
    auto patched = writer.pwrite(patch_write, 0);
    ASSERT_TRUE(patched.ok());
    EXPECT_EQ(patched.value_or_die(), patch.size());
    ASSERT_TRUE(writer.flush().ok());

    fermat::RandomReadFile reader;
    ASSERT_TRUE(reader.open(temp.path()).ok());
    EXPECT_EQ(reader.size(), payload.size());

    std::vector<char> tail(5);
    turbo::span<char> tail_span(tail.data(), tail.size());
    auto tail_read = reader.pread(tail_span, 5);
    ASSERT_TRUE(tail_read.ok());
    EXPECT_EQ(tail_read.value_or_die(), tail.size());
    EXPECT_EQ(std::string(tail.data(), tail.size()), "56789");

    auto seek_result = reader.seek(3);
    ASSERT_TRUE(seek_result.ok());
    EXPECT_EQ(seek_result.value_or_die(), 3);

    std::vector<char> rest(7);
    turbo::span<char> rest_span(rest.data(), rest.size());
    auto rest_read = reader.read(rest_span);
    ASSERT_TRUE(rest_read.ok());
    EXPECT_EQ(rest_read.value_or_die(), rest.size());
    EXPECT_EQ(std::string(rest.data(), rest.size()), "3456789");
}

TEST(RandomIOTest, Preadv) {
    TempFile temp("preadv.bin");
    const std::string payload = "0123456789";
    {
        turbo::span<const char> span(payload.data(), payload.size());
        turbo::span<char> write_span(const_cast<char *>(span.data()), span.size());
        ASSERT_TRUE(fermat::write_small_file(write_span, temp.path()).ok());
    }

    fermat::RandomReadFile reader;
    ASSERT_TRUE(reader.open(temp.path()).ok());

    char buf1[3]{};
    char buf2[4]{};
    std::vector<fermat::IOVec> iov;
    iov.emplace_back(buf1, sizeof(buf1));
    iov.emplace_back(buf2, sizeof(buf2));

    auto result = reader.preadv(iov, 2, sizeof(buf1) + sizeof(buf2));
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value_or_die(), sizeof(buf1) + sizeof(buf2));
    EXPECT_EQ(std::string(buf1, sizeof(buf1)), "234");
    EXPECT_EQ(std::string(buf2, sizeof(buf2)), "5678");
}

TEST(ReadLineFileTest, ReadsLinesWithNumbers) {
    TempFile temp("lines.txt");
    const std::string content = "alpha\nbeta\n\ngamma";
    {
        turbo::span<const char> span(content.data(), content.size());
        turbo::span<char> write_span(const_cast<char *>(span.data()), span.size());
        ASSERT_TRUE(fermat::write_small_file(write_span, temp.path()).ok());
    }

    fermat::ReadLineFile reader(64);
    ASSERT_TRUE(reader.open(temp.path()).ok());

    auto line1 = reader.readline();
    ASSERT_TRUE(line1.ok());
    EXPECT_EQ(line1.value_or_die().first, "alpha");
    EXPECT_EQ(line1.value_or_die().second, 1u);

    auto line2 = reader.readline();
    ASSERT_TRUE(line2.ok());
    EXPECT_EQ(line2.value_or_die().first, "beta");
    EXPECT_EQ(line2.value_or_die().second, 2u);

    auto line3 = reader.readline();
    ASSERT_TRUE(line3.ok());
    EXPECT_EQ(line3.value_or_die().first, "");
    EXPECT_EQ(line3.value_or_die().second, 3u);

    auto line4 = reader.readline();
    ASSERT_TRUE(line4.ok());
    EXPECT_EQ(line4.value_or_die().first, "gamma");
    EXPECT_EQ(line4.value_or_die().second, 4u);
    EXPECT_GT(reader.bytes_read(), 0u);
}

} // namespace
