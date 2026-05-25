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

#include <turbo/utility/status.h>
#include <turbo/files/filesystem.h>
#include <fermat/io/fd_guard.h>
#include <string>

namespace fermat {
    // Open a file (POSIX open or Windows CreateFile)
    turbo::Result<PlatformHandle> sys_open(const turbo::FilePath &path, int flags, int mode = 0) noexcept;

    // Close a handle (returns Status instead of int)
    turbo::Status sys_close(PlatformHandle h) noexcept;

    // Read from a handle
    turbo::Result<size_t> sys_read(PlatformHandle h, void *buf, size_t count) noexcept;

    // Write to a handle
    turbo::Result<size_t> sys_write(PlatformHandle h, const void *buf, size_t count) noexcept;

    turbo::Result<size_t> sys_pread(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept;

    turbo::Result<size_t> sys_pwrite(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept;

    // Vector I/O (readv / writev)
    turbo::Result<size_t> sys_readv(PlatformHandle h, const std::vector<turbo::span<char> > &iov,
                                    size_t total_hint) noexcept;

    turbo::Result<size_t> sys_writev(PlatformHandle h, const std::vector<turbo::span<const char> > &iov,
                                     size_t total_hint) noexcept;

    turbo::Result<size_t> sys_preadv(PlatformHandle h, size_t offset, const std::vector<turbo::span<char> > &iov,
                                     size_t total_hint) noexcept;

    turbo::Result<size_t> sys_pwritev(PlatformHandle h, size_t offset, const std::vector<turbo::span<const char> > &iov,
                                      size_t total_hint) noexcept;

    // Seek
    turbo::Result<int64_t> sys_seek(PlatformHandle h, int64_t offset, int whence) noexcept;

    inline turbo::Result<PlatformHandle> open_file_guard(const turbo::FilePath &path, int flags, int mode = 0) {
        TURBO_MOVE_OR_RAISE(auto h, sys_open(path, flags, mode));
        return HandleGuard(h);
    }
} // namespace fermat
