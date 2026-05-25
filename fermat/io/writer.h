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

namespace fermat {
    /// Sequential‑only file writer (no random access).
    class SequenceWriteFile : public IOWriter {
    public:
        SequenceWriteFile() = default;

        ~SequenceWriteFile() override = default;

        SequenceWriteFile(const SequenceWriteFile &) = delete;

        SequenceWriteFile &operator=(const SequenceWriteFile &) = delete;

        SequenceWriteFile(SequenceWriteFile &&) noexcept = default;

        SequenceWriteFile &operator=(SequenceWriteFile &&) noexcept = default;

        /// Opens the file for writing (creates/truncates).
        /// @param path The file path.
        /// @param mode Creation mode (permissions) for new files.
        /// @return OkStatus() on success, or an error status.
        turbo::Status open(const turbo::FilePath &path, int mode = 0644);

        // -------------------------------------------------------------------------
        // IOWriter interface (only sequential)
        // -------------------------------------------------------------------------

        /// Sequential write.
        turbo::Result<size_t> write(const turbo::span<char> &iov) override;

        /// Sequential scatter write.
        turbo::Result<size_t> writev(const std::vector<IOVec> &iov, size_t total_hint) override;

        /// Random write – not supported.
        turbo::Result<size_t> pwrite(const turbo::span<char> &, size_t) override {
            return turbo::unimplemented_error("SequenceWriteFile does not support pwrite");
        }

        /// Random scatter write – not supported.
        turbo::Result<size_t> pwritev(const std::vector<IOVec> &, size_t, size_t) override {
            return turbo::unimplemented_error("SequenceWriteFile does not support pwritev");
        }

        /// Flushes any buffered data to disk (full sync).
        turbo::Status flush();

    private:
        turbo::FilePath _path;
        HandleGuard _handleGuard;
    };

    /// File writer that supports both sequential and random access writes.
    class RandomWriteFile : public IOWriter {
    public:
        RandomWriteFile() = default;

        ~RandomWriteFile() override = default;

        RandomWriteFile(const RandomWriteFile &) = delete;

        RandomWriteFile &operator=(const RandomWriteFile &) = delete;

        RandomWriteFile(RandomWriteFile &&) noexcept = default;

        RandomWriteFile &operator=(RandomWriteFile &&) noexcept = default;

        /// Opens the file for writing (creates/truncates by default).
        /// @param path The file path.
        /// @param mode Creation mode (permissions) for new files.
        /// @param truncate If true, truncate existing file; if false, open for read/write without truncation.
        /// @return OkStatus() on success, or an error status.
        turbo::Status open(const turbo::FilePath &path, int mode = 0644, bool truncate = true);

        /// Repositions the sequential write offset.
        /// @param offset New offset from the beginning of the file.
        /// @return New offset on success, or a status error.
        turbo::Result<int64_t> seek(int64_t offset);

        // -------------------------------------------------------------------------
        // IOWriter interface
        // -------------------------------------------------------------------------

        /// Sequential write from the current file position.
        turbo::Result<size_t> write(const turbo::span<char> &iov) override;

        /// Sequential scatter write from the current file position.
        turbo::Result<size_t> writev(const std::vector<IOVec> &iov, size_t total_hint) override;

        /// Random write at a specified offset (does not change file position).
        turbo::Result<size_t> pwrite(const turbo::span<char> &iov, size_t offset) override;

        /// Random scatter write at a specified offset (does not change file position).
        turbo::Result<size_t> pwritev(const std::vector<IOVec> &iov, size_t offset, size_t total_hint) override;

        /// Flushes any buffered data to disk (full sync).
        turbo::Status flush();

    private:
        turbo::FilePath _path;
        HandleGuard _handleGuard;
        int64_t _sequential_pos = 0; ///< Current sequential write position (for `write`/`writev`).
    };

    /// Writes the entire content of a span to a file (overwrites existing file).
    /// @param span The data to write.
    /// @param path The file path.
    /// @return OkStatus() on success, or a status error.
    turbo::Status write_small_file(const turbo::span<char> &span, const turbo::FilePath &path);
} // namespace fermat
