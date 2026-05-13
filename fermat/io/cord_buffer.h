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
    template<size_t Alignment, size_t BlockSize>
    class CordBuffer;
    template<size_t Alignment, size_t BlockSize>
    class IOBuf;

    /// @brief A managed view for writing into borrowed CordBuffer segments.
    ///
    /// Lease provides a safe, sequential writing interface over discrete memory spans.
    /// It prevents common pitfalls like manual pointer arithmetic and buffer overflows
    /// by encapsulating the span collection and internal offsets.
    class BufferLease {
    public:
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
        size_t size() const { return _total_size; }

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
        ///   4. Finally, commit the lease to the CordBuffer via CordBuffer::commit().
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

    private:
        /// @brief Default constructor (only CordBuffer can create a Lease).
        BufferLease() = default;

        /// @brief Initialize the lease with multiple non‑contiguous writable spans.
        /// @param sp Vector of spans borrowed from CordBuffer.
        void set(std::vector<turbo::span<char> > sp);

        /// @brief Initialize the lease with a single contiguous writable span.
        /// @param sp A single span borrowed from CordBuffer.
        void set(turbo::span<char> sp);

        void clear_lease();

        /// Copy/move constructors and assignment operators are defaulted.
        /// They are kept private because Lease instances are not meant to be
        /// copied or moved by users; only CordBuffer manages them.
        BufferLease(BufferLease &) = default;

        BufferLease(BufferLease &&) noexcept = default;

        BufferLease &operator=(const BufferLease &) = default;

        BufferLease &operator=(BufferLease &&) noexcept = default;

        template<size_t Alignment, size_t BlockSize>
        friend class CordBuffer;
        template<size_t Alignment, size_t BlockSize>
        friend class IOBuf;

    private:
        std::vector<turbo::span<char> > _spans;
        size_t _total_size{0};
        size_t _index{0};
        size_t _offset{0};
        size_t _capacity{0};
    };


    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class CordBuffer {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr bool is_iobuf = true;
        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;


    public:
        CordBuffer() = default;

        /// total = block_reserve + prefix_reserve
        CordBuffer(size_t block_reserve, size_t prefix_reserve = 0);

        CordBuffer(const CordBuffer &) = delete;

        CordBuffer &operator=(const CordBuffer &) = delete;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        CordBuffer(CordBuffer &&rhs) noexcept
            : _views(std::move(rhs._views)),
              _total_size(std::exchange(rhs._total_size, 0)),
              _index(std::exchange(rhs._index, 0)),
              _view_start(std::exchange(rhs._view_start, 0)),
              _lease(std::move(rhs._lease)) {
            // After moving _views, rhs._views is empty. Its _lease is now moved,
            // so rhs._lease will have _capacity = 0 (since BufferLease's move constructor
            // moves the spans and resets the source to empty). That is desirable.
            // No further action needed.
        }

        // Move assignment: release current resources, then transfer from rhs.
        CordBuffer &operator=(CordBuffer &&rhs) noexcept {
            if (this != &rhs) {
                // Free current resources

                // Transfer ownership
                _views = std::move(rhs._views);
                _total_size = std::exchange(rhs._total_size, 0);
                _index = std::exchange(rhs._index, 0);
                _view_start = std::exchange(rhs._view_start, 0);
                _lease = std::move(rhs._lease);
            }
            return *this;
        }

        ~CordBuffer() {

        }

        template<size_t OA, size_t OB>
        [[nodiscard]] bool share_able_to(const CordBuffer<OA, OB> &rhs) const;

        [[nodiscard]] size_t block_size() const;

        [[nodiscard]] size_t alignment() const;


        /// @brief Return the `idx`-th logical block as a string_view (read‑only).
        std::string_view block_view(size_t idx) const;


        [[nodiscard]] size_t size() const;

        [[nodiscard]] size_t blocks() const;

        size_t readable_blocks() const;

        size_t prepend_blocks() const;


        turbo::Status append(std::string_view data);

        turbo::Status append(const void *data, size_t size);


    protected:
        std::vector<Buffer<char, Alignment>> _views; ///< All block views (including Umount ones).
        size_t _total_size{0}; ///< Total logical bytes stored.
        size_t _index{0}; ///< Index of the view currently being written (next commit position).
        size_t _view_start{0}; ///< First logical block index (skip Umount prefix).
        BufferLease _lease; ///< Active lease, if any.
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines
    ///
    ///////////////////////////////////////////////////////////////////////////
    ///


    /// @brief Access a specific block as a string_view for read-only inspection.
    template<size_t Alignment, size_t BlockSize>
    inline std::string_view CordBuffer<Alignment, BlockSize>::block_view(size_t idx) const {
        auto ridx = idx + _view_start;
        if (ridx >= _views.size() || _views[ridx].length == 0) {
            return {};
        }

        const auto &v = _views[ridx];
        return {v.block->data + v.offset, v.length};
    }


    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    inline bool CordBuffer<Alignment, BlockSize>::share_able_to(const CordBuffer<OA, OB> &rhs) const {
        TURBO_UNUSED(rhs);
        return (Alignment == OA) && (BlockSize % OB == 0);
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBuffer<Alignment, BlockSize>::block_size() const {
        return BlockSize;
    }


    /// @brief Total logical bytes in the CordBuffer.
    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBuffer<Alignment, BlockSize>::size() const {
        return _total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBuffer<Alignment, BlockSize>::blocks() const {
        return _views.size() - _view_start;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBuffer<Alignment, BlockSize>::readable_blocks() const {
        if (_index == _views.size() || _views[_index].length == 0) {
            return _index - _view_start;
        }
        return _index - _view_start + 1;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBuffer<Alignment, BlockSize>::prepend_blocks() const {
        return _view_start;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBuffer<Alignment, BlockSize>::alignment() const {
        return Alignment;
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBuffer<Alignment, BlockSize>::append(std::string_view data) {
        return append(const_cast<char *>(data.data()), data.size());
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBuffer<Alignment, BlockSize>::append(const void *data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();

        /// INVARIANT: No append allowed during an active borrowing session.
        DKCHECK(!_lease.borrowed()) << "logic error: appending during borrowing";

        const char *src = static_cast<const char *>(data);
        auto remnain = size;
        while (remnain > 0) {
            if (_views.empty() || _views.back().size() == _views.back().size()) {
                _views.push_back(Buffer<char, Alignment>());
                _views.back().reserve(BlockSize);
            }
            auto app_len = std::min(remnain, _views.back().capacity() - _views.back().size());
            _views.back().append(src, app_len);
            remnain -= app_len;
            src += app_len;
        }

        return  turbo::OkStatus();
    }


    /// total = block_reserve + prefix_reserve
    template<size_t Alignment, size_t BlockSize>
    CordBuffer<Alignment, BlockSize>::CordBuffer(size_t block_reserve, size_t prefix_reserve) {
        _views.reserve(prefix_reserve + block_reserve);
        _view_start = prefix_reserve;
        _index = prefix_reserve;
    }


} // namespace fermat
