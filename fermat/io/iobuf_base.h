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
#include <fermat/container/stl.h>
#include <fermat/memory/allocator.h>
#include <type_traits>
#include <turbo/log/check.h>
#include <turbo/log/logging.h>
#include <turbo/utility/status.h>

namespace fermat {
    /// @brief A managed view for writing into borrowed IOBuf segments.
    ///
    /// Lease provides a safe, sequential writing interface over discrete memory spans.
    /// It prevents common pitfalls like manual pointer arithmetic and buffer overflows
    /// by encapsulating the span collection and internal offsets.
    class IOBufBase;

    class Lease {
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
        ///   4. Finally, commit the lease to the IOBuf via IOBufBase::commit().
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
        /// @brief Default constructor (only IOBufBase can create a Lease).
        Lease() = default;

        /// @brief Initialize the lease with multiple non‑contiguous writable spans.
        /// @param sp Vector of spans borrowed from IOBufBase.
        void set(std::vector<turbo::span<char> > sp);

        /// @brief Initialize the lease with a single contiguous writable span.
        /// @param sp A single span borrowed from IOBufBase.
        void set(turbo::span<char> sp);

        void clear_lease();

        /// Copy/move constructors and assignment operators are defaulted.
        /// They are kept private because Lease instances are not meant to be
        /// copied or moved by users; only IOBufBase manages them.
        Lease(Lease &) = default;

        Lease(Lease &&) noexcept = default;

        Lease &operator=(const Lease &) = default;

        Lease &operator=(Lease &&) noexcept = default;

        friend class IOBufBase;


        friend class IOBufBase;

    private:
        std::vector<turbo::span<char> > _spans;
        size_t _total_size{0};
        size_t _index{0};
        size_t _offset{0};
        size_t _capacity{0};
    };

    /// @brief IOBufBase – the core zero‑copy buffer manager.
    ///
    /// IOBufBase manages a collection of reference‑counted memory blocks.
    /// It supports two modes of operation:
    ///   - External writing via `borrow()` + `Lease` + `commit()`
    ///   - Direct appending of data (copy) via `append()`
    ///   - Sharing or moving blocks between IOBuf instances
    ///   - Consuming data from the head with `custom()` or `pop_front()`
    ///
    /// The `borrow()` / `commit()` pair is the primary zero‑copy interface:
    ///   1. `borrow()` returns a `Lease*` that gives write access to internal buffers.
    ///   2. Fill the lease using `Lease::write()` (copy) or `Lease::visit_remaining()`
    ///      + external fill (e.g., readv). Then call `advance(n)`.
    ///   3. `commit(lease)` finalises the write, updates block states and total size.
    ///
    /// All operations that modify the buffer layout are prohibited while a lease
    /// is active (checked by `is_borrowing()`).
    class IOBufBase {
    public:
        // ------------------------------------------------------------------------
        // Block and block view definitions
        // ------------------------------------------------------------------------

        struct LeaseClearGuard {
            LeaseClearGuard(IOBufBase *buf, Lease *l) :_buf(buf), _l(l) {
                KCHECK(buf != nullptr);
                KCHECK(l != nullptr);
            }
            ~LeaseClearGuard() {
                _l->clear();
                _buf->commit(_l);
            }
        private:
            IOBufBase *_buf;
            Lease *_l;
        };

        struct LeaseGuard {
            LeaseGuard(IOBufBase *buf, Lease *l) :_buf(buf), _l(l) {
                KCHECK(buf != nullptr);
                KCHECK(l != nullptr);
            }
            ~LeaseGuard() {
                _buf->commit(_l);
            }
        private:
            IOBufBase *_buf;
            Lease *_l;
        };
        /// @brief A reference‑counted raw memory block.
        struct Block {
            Block(char *d, size_t cap) : data(d), capacity(cap) {
            }

            Block() = default;

            /// @brief True if this block is shared by more than one IOBuf.
            bool is_shared() const noexcept {
                return ref_count.load(std::memory_order_acquire) > 1;
            }

            /// @brief Increase reference count (used when sharing a block).
            void share() {
                ref_count.fetch_add(1, std::memory_order_relaxed);
            }

            std::atomic<uint32_t> ref_count{0};
            uint32_t capacity{0};
            char *data{nullptr};
        };

        /// @brief Logical view of a block inside an IOBuf.
        ///        Each view holds a pointer to a physical Block and a sub‑range
        ///        (offset, length) that is part of the logical buffer.
        enum class BlockStatus : uint8_t {
            Writeable = 0, ///< Block is owned and writable.
            Borrowing = 1, ///< Block is currently leased out for writing.
            Reference = 2, ///< Block is shared from another IOBuf (read‑only).
            Immutable = 3, ///< Block is sealed (no further writes allowed).
            Umount = 4 ///< Block has been logically removed.
        };

        struct BlockView {
            Block *block{nullptr};
            uint32_t offset{0};
            uint32_t length{0};
            BlockStatus status{BlockStatus::Writeable};

