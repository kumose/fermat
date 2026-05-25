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

#include <fermat/io/sysio.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#endif

namespace fermat {
#ifdef _WIN32
    static turbo::Result<PlatformHandle> sys_open_windows(const turbo::FilePath &path, int flags, int mode) noexcept {
        std::wstring wpath = path.wstring();
        DWORD desired_access = 0;
        DWORD creation_disposition = 0;

        switch (flags & O_ACCMODE) {
            case O_RDONLY: desired_access = GENERIC_READ;
                break;
            case O_WRONLY: desired_access = GENERIC_WRITE;
                break;
            case O_RDWR: desired_access = GENERIC_READ | GENERIC_WRITE;
                break;
            default: break;
        }
        if (flags & O_CREAT) {
            creation_disposition = (flags & O_EXCL) ? CREATE_NEW : OPEN_ALWAYS;
        } else if (flags & O_TRUNC) {
            creation_disposition = TRUNCATE_EXISTING;
        } else {
            creation_disposition = OPEN_EXISTING;
        }

        DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        DWORD attributes = FILE_ATTRIBUTE_NORMAL;
        HANDLE h = ::CreateFileW(wpath.c_str(), desired_access, share_mode, nullptr,
                                 creation_disposition, attributes, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            return turbo::make_status(::GetLastError(), "open file:", path.string(), "fail");
        }
        return h;
    }
#else
    static turbo::Result<PlatformHandle> sys_open_posix(const turbo::FilePath &path, int flags, int mode) noexcept {
        std::string p = path.string();
        int fd = ::open(p.c_str(), flags, static_cast<mode_t>(mode));
        if (fd == -1) {
            return turbo::make_status(errno, "open file:", p, "fail");
        }
        return fd;
    }
#endif

    turbo::Result<PlatformHandle> sys_open(const turbo::FilePath &path, int flags, int mode) noexcept {
#ifdef _WIN32
        return sys_open_windows(path, flags, mode);
#else
        return sys_open_posix(path, flags, mode);
#endif
    }

    // close
#ifdef _WIN32
    static turbo::Status sys_close_windows(PlatformHandle h) noexcept {
        if (!::CloseHandle(h)) {
            return turbo::make_status(::GetLastError(), "CloseHandle failed");
        }
        return turbo::OkStatus();
    }
#else
    static turbo::Status sys_close_posix(PlatformHandle h) noexcept {
        if (::close(h) != 0) {
            return turbo::make_status(errno, "close failed");
        }
        return turbo::OkStatus();
    }
#endif

    turbo::Status sys_close(PlatformHandle h) noexcept {
#ifdef _WIN32
        return sys_close_windows(h);
#else
        return sys_close_posix(h);
#endif
    }

