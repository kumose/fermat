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

#include <fermat/container/buffer.h>
#include <fermat/container/stl.h>
#include <memory>
#include <new>

namespace fermat {

    /// @brief A managed view for writing into borrowed CordBufferBase segments.
    ///
    /// Lease provides a safe, sequential writing interface over discrete memory spans.
    /// It prevents common pitfalls like manual pointer arithmetic and buffer overflows
    /// by encapsulating the span collection and internal offsets.
    class DeprecateBufferLease {
    public:
        /// @brief Default constructor (only CordBufferBase can create a Lease).
        DeprecateBufferLease() = default;

        /// @brief Write data sequentially into the leased spans (append mode).
        ///        Data is copied from @p data into the internal buffers, automatically
        ///        crossing span boundaries. The write cursor advances by @p len bytes.
        /// @param data Pointer to the source data.
        /// @param len  Number of bytes to write.
        /// @return turbo::OkStatus on success, or turbo::OutOfRangeError if remaining capacity is insufficient.
        turbo::Status write(const char *data, size_t len);

        /// @brief Shrink the written size by removing @p n bytes from the end.
        ///        This rewinds both the total size and the internal write cursors.
        /// @param n Number of bytes to remove (clamped to current size).
        void pop_back(size_t n);

        /// @brief Reset the lease to its initial empty state.
        ///        Clears all written data and resets write position to the beginning.
        void clear();

        /// @brief Get the total number of bytes successfully written (via write or advance).
        /// @return Total bytes written so far.
       [[nodiscard]] size_t size() const { return _total_size; }

        /// @brief Get the total capacity (maximum writable bytes) of this lease.
        /// @return Capacity in bytes.
        size_t capacity() const { return _capacity; }

        /// @brief Check if there is any remaining writable space.
        /// @return true if remaining() > 0, false otherwise.
        operator bool() const noexcept {
            return _capacity - _total_size > 0;
        }

        /// @brief Get the number of bytes still available for writing.
        /// @return Remaining capacity.
        size_t remaining() const { return _capacity - _total_size; }

        /// @brief Check if no data has been written yet.
        /// @return true if size() == 0.
        bool empty() const noexcept {
            return _total_size == 0;
        }

        /// @brief Check whether this lease is currently borrowed (i.e., has valid spans).
        /// @return true if capacity > 0, false otherwise.
        bool borrowed() const noexcept {
            return _capacity != 0;
        }


        /// @brief Zero-copy write helpers: collect writable segments, fill externally, then commit.
        ///
        /// These two methods are used together for advanced scenarios where you need
        /// to write directly into the lease's internal buffers without an extra copy
        /// (e.g., using readv/writev, DMA, or filling iovec arrays).
        ///
        /// Typical usage:
        ///   1. Call visit_remaining() with a visitor that collects each writable segment
        ///      (pointer + capacity) into an iovec array or similar structure.
        ///   2. Use the collected information to perform a system call (e.g., readv)
        ///      that fills the buffers. Let N be the total number of bytes actually written.
        ///   3. Call advance(N) exactly once to commit the written bytes.
        ///   4. Finally, commit the lease to the CordBufferBase via CordBufferBase::commit().
        ///
        /// Important:
        ///   - visit_remaining() does NOT advance the write cursor.
        ///   - The visitor may be called multiple times (once per contiguous block).
        ///   - Do NOT call write() on the same lease when using this low-level pair.
        ///   - advance() must be called exactly once after all segments have been filled,
        ///     with the total number of bytes written across all segments.
        ///
        /// Example (using readv):
        ///   struct iovec iov[16];
        ///   int iovcnt = 0;
        ///   lease.visit_remaining([&](char* ptr, size_t cap) {
        ///       iov[iovcnt].iov_base = ptr;
        ///       iov[iovcnt].iov_len  = cap;
        ///       ++iovcnt;
        ///       return iovcnt < 16;   // continue while space remains in array
        ///   });
        ///   ssize_t n = readv(fd, iov, iovcnt);
        ///   if (n > 0) lease.advance(n);
        ///   iob.commit(lease);
        ///
        /// @param visitor Callback with signature bool(char* buffer, size_t capacity).
        ///                Return true to continue to next segment, false to stop.
        /// @param n       Total number of bytes actually written (must <= remaining()).
        using VisitorCallback = std::function<bool(char *, size_t)>;

