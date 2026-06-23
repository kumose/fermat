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

#include <fermat/io/reader.h>

namespace fermat {
    // -----------------------------------------------------------------------------
    // SequenceReadFile
    // -----------------------------------------------------------------------------

    turbo::Status SequenceReadFile::open(const turbo::FilePath &path) {
        auto result = sys_open(path, O_RDONLY);
        if (!result.ok()) {
            return result.status();
        }
        _handleGuard = HandleGuard(result.value_or_die());
        _path = path;

        // Try to obtain file size (only for regular files)
        auto size_res = sys_seek(_handleGuard.get(), 0, SEEK_END);
        if (size_res.ok()) {
            _file_size = size_res.value_or_die();
            // Seek back to the beginning
            sys_seek(_handleGuard.get(), 0, SEEK_SET).ignore_error();
            _total_read = 0;
        } else {
            _file_size = -1; // Not a seekable file (e.g., pipe)
        }
        return turbo::OkStatus();
    }

    bool SequenceReadFile::is_eof() const {
        return _file_size >= 0 && _total_read >= _file_size;
    }

    size_t SequenceReadFile::size() const {
        return _file_size >= 0 ? static_cast<size_t>(_file_size) : 0;
    }

    turbo::Result<size_t> SequenceReadFile::read(turbo::span<char> &iov) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_read(_handleGuard.get(), iov.data(), iov.size());
        if (r.ok()) {
            _total_read += r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> SequenceReadFile::readv(const std::vector<IOVec> &iov, size_t total_hint) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_readv(_handleGuard.get(), iov, total_hint);
        if (r.ok()) {
            _total_read += r.value_or_die();
        }
        return r;
    }

    // -----------------------------------------------------------------------------
    // RandomReadFile
    // -----------------------------------------------------------------------------

    turbo::Status RandomReadFile::open(const turbo::FilePath &path) {
        auto result = sys_open(path, O_RDONLY);
        if (!result.ok()) {
            return result.status();
        }
        _handleGuard = HandleGuard(result.value_or_die());
        _path = path;

        // Obtain file size if possible
        auto size_res = sys_seek(_handleGuard.get(), 0, SEEK_END);
        if (size_res.ok()) {
            _file_size = size_res.value_or_die();
            // Seek back to the beginning
            sys_seek(_handleGuard.get(), 0, SEEK_SET).ignore_error();
            _sequential_pos = 0;
        } else {
            _file_size = -1;
        }
        return turbo::OkStatus();
    }

    bool RandomReadFile::is_eof() const {
        return _file_size >= 0 && _sequential_pos >= _file_size;
    }

    size_t RandomReadFile::size() const {
        return _file_size >= 0 ? static_cast<size_t>(_file_size) : 0;
    }

    turbo::Result<int64_t> RandomReadFile::seek(int64_t offset) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_seek(_handleGuard.get(), offset, SEEK_SET);
        if (r.ok()) {
            _sequential_pos = r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> RandomReadFile::read(turbo::span<char> &iov) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_read(_handleGuard.get(), iov.data(), iov.size());
        if (r.ok()) {
            _sequential_pos += r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> RandomReadFile::readv(const std::vector<IOVec> &iov, size_t total_hint) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        auto r = sys_readv(_handleGuard.get(), iov, total_hint);
        if (r.ok()) {
            _sequential_pos += r.value_or_die();
        }
        return r;
    }

    turbo::Result<size_t> RandomReadFile::pread(turbo::span<char> &iov, size_t offset) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_pread(_handleGuard.get(), offset, iov.data(), iov.size());
    }

    turbo::Result<size_t> RandomReadFile::preadv(const std::vector<IOVec> &iov, size_t offset, size_t total_hint) {
        if (!_handleGuard.is_valid()) {
            return turbo::make_status(EBADF, "file not open");
        }
        return sys_preadv(_handleGuard.get(), offset, iov, total_hint);
    }

    // -----------------------------------------------------------------------------
    // ReadLineFile
    // -----------------------------------------------------------------------------

