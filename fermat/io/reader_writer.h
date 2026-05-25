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

#include <turbo/container/span.h>
#include <turbo/utility/status.h>

namespace fermat {
    /// Scatter/gather buffer entry, layout-compatible with POSIX struct iovec.
    struct IOVec {
        IOVec() = default;

        virtual ~IOVec() = default;

        IOVec(void *d, size_t len) : iov_base(d), iov_len(len) {
        }

        IOVec(const IOVec &) = default;

        IOVec(IOVec &&) = default;

        IOVec &operator=(const IOVec &) = default;

        IOVec &operator=(IOVec &&) = default;

        /// Pointer to the data buffer.
        void *iov_base{nullptr};
        ///<Length of the data buffer in bytes.
        size_t iov_len{0};
    };

    // Abstraction for reading data.
    // The simplest implementation is to embed a file descriptor and read from it.
    class IOReader {
    public:
        virtual ~IOReader() = default;

        virtual turbo::Result<size_t> readv(const std::vector<IOVec> &iov, size_t total_bytes_reserved) =
        0;

        virtual turbo::Result<size_t> preadv(const std::vector<IOVec> &iov, size_t offset,
                                             size_t total_bytes_reserved) =
        0;

        virtual turbo::Result<size_t> read(turbo::span<char> &iov) = 0;

        virtual turbo::Result<size_t> pread(turbo::span<char> &iov, size_t offset) = 0;
    };

    class IOWriter {
    public:
        virtual ~IOWriter() = default;

        virtual turbo::Result<size_t> writev(const std::vector<IOVec> &iov, size_t total_bytes_reserved) =
        0;

        virtual turbo::Result<size_t> pwritev(const std::vector<IOVec> &iov, size_t offset,
                                              size_t total_bytes_reserved) =
        0;

        virtual turbo::Result<size_t> write(const turbo::span<char> &iov) = 0;

        virtual turbo::Result<size_t> pwrite(const turbo::span<char> &iov, size_t offset) = 0;
    };
} // namespace fermat
