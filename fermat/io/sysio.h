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

#include <fermat/io/reader_writer.h>
#include <turbo/utility/status.h>
#include <turbo/files/filesystem.h>
#include <fermat/io/fd_guard.h>
#include <string>

namespace fermat {
    /// Opens a file or device.
    /// @param path The file path to open.
    /// @param flags Platform‑specific open flags (e.g., O_RDONLY, O_WRONLY, etc.).
    /// @param mode Creation mode (permissions) for new files, ignored when not creating.
    /// @return A valid platform handle on success, or a status error on failure.
    turbo::Result<PlatformHandle> sys_open(const turbo::FilePath &path, int flags, int mode = 0) noexcept;

    /// Closes a previously opened handle.
    /// @param h The handle to close.
    /// @return OkStatus() on success, or an error status if closing fails.
    turbo::Status sys_close(PlatformHandle h) noexcept;

    /// Reads up to `count` bytes from the handle into the provided buffer.
    /// @param h The handle to read from.
    /// @param buf Destination buffer.
    /// @param count Maximum number of bytes to read.
    /// @return Number of bytes actually read on success, or a status error.
    turbo::Result<size_t> sys_read(PlatformHandle h, void *buf, size_t count) noexcept;

    /// Writes up to `count` bytes from the buffer to the handle.
    /// @param h The handle to write to.
    /// @param buf Source buffer.
    /// @param count Maximum number of bytes to write.
    /// @return Number of bytes actually written on success, or a status error.
    turbo::Result<size_t> sys_write(PlatformHandle h, const void *buf, size_t count) noexcept;

    /// Reads from a given file offset without changing the file position.
    /// @param h The handle (must support positioned reads).
    /// @param offset Offset from the beginning of the file.
    /// @param buf Destination buffer.
    /// @param count Maximum number of bytes to read.
    /// @return Number of bytes read on success, or a status error.
    turbo::Result<size_t> sys_pread(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept;

    /// Writes to a given file offset without changing the file position.
    /// @param h The handle (must support positioned writes).
    /// @param offset Offset from the beginning of the file.
    /// @param buf Source buffer.
    /// @param count Maximum number of bytes to write.
    /// @return Number of bytes written on success, or a status error.
    turbo::Result<size_t> sys_pwrite(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept;

    /// Scatter‑gather read (readv). Reads data into multiple buffers.
    /// @param h The handle.
    /// @param iov Vector of spans representing the scatter buffers.
    /// @param total_hint Total number of bytes expected to be read (used for optimisation).
    /// @return Number of bytes read on success, or a status error.
    turbo::Result<size_t> sys_readv(PlatformHandle h, const std::vector<IOVec> &iov,
                                    size_t total_hint) noexcept;

    /// Scatter‑gather write (writev). Writes data from multiple buffers.
    /// @param h The handle.
    /// @param iov Vector of spans representing the gather buffers.
    /// @param total_hint Total number of bytes expected to be written (used for optimisation).
    /// @return Number of bytes written on success, or a status error.
    turbo::Result<size_t> sys_writev(PlatformHandle h, const std::vector<IOVec> &iov,
                                     size_t total_hint) noexcept;

    /// Scatter‑gather positioned read (preadv). Reads from a given offset without changing file position.
    /// @param h The handle.
    /// @param offset Offset from the beginning of the file.
    /// @param iov Vector of spans representing the scatter buffers.
    /// @param total_hint Total bytes expected to be read.
    /// @return Number of bytes read on success, or a status error.
    turbo::Result<size_t> sys_preadv(PlatformHandle h, size_t offset, const std::vector<IOVec > &iov,
                                     size_t total_hint) noexcept;

    /// Scatter‑gather positioned write (pwritev). Writes to a given offset without changing file position.
    /// @param h The handle.
    /// @param offset Offset from the beginning of the file.
    /// @param iov Vector of spans representing the gather buffers.
    /// @param total_hint Total bytes expected to be written.
    /// @return Number of bytes written on success, or a status error.
    turbo::Result<size_t> sys_pwritev(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                      size_t total_hint) noexcept;

    /// Flushes any buffered writes for the handle (full sync, including metadata where required).
    /// @param h The handle.
    /// @return Number of bytes flushed (or zero on success), or a status error.
    turbo::Status sys_flush(PlatformHandle h) noexcept;

    /// Flushes only the data (not metadata) for the handle (e.g., fdatasync on POSIX).
    /// @param h The handle.
    /// @return Number of bytes flushed (or zero on success), or a status error.
    turbo::Status sys_flush_data(PlatformHandle h) noexcept;

    /// Repositions the file offset associated with the handle.
    /// @param h The handle.
    /// @param offset Desired offset.
    /// @param whence One of SEEK_SET, SEEK_CUR, or SEEK_END (platform‑specific constants).
    /// @return New offset from the beginning of the file on success, or a status error.
    turbo::Result<int64_t> sys_seek(PlatformHandle h, int64_t offset, int whence) noexcept;

    /// Truncates or extends the file associated with the handle to the specified size.
    /// @param h The handle to operate on.
    /// @param n The new file size in bytes. If larger than the current size, the file is extended
    ///          (the extended portion reads as zero). If smaller, data beyond the new size is lost.
    /// @return The new file size (n) on success, or a status error on failure.
    turbo::Result<int64_t> sys_truncate(PlatformHandle h, int64_t n) noexcept;

    /// Opens a file and returns a RAII guard that automatically closes the handle.
    /// @param path The file path to open.
    /// @param flags Platform‑specific open flags.
    /// @param mode Creation mode (permissions) for new files.
    /// @return A HandleGuard holding the opened handle on success, or a status error.
    inline turbo::Result<PlatformHandle> open_file_guard(const turbo::FilePath &path, int flags, int mode = 0) {
        TURBO_MOVE_OR_RAISE(auto h, sys_open(path, flags, mode));
        return HandleGuard(h);
    }
} // namespace fermat