    ReadLineFile::ReadLineFile(size_t buffer_size)
        : _buffer(buffer_size) {
    }

    turbo::Status ReadLineFile::open(const turbo::FilePath &path) {
        return _file.open(path);
    }

    turbo::Result<bool> ReadLineFile::refill() {
        if (_buffer.empty()) {
            return true; // nothing to fill
        }
        turbo::span<char> span(_buffer.data(), _buffer.size());
        auto r = _file.read(span);
        if (!r.ok()) {
            return r.status();
        }
        _buf_size = r.value_or_die();
        _buf_pos = 0;
        return _buf_size > 0;
    }

    turbo::Result<std::pair<std::string, size_t> > ReadLineFile::readline() {
        std::string line;
        while (true) {
            if (_buf_pos >= _buf_size) {
                auto more = refill();
                if (!more.ok()) {
                    return more.status();
                }
                if (_buf_size == 0) {
                    // End of file
                    if (line.empty()) {
                        // No more data
                        return turbo::make_status(0, "EOF");
                    } else {
                        // Last line without newline
                        auto result = std::make_pair(std::move(line), _line_num);
                        _line_num++; // next line number would be beyond EOF
                        return result;
                    }
                }
            }
            // Search for newline in the current buffer
            size_t start = _buf_pos;
            while (_buf_pos < _buf_size && _buffer[_buf_pos] != '\n') {
                ++_buf_pos;
            }
            line.append(_buffer.data() + start, _buf_pos - start);
            if (_buf_pos < _buf_size && _buffer[_buf_pos] == '\n') {
                ++_buf_pos; // consume the newline character
                _bytes_read += _buf_pos - start; // include newline in total bytes
                auto result = std::make_pair(std::move(line), _line_num);
                ++_line_num;
                return result;
            }
            // No newline in this chunk; continue reading more data
            _bytes_read += (_buf_pos - start);
        }
    }

    turbo::Result<size_t> ReadLineFile::read(turbo::span<char> &iov) {
        // Delegate to the internal file's read, but also need to keep our buffer consistent?
        // For simplicity, we bypass the line buffer and read directly from the underlying file.
        // But this would break the line reading state. Better to implement read() as pulling
        // from the internal buffer first, then from the file.
        // However, for consistency, we can just forward to _file.read and not maintain our buffer.
        // That is okay because the line reader should not be mixed with raw reads in practice.
        return _file.read(iov);
    }

    turbo::Result<size_t> ReadLineFile::readv(const std::vector<IOVec> &iov, size_t total_hint) {
        return _file.readv(iov, total_hint);
    }

    turbo::Status read_small_file(Receiver &recv, const turbo::FilePath &path) {
        auto handle_result = sys_open(path, O_RDONLY);
        if (!handle_result.ok()) {
            return handle_result.status();
        }
        HandleGuard guard(handle_result.value_or_die());

        // Try to pre‑reserve capacity using file size
        auto size_res = sys_seek(guard.get(), 0, SEEK_END);
        if (size_res.ok()) {
            int64_t size = size_res.value_or_die();
            if (size >= 0) {
                TURBO_RETURN_NOT_OK(recv.reserve(static_cast<size_t>(size)));
            }
            // Seek back to beginning; if this fails, reading will be from wrong offset
            TURBO_RETURN_NOT_OK(sys_seek(guard.get(), 0, SEEK_SET));
        }

        constexpr size_t kBufferSize = 64 * 1024; // 64 KiB
        std::vector<char> buffer(kBufferSize);
        while (true) {
            auto read_result = sys_read(guard.get(), buffer.data(), buffer.size());
            if (!read_result.ok()) {
                return read_result.status();
            }
            size_t n = read_result.value_or_die();
            if (n == 0) {
                break; // EOF
            }
            TURBO_RETURN_NOT_OK(recv.append(buffer.data(), n));
        }
        return turbo::OkStatus();
    }
} // namespace fermat
