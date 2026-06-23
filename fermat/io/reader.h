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

#pragma once

#include <fermat/io/sysio.h>
#include <fermat/container/receiver.h>

namespace fermat {
    /// Sequential‑only file reader (no random access).
    class SequenceReadFile : public IOReader {
    public:
        SequenceReadFile() = default;

        ~SequenceReadFile() override = default;

        SequenceReadFile(const SequenceReadFile &) = delete;

        SequenceReadFile &operator=(const SequenceReadFile &) = delete;

        SequenceReadFile(SequenceReadFile &&) noexcept = default;

        SequenceReadFile &operator=(SequenceReadFile &&) noexcept = default;

        /// Opens the file for sequential reading.
        /// @return OkStatus() on success, or an error status.
        turbo::Status open(const turbo::FilePath &path);

        // -------------------------------------------------------------------------
        // IOReader interface (only sequential)
        // -------------------------------------------------------------------------

        /// Sequential read.
        turbo::Result<size_t> read(turbo::span<char> &iov) override;

        /// Sequential scatter read.
        turbo::Result<size_t> readv(const std::vector<IOVec> &iov, size_t total_hint) override;

        /// Random read – not supported for sequential‑only files.
        turbo::Result<size_t> pread(turbo::span<char> &, size_t) override {
            return turbo::unimplemented_error("SequenceReadFile does not support pread");
        }

        /// Random scatter read – not supported.
        turbo::Result<size_t> preadv(const std::vector<IOVec> &, size_t, size_t) override {
            return turbo::unimplemented_error("SequenceReadFile does not support preadv");
        }

        /// Returns true if the sequential read position has reached the end of the file.
        [[nodiscard]] bool is_eof() const;

        /// Returns the total file size in bytes if known (regular file), otherwise 0.
        [[nodiscard]] size_t size() const;

    private:
        turbo::FilePath _path;
        HandleGuard _handleGuard; ///< RAII wrapper for the open file handle.
        int64_t _file_size = -1; ///< Total file size, or -1 if unknown (e.g., pipe).
        int64_t _total_read = 0; ///< Number of bytes read sequentially so far.
    };

    /// File reader that supports both sequential and random access.
    class RandomReadFile : public IOReader {
    public:
        RandomReadFile() = default;

        ~RandomReadFile() override = default;

        RandomReadFile(const RandomReadFile &) = delete;

        RandomReadFile &operator=(const RandomReadFile &) = delete;

        RandomReadFile(RandomReadFile &&) noexcept = default;

        RandomReadFile &operator=(RandomReadFile &&) noexcept = default;

        /// Opens the file for reading (both sequential and random access).
        /// @return OkStatus() on success, or an error status.
        turbo::Status open(const turbo::FilePath &path);

        /// Repositions the sequential read offset.
        /// @param offset New offset from the beginning of the file.
        /// @return New offset on success, or a status error.
        turbo::Result<int64_t> seek(int64_t offset);

        // -------------------------------------------------------------------------
        // IOReader interface
        // -------------------------------------------------------------------------

        /// Sequential read from the current file position.
        turbo::Result<size_t> read(turbo::span<char> &iov) override;

        /// Sequential scatter read from the current file position.
        turbo::Result<size_t> readv(const std::vector<IOVec> &iov, size_t total_hint) override;

        /// Random read from a specified offset (does not change file position).
        turbo::Result<size_t> pread(turbo::span<char> &iov, size_t offset) override;

        /// Random scatter read from a specified offset (does not change file position).
        turbo::Result<size_t> preadv(const std::vector<IOVec> &iov, size_t offset, size_t total_hint) override;

        /// Returns true if the sequential read position has reached the end of the file.
        [[nodiscard]] bool is_eof() const;

        /// Returns the total file size in bytes if known (regular file), otherwise 0.
        [[nodiscard]] size_t size() const;

    private:
        turbo::FilePath _path;
        HandleGuard _handleGuard; ///< RAII wrapper for the open file handle.
        int64_t _file_size = -1; ///< Total file size, or -1 if unknown.
        int64_t _sequential_pos = 0; ///< Current sequential read position.
    };

    /// Line‑oriented file reader with large internal buffer and line numbering.
    /// Implements IOReader for sequential raw reads as well.
    class ReadLineFile : public IOReader {
    public:
        /// Constructs a reader with the specified buffer size (default 256 KiB).
        explicit ReadLineFile(size_t buffer_size = 256 * 1024);

        ~ReadLineFile() override = default;

        ReadLineFile(const ReadLineFile &) = delete;

        ReadLineFile &operator=(const ReadLineFile &) = delete;

        ReadLineFile(ReadLineFile &&) noexcept = default;

        ReadLineFile &operator=(ReadLineFile &&) noexcept = default;

        /// Opens the file.
        /// @return OkStatus() on success, or an error status.
        turbo::Status open(const turbo::FilePath &path);

        /// Reads the next line (excluding newline). Returns the line and its number (1‑based).
        /// On EOF: returns a special error (or a Result with empty string and line number 0).
        turbo::Result<std::pair<std::string, size_t> > readline();

        /// Returns the current line number (next line to be read).
        [[nodiscard]] size_t line_number() const noexcept { return _line_num; }

        /// Returns the total number of bytes read so far (including newlines).
        [[nodiscard]] size_t bytes_read() const noexcept { return _bytes_read; }

        // -------------------------------------------------------------------------
        // IOReader interface (sequential raw reads, delegates to internal buffer)
        // -------------------------------------------------------------------------

        turbo::Result<size_t> read(turbo::span<char> &iov) override;

        turbo::Result<size_t> readv(const std::vector<IOVec> &iov, size_t total_hint) override;

        turbo::Result<size_t> pread(turbo::span<char> &, size_t) override {
            return turbo::unimplemented_error("ReadLineFile does not support pread");
        }

        turbo::Result<size_t> preadv(const std::vector<IOVec> &, size_t, size_t) override {
            return turbo::unimplemented_error("ReadLineFile does not support preadv");
        }

    private:
        // Refills the internal buffer from the underlying file.
        turbo::Result<bool> refill();

        SequenceReadFile _file; ///< Underlying sequential file reader.
        std::vector<char> _buffer; ///< Large internal buffer.
        size_t _buf_pos = 0; ///< Current read position in buffer.
        size_t _buf_size = 0; ///< Number of valid bytes in buffer.
        size_t _line_num = 1; ///< Next line number to read.
        size_t _bytes_read = 0; ///< Total bytes consumed from the file (including newlines).
    };

    turbo::Status read_small_file(Receiver &recv, const turbo::FilePath &path);
} // namespace fermat
