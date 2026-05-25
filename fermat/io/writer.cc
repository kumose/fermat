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

#include <fermat/io/writer.h>

namespace fermat {
    turbo::Status SequenceWriteFile::open(const turbo::FilePath &path, int mode) {
        // O_WRONLY | O_CREAT | O_TRUNC
        int flags = O_WRONLY | O_CREAT | O_TRUNC;
        auto result = sys_open(path, flags, mode);
        if (!result.ok()) {
            return result.status();
        }
        _handleGuard = HandleGuard(result.value_or_die());
        _path = path;
        return turbo::OkStatus();
    }

    turbo::Result<size_t> SequenceWriteFile::write(const turbo::span<char> &iov) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_write(_handleGuard.get(), iov.data(), iov.size());
    }

    turbo::Result<size_t> SequenceWriteFile::writev(const std::vector<IOVec> &iov, size_t total_hint) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_writev(_handleGuard.get(), iov, total_hint);
    }

    turbo::Status SequenceWriteFile::flush() {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_flush(_handleGuard.get());
    }

    // -----------------------------------------------------------------------------
    // RandomWriteFile
    // -----------------------------------------------------------------------------

    turbo::Status RandomWriteFile::open(const turbo::FilePath &path, int mode, bool truncate) {
        int flags = O_RDWR | O_CREAT;
        if (truncate) {
            flags |= O_TRUNC;
        }
        auto result = sys_open(path, flags, mode);
        if (!result.ok()) {
            return result.status();
        }
        _handleGuard = HandleGuard(result.value_or_die());
        _path = path;
        // Determine initial sequential position (should be 0 after open)
        auto seek_res = sys_seek(_handleGuard.get(), 0, SEEK_CUR);
        if (seek_res.ok()) {
            _sequential_pos = seek_res.value_or_die();
        } else {
            // If seeking fails, assume position is 0 (but log or ignore)
            _sequential_pos = 0;
        }
        return turbo::OkStatus();
    }

    turbo::Result<int64_t> RandomWriteFile::seek(int64_t offset) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_seek(_handleGuard.get(), offset, SEEK_SET);
        if (r.ok()) {
            _sequential_pos = r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> RandomWriteFile::write(const turbo::span<char> &iov) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_write(_handleGuard.get(), iov.data(), iov.size());
        if (r.ok()) {
            _sequential_pos += r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> RandomWriteFile::writev(const std::vector<IOVec> &iov, size_t total_hint) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_writev(_handleGuard.get(), iov, total_hint);
        if (r.ok()) {
            _sequential_pos += r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> RandomWriteFile::pwrite(const turbo::span<char> &iov, size_t offset) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        // pwrite does not affect sequential position
        return sys_pwrite(_handleGuard.get(), offset, iov.data(), iov.size());
    }

    turbo::Result<size_t> RandomWriteFile::pwritev(const std::vector<IOVec> &iov, size_t offset, size_t total_hint) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_pwritev(_handleGuard.get(), offset, iov, total_hint);
    }

    turbo::Status RandomWriteFile::flush() {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_flush(_handleGuard.get());
    }

    turbo::Status write_small_file(const turbo::span<char>& span, const turbo::FilePath& path) {
        // Open file: write-only, create if not exists, truncate if exists
        constexpr int kOpenFlags = O_WRONLY | O_CREAT | O_TRUNC;
        constexpr int kOpenMode = 0644; // rw-r--r--

        auto open_result = sys_open(path, kOpenFlags, kOpenMode);
        if (!open_result.ok()) {
            return open_result.status();
        }
        HandleGuard guard(open_result.value_or_die());

        // Write all data (sys_write handles partial writes internally)
        auto write_result = sys_write(guard.get(), span.data(), span.size());
        if (!write_result.ok()) {
            return write_result.status();
        }
        size_t written = write_result.value_or_die();
        if (written != span.size()) {
            // Should not happen with regular files, but check anyway
            return turbo::make_status(EIO, "partial write: only ", written, " of ", span.size(), " bytes written");
        }
        return turbo::OkStatus();
    }
} // namespace fermat