    // read
#ifdef _WIN32
    static turbo::Result<size_t> sys_read_windows(PlatformHandle h, void *buf, size_t count) noexcept {
        DWORD bytes_read = 0;
        DWORD total = 0;
        char *dst = static_cast<char *>(buf);
        while (total < count) {
            DWORD to_read = static_cast<DWORD>(count - total);
            if (!::ReadFile(h, dst + total, to_read, &bytes_read, nullptr)) {
                DWORD err = ::GetLastError();
                if (total > 0) return total;
                return turbo::make_status(err, "ReadFile failed");
            }
            if (bytes_read == 0) break;
            total += bytes_read;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_read_posix(PlatformHandle h, void *buf, size_t count) noexcept {
        ssize_t n;
        char *dst = static_cast<char *>(buf);
        size_t total = 0;
        while (total < count) {
            n = ::read(h, dst + total, count - total);
            if (n < 0) {
                if (errno == EINTR) continue;
                if (total > 0) return total;
                return turbo::make_status(errno, "read failed");
            }
            if (n == 0) break;
            total += n;
        }
        return total;
    }
#endif

    turbo::Result<size_t> sys_read(PlatformHandle h, void *buf, size_t count) noexcept {
#ifdef _WIN32
        return sys_read_windows(h, buf, count);
#else
        return sys_read_posix(h, buf, count);
#endif
    }

    // write
#ifdef _WIN32
    static turbo::Result<size_t> sys_write_windows(PlatformHandle h, const void *buf, size_t count) noexcept {
        DWORD bytes_written = 0;
        DWORD total = 0;
        const char *src = static_cast<const char *>(buf);
        while (total < count) {
            DWORD to_write = static_cast<DWORD>(count - total);
            if (!::WriteFile(h, src + total, to_write, &bytes_written, nullptr)) {
                DWORD err = ::GetLastError();
                if (total > 0) return total;
                return turbo::make_status(err, "WriteFile failed");
            }
            total += bytes_written;
            if (bytes_written == 0) break;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_write_posix(PlatformHandle h, const void *buf, size_t count) noexcept {
        ssize_t n;
        const char *src = static_cast<const char *>(buf);
        size_t total = 0;
        while (total < count) {
            n = ::write(h, src + total, count - total);
            if (n < 0) {
                if (errno == EINTR) continue;
                if (total > 0) return total;
                return turbo::make_status(errno, "write failed");
            }
            if (n == 0) break;
            total += n;
        }
        return total;
    }
#endif

    turbo::Result<size_t> sys_write(PlatformHandle h, const void *buf, size_t count) noexcept {
#ifdef _WIN32
        return sys_write_windows(h, buf, count);
#else
        return sys_write_posix(h, buf, count);
#endif
    }

    // pread
#ifdef _WIN32
    static turbo::Result<size_t> sys_pread_windows(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept {
        OVERLAPPED ov = {};
        ov.Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
        ov.OffsetHigh = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFF);
        DWORD bytes_read = 0;
        char *dst = static_cast<char *>(buf);
        size_t total = 0;
        while (total < count) {
            DWORD to_read = static_cast<DWORD>(count - total);
            if (!::ReadFile(h, dst + total, to_read, &bytes_read, &ov)) {
                DWORD err = ::GetLastError();
                if (err == ERROR_IO_PENDING) {
                    if (!::GetOverlappedResult(h, &ov, &bytes_read, TRUE)) {
                        err = ::GetLastError();
                        if (total > 0) return total;
                        return turbo::make_status(err, "ReadFile overlapped failed");
                    }
                } else {
                    if (total > 0) return total;
                    return turbo::make_status(err, "ReadFile failed");
                }
            }
            total += bytes_read;
            if (bytes_read == 0) break;
            ov.Offset += bytes_read;
            ov.OffsetHigh += (ov.Offset < bytes_read) ? 1 : 0;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_pread_posix(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept {
        ssize_t n;
        char *dst = static_cast<char *>(buf);
        size_t total = 0;
        while (total < count) {
            n = ::pread(h, dst + total, count - total, static_cast<off_t>(offset + total));
            if (n < 0) {
                if (errno == EINTR) continue;
                if (total > 0) return total;
                return turbo::make_status(errno, "pread failed");
            }
            if (n == 0) break;
            total += n;
        }
        return total;
    }
#endif

    turbo::Result<size_t> sys_pread(PlatformHandle h, size_t offset, void *buf, size_t count) noexcept {
#ifdef _WIN32
        return sys_pread_windows(h, offset, buf, count);
#else
        return sys_pread_posix(h, offset, buf, count);
#endif
    }

    // pwrite
#ifdef _WIN32
    static turbo::Result<size_t> sys_pwrite_windows(PlatformHandle h, size_t offset, const void *buf,
                                                    size_t count) noexcept {
        OVERLAPPED ov = {};
        ov.Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
        ov.OffsetHigh = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFF);
        DWORD bytes_written = 0;
        const char *src = static_cast<const char *>(buf);
        size_t total = 0;
        while (total < count) {
            DWORD to_write = static_cast<DWORD>(count - total);
            if (!::WriteFile(h, src + total, to_write, &bytes_written, &ov)) {
                DWORD err = ::GetLastError();
                if (err == ERROR_IO_PENDING) {
                    if (!::GetOverlappedResult(h, &ov, &bytes_written, TRUE)) {
                        err = ::GetLastError();
                        if (total > 0) return total;
                        return turbo::make_status(err, "WriteFile overlapped failed");
                    }
                } else {
                    if (total > 0) return total;
                    return turbo::make_status(err, "WriteFile failed");
                }
            }
            total += bytes_written;
            if (bytes_written == 0) break;
            ov.Offset += bytes_written;
            ov.OffsetHigh += (ov.Offset < bytes_written) ? 1 : 0;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_pwrite_posix(PlatformHandle h, size_t offset, const void *buf,
                                                  size_t count) noexcept {
        ssize_t n;
        const char *src = static_cast<const char *>(buf);
        size_t total = 0;
        while (total < count) {
            n = ::pwrite(h, src + total, count - total, static_cast<off_t>(offset + total));
            if (n < 0) {
                if (errno == EINTR) continue;
                if (total > 0) return total;
                return turbo::make_status(errno, "pwrite failed");
            }
            if (n == 0) break;
            total += n;
        }
        return total;
    }
#endif

    turbo::Result<size_t> sys_pwrite(PlatformHandle h, size_t offset, const void *buf, size_t count) noexcept {
#ifdef _WIN32
        return sys_pwrite_windows(h, offset, buf, count);
#else
        return sys_pwrite_posix(h, offset, buf, count);
#endif
    }

    // readv
#ifdef _WIN32
    static turbo::Result<size_t> sys_readv_windows(PlatformHandle h, const std::vector<IOVec> &iov,
                                                   size_t total_hint) noexcept {
        (void) total_hint;
        size_t total = 0;
        for (const auto &vec: iov) {
            char *buf = static_cast<char *>(vec.iov_base);
            size_t len = vec.iov_len;
            while (len > 0) {
                auto r = sys_read(h, buf, len);
                if (!r.ok()) {
                    if (total > 0) return total;
                    return r.status();
                }
                size_t n = r.value();
                if (n == 0) break;
                total += n;
                buf += n;
                len -= n;
            }
            if (len > 0) break;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_readv_posix(PlatformHandle h, const std::vector<IOVec> &iov,
                                                 size_t total_hint) noexcept {
        (void) total_hint;
        // IOVec is layout-compatible with struct iovec
        ssize_t n = ::readv(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()));
        if (n < 0) {
            if (errno == EINTR) {
                n = ::readv(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()));
                if (n < 0) return turbo::make_status(errno, "readv failed");
            } else {
                return turbo::make_status(errno, "readv failed");
            }
        }
        return static_cast<size_t>(n);
    }
#endif

    turbo::Result<size_t> sys_readv(PlatformHandle h, const std::vector<IOVec> &iov, size_t total_hint) noexcept {
#ifdef _WIN32
        return sys_readv_windows(h, iov, total_hint);
#else
        return sys_readv_posix(h, iov, total_hint);
#endif
    }

    // writev
#ifdef _WIN32
    static turbo::Result<size_t> sys_writev_windows(PlatformHandle h, const std::vector<IOVec> &iov,
                                                    size_t total_hint) noexcept {
        (void) total_hint;
        size_t total = 0;
        for (const auto &vec: iov) {
            const char *buf = static_cast<const char *>(vec.iov_base);
            size_t len = vec.iov_len;
            while (len > 0) {
                auto r = sys_write(h, buf, len);
                if (!r.ok()) {
                    if (total > 0) return total;
                    return r.status();
                }
                size_t n = r.value();
                if (n == 0) break;
                total += n;
                buf += n;
                len -= n;
            }
            if (len > 0) break;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_writev_posix(PlatformHandle h, const std::vector<IOVec> &iov,
                                                  size_t total_hint) noexcept {
        (void) total_hint;
        ssize_t n = ::writev(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()));
        if (n < 0) {
            if (errno == EINTR) {
                n = ::writev(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()));
                if (n < 0) return turbo::make_status(errno, "writev failed");
            } else {
                return turbo::make_status(errno, "writev failed");
            }
        }
        return static_cast<size_t>(n);
    }
#endif

    turbo::Result<size_t> sys_writev(PlatformHandle h, const std::vector<IOVec> &iov, size_t total_hint) noexcept {
#ifdef _WIN32
        return sys_writev_windows(h, iov, total_hint);
#else
        return sys_writev_posix(h, iov, total_hint);
#endif
    }

    // preadv
#ifdef _WIN32
    static turbo::Result<size_t> sys_preadv_windows(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                                    size_t total_hint) noexcept {
        (void) total_hint;
        size_t total = 0;
        size_t cur_offset = offset;
        for (const auto &vec: iov) {
            char *buf = static_cast<char *>(vec.iov_base);
            size_t len = vec.iov_len;
            while (len > 0) {
                auto r = sys_pread(h, cur_offset, buf, len);
                if (!r.ok()) {
                    if (total > 0) return total;
                    return r.status();
                }
                size_t n = r.value();
                if (n == 0) break;
                total += n;
                buf += n;
                len -= n;
                cur_offset += n;
            }
            if (len > 0) break;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_preadv_posix(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                                  size_t total_hint) noexcept {
        (void) total_hint;
        ssize_t n = ::preadv(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()),
                             static_cast<off_t>(offset));
        if (n < 0) {
            if (errno == EINTR) {
                n = ::preadv(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()),
                             static_cast<off_t>(offset));
                if (n < 0) return turbo::make_status(errno, "preadv failed");
            } else {
                return turbo::make_status(errno, "preadv failed");
            }
        }
        return static_cast<size_t>(n);
    }
#endif

    turbo::Result<size_t> sys_preadv(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                     size_t total_hint) noexcept {
#ifdef _WIN32
        return sys_preadv_windows(h, offset, iov, total_hint);
#else
        return sys_preadv_posix(h, offset, iov, total_hint);
#endif
    }

    // pwritev
#ifdef _WIN32
    static turbo::Result<size_t> sys_pwritev_windows(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                                     size_t total_hint) noexcept {
        (void) total_hint;
        size_t total = 0;
        size_t cur_offset = offset;
        for (const auto &vec: iov) {
            const char *buf = static_cast<const char *>(vec.iov_base);
            size_t len = vec.iov_len;
            while (len > 0) {
                auto r = sys_pwrite(h, cur_offset, buf, len);
                if (!r.ok()) {
                    if (total > 0) return total;
                    return r.status();
                }
                size_t n = r.value();
                if (n == 0) break;
                total += n;
                buf += n;
                len -= n;
                cur_offset += n;
            }
            if (len > 0) break;
        }
        return total;
    }
#else
    static turbo::Result<size_t> sys_pwritev_posix(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                                   size_t total_hint) noexcept {
        (void) total_hint;
        ssize_t n = ::pwritev(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()),
                              static_cast<off_t>(offset));
        if (n < 0) {
            if (errno == EINTR) {
                n = ::pwritev(h, reinterpret_cast<const struct iovec *>(iov.data()), static_cast<int>(iov.size()),
                              static_cast<off_t>(offset));
                if (n < 0) return turbo::make_status(errno, "pwritev failed");
            } else {
                return turbo::make_status(errno, "pwritev failed");
            }
        }
        return static_cast<size_t>(n);
    }
#endif

    turbo::Result<size_t> sys_pwritev(PlatformHandle h, size_t offset, const std::vector<IOVec> &iov,
                                      size_t total_hint) noexcept {
#ifdef _WIN32
        return sys_pwritev_windows(h, offset, iov, total_hint);
#else
        return sys_pwritev_posix(h, offset, iov, total_hint);
#endif
    }

    // flush
#ifdef _WIN32
    static turbo::Status sys_flush_windows(PlatformHandle h) noexcept {
        if (!::FlushFileBuffers(h)) {
            return turbo::make_status(::GetLastError(), "FlushFileBuffers failed");
        }
        return turbo::OkStatus();
    }
#else
    static turbo::Status sys_flush_posix(PlatformHandle h) noexcept {
        if (::fsync(h) != 0) {
            return turbo::make_status(errno, "fsync failed");
        }
        return turbo::OkStatus();
    }
#endif

    turbo::Status sys_flush(PlatformHandle h) noexcept {
#ifdef _WIN32
        return sys_flush_windows(h);
#else
        return sys_flush_posix(h);
#endif
    }

    // flush_data
#ifdef _WIN32
    static turbo::Status sys_flush_data_windows(PlatformHandle h) noexcept {
        // Windows has no direct data-only flush; fall back to full flush.
        return sys_flush_windows(h);
    }
#else
    static turbo::Status sys_flush_data_posix(PlatformHandle h) noexcept {
#ifdef _POSIX_SYNCHRONIZED_IO
        if (::fdatasync(h) != 0) {
            return turbo::make_status(errno, "fdatasync failed");
        }
        return turbo::OkStatus();
#else
        return sys_flush_posix(h);
#endif
    }
#endif

    turbo::Status sys_flush_data(PlatformHandle h) noexcept {
#ifdef _WIN32
        return sys_flush_data_windows(h);
#else
        return sys_flush_data_posix(h);
#endif
    }

    // seek
#ifdef _WIN32
    static turbo::Result<int64_t> sys_seek_windows(PlatformHandle h, int64_t offset, int whence) noexcept {
        DWORD method;
        switch (whence) {
            case SEEK_SET: method = FILE_BEGIN;
                break;
            case SEEK_CUR: method = FILE_CURRENT;
                break;
            case SEEK_END: method = FILE_END;
                break;
            default: return turbo::make_status(EINVAL, "invalid whence");
        }
        LARGE_INTEGER li;
        li.QuadPart = offset;
        LARGE_INTEGER new_pos;
        if (!::SetFilePointerEx(h, li, &new_pos, method)) {
            return turbo::make_status(::GetLastError(), "SetFilePointerEx failed");
        }
        return new_pos.QuadPart;
    }
#else
    static turbo::Result<int64_t> sys_seek_posix(PlatformHandle h, int64_t offset, int whence) noexcept {
        off_t result = ::lseek(h, static_cast<off_t>(offset), whence);
        if (result == static_cast<off_t>(-1)) {
            return turbo::make_status(errno, "lseek failed");
        }
        return static_cast<int64_t>(result);
    }
#endif

    turbo::Result<int64_t> sys_seek(PlatformHandle h, int64_t offset, int whence) noexcept {
#ifdef _WIN32
        return sys_seek_windows(h, offset, whence);
#else
        return sys_seek_posix(h, offset, whence);
#endif
    }

    // truncate
#ifdef _WIN32
    static turbo::Result<int64_t> sys_truncate_windows(PlatformHandle h, int64_t n) noexcept {
        LARGE_INTEGER li;
        li.QuadPart = n;
        if (!::SetFilePointerEx(h, li, nullptr, FILE_BEGIN)) {
            return turbo::make_status(::GetLastError(), "SetFilePointerEx failed");
        }
        if (!::SetEndOfFile(h)) {
            return turbo::make_status(::GetLastError(), "SetEndOfFile failed");
        }
        return n;
    }
#else
    static turbo::Result<int64_t> sys_truncate_posix(PlatformHandle h, int64_t n) noexcept {
        if (::ftruncate(h, static_cast<off_t>(n)) != 0) {
            return turbo::make_status(errno, "ftruncate failed");
        }
        return n;
    }
#endif

    turbo::Result<int64_t> sys_truncate(PlatformHandle h, int64_t n) noexcept {
#ifdef _WIN32
        return sys_truncate_windows(h, n);
#else
        return sys_truncate_posix(h, n);
#endif
    }
} // namespace fermat