            uint32_t end_offset() const { return offset + length; }

            /// @brief Returns a writable span covering the currently unused tail of the block.
            turbo::span<char> write_able() {
                return turbo::span<char>(block->data + length + offset,
                                         block->capacity - length - offset);
            }
        };

        // ------------------------------------------------------------------------
        // Public API
        // ------------------------------------------------------------------------

        virtual ~IOBufBase() = default;

        /// @brief Checks whether this IOBuf can share its blocks with `rhs`.
        ///        Sharing is possible when alignments match and this block size
        ///        is at least as large as `rhs.block_size()`.
        [[nodiscard]] bool share_able_to(const IOBufBase &rhs);

        [[nodiscard]] size_t block_size() const;

        [[nodiscard]] size_t alignment() const;

        /// @brief Total writable space available in the last block(s).
        ///        Returns an error if a lease is currently active.
        turbo::Result<size_t> write_able_size() const;

        // ------------------------------------------------------------------------
        // Borrow / commit – zero‑copy write interface
        // ------------------------------------------------------------------------

        /// @brief Borrow the next writable block (or allocate a new one) and return a `Lease`.
        ///
        /// The returned `Lease*` provides methods to write data (`write()`), or to obtain
        /// raw pointers for zero‑copy system calls (`visit_remaining()` + `advance()`).
        /// After filling the lease, the caller MUST call `commit(lease)` to finalise.
        ///
        /// Only one lease may be active at a time; subsequent `borrow()` calls will
        /// fail with `kUnavailable` until the current lease is committed.
        ///
        /// Overloads:
        ///   - `borrow()`              : takes the next writable piece (any size).
        ///   - `borrow(byte_size, combine)` : ensures at least `byte_size` capacity.
        ///         If `combine` > block_size, the system attempts to allocate a
        ///         single contiguous block of size `combine` (rounded up to block size).
        ///
        /// @return On success, a non‑null `Lease*` owned by this IOBuf.
        ///         On failure, an error status.
        turbo::Result<Lease *> borrow();

        turbo::Result<Lease *> borrow(size_t byte_size, std::optional<int> combine = std::nullopt);

        /// @brief Commit a previously borrowed lease.
        ///
        /// This call:
        ///   - Transfers the data written into the lease (via `write()` or `advance()`)
        ///     to the IOBuf, updating block lengths and total size.
        ///   - Releases the lease, making the IOBuf available for further operations.
        ///   - Resets the internal lease state.
        ///
        /// The lease pointer MUST be the one returned by `borrow()` from `this` IOBuf.
        /// Passing a foreign lease or committing twice will trigger a fatal CHECK.
        ///
        /// @param lease The lease to commit (must belong to this IOBuf).
        void commit(Lease *lease);

        // ------------------------------------------------------------------------
        // Utilities to reclaim trailing space
        // ------------------------------------------------------------------------

        /// @brief Remove empty writable blocks from the end.
        ///        Only affects blocks with status `Writeable` and zero `length`.
        ///        Returns the number of bytes reclaimed.
        turbo::Result<size_t> shrink();

        /// @brief Seal the last writable block (make it Immutable) and remove any
        ///        trailing empty blocks afterwards.
        ///        Returns the number of bytes reclaimed (empty blocks only).
        turbo::Result<size_t> shrink_immutable();

        // ------------------------------------------------------------------------
        // Direct appending (copy)
        // ------------------------------------------------------------------------

        /// @brief Copy data into the IOBuf (append). May allocate new blocks if needed.
        turbo::Status append(std::string_view data);

        turbo::Status append(void *data, size_t size);

        // ------------------------------------------------------------------------
        // Read‑only access to blocks
        // ------------------------------------------------------------------------

        /// @brief Return the `idx`-th logical block as a string_view (read‑only).
        std::string_view block_view(size_t idx) const;

        /// @brief Number of prefix (Umount) blocks that are reserved for prepend operations.
        size_t prepend_blocks() const;

        /// @brief Total number of bytes stored in this IOBuf.
        [[nodiscard]] size_t size() const;

        // ------------------------------------------------------------------------
        // Prepend and append operations (sharing / moving)
        // ------------------------------------------------------------------------

        /// @brief Prepend this IOBuf's data to `lhs` (sharing if compatible, else copying).
        turbo::Status prepend_to(IOBufBase &lhs);

        /// @brief Move the content of `lhs` to the front of this IOBuf.
        ///        Requires compatible alignment / block size, otherwise returns error.
        turbo::Status prepend(IOBufBase &&lhs);

        /// @brief Move the content of `lhs` to the end of this IOBuf.
        ///        Requires compatible alignment / block size, otherwise returns error.
        turbo::Status append(IOBufBase &&lhs);

        /// @brief Append this IOBuf's data to `rhs` (sharing if compatible, else copying).
        turbo::Status append_to(IOBufBase &rhs);

        // ------------------------------------------------------------------------
        // Low‑level inspection and consumption
        // ------------------------------------------------------------------------