        void visit_remaining(const VisitorCallback &visitor) const;

        void advance(size_t n);

        /// @brief Initialize the lease with multiple non‑contiguous writable spans.
        /// @param sp Vector of spans borrowed from CordBufferBase.
        void set(std::vector<turbo::span<char> > sp);

        /// @brief Initialize the lease with a single contiguous writable span.
        /// @param sp A single span borrowed from CordBufferBase.
        void set(turbo::span<char> sp);

        void clear_lease();

        /// Copy/move constructors and assignment operators are defaulted.
        /// They are kept private because Lease instances are not meant to be
        /// copied or moved by users; only CordBufferBase manages them.
        DeprecateBufferLease(DeprecateBufferLease &) = default;

        DeprecateBufferLease(DeprecateBufferLease &&) noexcept = default;

        DeprecateBufferLease &operator=(const DeprecateBufferLease &) = default;

        DeprecateBufferLease &operator=(DeprecateBufferLease &&) noexcept = default;

        [[nodiscard]] const std::vector<turbo::span<char>> &spans() const noexcept {
            return _spans;
        }
    private:
        std::vector<turbo::span<char> > _spans;
        size_t _total_size{0};
        size_t _index{0};
        size_t _offset{0};
        size_t _capacity{0};
    };

    class BufferLease {
    public:
        /// @brief Default constructor (only CordBufferBase can create a Lease).
        BufferLease() = default;

        /// @brief Reset the lease to its initial empty state.
        ///        Clears all written data and resets write position to the beginning.
        void clear();

        /// @brief Get the total number of bytes successfully written (via write or advance).
        /// @return Total bytes written so far.
       [[nodiscard]] size_t size() const { return _total_size; }

        /// @brief Get the total capacity (maximum writable bytes) of this lease.
        /// @return Capacity in bytes.
       [[nodiscard]] size_t capacity() const { return _capacity; }


        /// @brief Get the number of bytes still available for writing.
        /// @return Remaining capacity.
        [[nodiscard]] size_t remaining() const { return _capacity - _total_size; }

        /// @brief Check if no data has been written yet.
        /// @return true if size() == 0.
        [[nodiscard]] bool empty() const noexcept {
            return _total_size == 0;
        }

        /// @brief Check whether this lease is currently borrowed (i.e., has valid spans).
        /// @return true if capacity > 0, false otherwise.
        [[nodiscard]] bool borrowed() const noexcept {
            return _capacity != 0;
        }

        void set_size(size_t n);

        /// @brief Initialize the lease with multiple non‑contiguous writable spans.
        /// @param sp Vector of spans borrowed from CordBufferBase.
        void set(std::vector<turbo::span<char> > sp);

        /// @brief Initialize the lease with a single contiguous writable span.
        /// @param sp A single span borrowed from CordBufferBase.
        void set(turbo::span<char> sp);

        void clear_lease();

        /// Copy/move constructors and assignment operators are defaulted.
        /// They are kept private because Lease instances are not meant to be
        /// copied or moved by users; only CordBufferBase manages them.
        BufferLease(BufferLease &) = default;

        BufferLease(BufferLease &&) noexcept = default;

        BufferLease &operator=(const BufferLease &) = default;

        BufferLease &operator=(BufferLease &&) noexcept = default;

        [[nodiscard]] const std::vector<turbo::span<char>> &spans() const noexcept {
            return _spans;
        }
    private:
        std::vector<turbo::span<char> > _spans;
        size_t _total_size{0};
        size_t _capacity{0};
    };

}  // namespace fermat