        /// @brief Consume `n` bytes from the head of the buffer.
        ///        Fully consumed blocks are released (if refcount reaches zero).
        turbo::Status custom(size_t n);

        /// @brief Number of logical blocks (excluding Umount blocks).
        ///        This includes all blocks in the IOBuf, including those that have been
        ///        partially written but are still writable, as well as immutable blocks.
        ///        Use this only for debugging or introspection; for accessing readable
        ///        data, prefer `readable_blocks()` and `readable_peek()`.
        [[nodiscard]] size_t blocks() const;

        /// @brief Get a pointer to the `idx`-th logical block view (read‑only).
        ///        This gives raw access to any block, regardless of whether it contains
        ///        readable data (e.g., a block that is still writable or has been
        ///        fully committed). Intended for debugging or low‑level inspection.
        const BlockView *peek(size_t idx) const;

        /// @brief Number of blocks that actually contain readable (committed) data.
        ///        This is the count of blocks that have non‑zero `length` and are
        ///        logically part of the buffer. It excludes empty writable tails
        ///        and `Umount` blocks. This should be used for normal iteration
        ///        over the data.
        size_t readable_blocks() const;

        /// @brief Get a pointer to the readable block at logical index `idx`.
        ///        The index is relative to the first readable block (i.e., block 0
        ///        corresponds to the first block with data). Returns `nullptr` if
        ///        `idx` is out of bounds or the block has no readable data.
        ///        This is the recommended way to traverse the readable content
        ///        of the IOBuf.
        const BlockView *readable_peek(size_t idx) const;

        /// @brief True if there is an active lease (i.e., the buffer is borrowed for writing).
        bool is_borrowing() const;

    protected:
        // ------------------------------------------------------------------------
        // Internal helpers
        // ------------------------------------------------------------------------
        static Allocator<Block> alloc;

        Block *create_block(size_t nblock);

        void release_block(Block *block);

        void alloc_block(size_t n, bool combine);

        size_t do_shrink_immutable();

        size_t do_shrink();

        void do_release();

        turbo::Result<Lease *> get_combine_write_able(size_t byte_size, int combine);

        turbo::Status prepend_share_to(IOBufBase &lhs);

        turbo::Status prepend_copy_to(IOBufBase &lhs);

        turbo::Status append_share_to(IOBufBase &rhs);

        turbo::Status append_copy_to(IOBufBase &rhs);

    protected:
        size_t _alignment{0}; ///< Memory alignment for all allocations.
        size_t _block_size{0}; ///< Size of a standard block (may be combined for larger requests).

        std::vector<BlockView> _views; ///< All block views (including Umount ones).
        size_t _total_size{0}; ///< Total logical bytes stored.
        size_t _index{0}; ///< Index of the view currently being written (next commit position).
        size_t _view_start{0}; ///< First logical block index (skip Umount prefix).
        Lease _lease; ///< Active lease, if any.
    };

    ///////////////////////////////////////////////////////////////////////////
    ///

    inline turbo::Result<size_t> IOBufBase::shrink() {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink();
    }

    inline turbo::Result<size_t> IOBufBase::shrink_immutable() {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink_immutable();
    }

    /// @brief Access a specific block as a string_view for read-only inspection.
    inline std::string_view IOBufBase::block_view(size_t idx) const {
        auto ridx = idx + _view_start;
        if (ridx >= _views.size() || _views[ridx].length == 0) {
            return {};
        }

        const auto &v = _views[ridx];
        return {v.block->data + v.offset, v.length};
    }

    inline const IOBufBase::BlockView *IOBufBase::peek(size_t idx) const {
        if (idx >= _views.size()) {
            return nullptr;
        }

        const auto &v = _views[idx];
        return &v;
    }

    inline const IOBufBase::BlockView *IOBufBase::readable_peek(size_t idx) const {
        auto ridx = idx + _view_start;
        if (idx >= _views.size() || ridx >=  _views.size() || _views[ridx].length == 0) {
            return nullptr;
        }

        const auto &v = _views[ridx];
        return &v;
    }

    inline bool IOBufBase::share_able_to(const IOBufBase &rhs) {
        return _alignment == rhs._alignment && _block_size >= rhs._block_size;
    }

    inline size_t IOBufBase::block_size() const {
        return _block_size;
    }

    inline size_t IOBufBase::alignment() const {
        return _alignment;
    }

    inline bool IOBufBase::is_borrowing() const {
        return _lease.borrowed();
    }

    inline size_t IOBufBase::prepend_blocks() const {
        return _view_start;
    }

    /// @brief Total logical bytes in the IOBuf.
    inline size_t IOBufBase::size() const {
        return _total_size;
    }


    inline size_t IOBufBase::blocks() const {
        return _views.size() - _view_start;
    }

    inline size_t IOBufBase::readable_blocks() const {
        if (_index == _views.size() || _views[_index].length == 0) {
            return _index - _view_start;
        }
        return _index - _view_start + 1;
    }
} // namespace fermat
