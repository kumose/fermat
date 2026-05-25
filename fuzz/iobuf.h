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
#include <fermat/io/buffer_lease.h>
#include <fermat/container/stl.h>
#include <memory>
#include <new>

namespace fermat {
    template<size_t A>
    struct BlockBuffer {
        static constexpr size_t kAlignment = A;

        BlockBuffer() = default;

        /// @brief True if this block is shared by more than one IOBuf.
        [[nodiscard]] bool is_shared() const noexcept {
            return ref_count.load(std::memory_order_acquire) > 1;
        }

        /// @brief Increase reference count (used when sharing a block).
        void share() {
            ref_count.fetch_add(1, std::memory_order_relaxed);
        }


        Buffer<char, A> buffer;
        std::atomic<uint64_t> ref_count{0};
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

    template<size_t A, size_t B>
    struct BlockView {
        static constexpr size_t kAlignment = A;
        static constexpr size_t kBlockSize = B;

        BlockBuffer<A> *block{nullptr};

        BlockStatus status{BlockStatus::Writeable};

    private:
        uint32_t offset{0};
        uint32_t length{0};

    public:
        [[nodiscard]] uint32_t end_offset() const { return offset + length; }

        /// @brief Returns a writable span covering the currently unused tail of the block.
        turbo::span<char> write_able() {
            return turbo::span<char>(block->buffer.data() + length + offset,
                                     block->buffer.capacity() - length - offset);
        }


        [[nodiscard]] size_t capacity() const {
            if (!block) {
                return 0;
            }
            return block->buffer.capacity();
        }

        [[nodiscard]] size_t remaining() const {
            if (!block) {
                return 0;
            }
            return block->buffer.capacity() - offset - length;
        }

        [[nodiscard]] bool full_fill() const {
            if (!block) {
                return true;
            }
            return block->buffer.capacity() - offset - length == 0;
        }

        [[nodiscard]] size_t size() const {
            return length;
        }

        [[nodiscard]] const char *data() const {
            if (!block) {
                return nullptr;
            }
            return block->buffer.data() + offset;
        }


        [[nodiscard]] size_t write_able_size() const {
            return block->buffer.capacity() - length - offset;
        }

        bool set_write_able() {
            if (status == BlockStatus::Writeable) {
                return true;
            }
            if (status == BlockStatus::Immutable && length + offset < block->buffer.capacity()) {
                status = BlockStatus::Writeable;
                return true;
            }
            return false;
        }

        template<size_t OB>
        void operator=(BlockView<A, OB> rhs) {
            KCHECK(block == nullptr) << "may lease memory";
            static_assert(OB % B == 0, "block not aligned");
            block = rhs.block;
            length = rhs.length;
            offset = rhs.offset;
            status = rhs.status;
        }
    };


    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class IOBuf {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr bool is_iobuf = true;
        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;

        using block_type = BlockBuffer<kAlignment>;
        using block_view_type = BlockView<kAlignment, kBlockSize>;

    public:
        IOBuf() = default;

        /// total = block_reserve + prefix_reserve
        IOBuf(size_t block_reserve, size_t prefix_reserve = 0);

        IOBuf(const IOBuf &) = delete;

        IOBuf &operator=(const IOBuf &) = delete;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        IOBuf(IOBuf &&rhs) noexcept
            : _views(std::move(rhs._views)),
              _total_size(std::exchange(rhs._total_size, 0)),
              _index(std::exchange(rhs._index, 0)),
              _view_start(std::exchange(rhs._view_start, 0)),
              _lease(std::move(rhs._lease)) {
            // After moving _views, rhs._views is empty. Its _lease is now moved,
            // so rhs._lease will have _capacity = 0 (since DeprecateBufferLease's move constructor
            // moves the spans and resets the source to empty). That is desirable.
            // No further action needed.
        }

        // Move assignment: release current resources, then transfer from rhs.
        IOBuf &operator=(IOBuf &&rhs) noexcept {
            if (this != &rhs) {
                // Free current resources
                do_release();

                // Transfer ownership
                _views = std::move(rhs._views);
                _total_size = std::exchange(rhs._total_size, 0);
                _index = std::exchange(rhs._index, 0);
                _view_start = std::exchange(rhs._view_start, 0);
                _lease = std::move(rhs._lease);
            }
            return *this;
        }

        ~IOBuf() {
            do_release();
        }

        template<size_t OA, size_t OB>
        [[nodiscard]] bool share_able_to(const IOBuf<OA, OB> &rhs) const;

        [[nodiscard]] size_t block_size() const;

        [[nodiscard]] size_t alignment() const;

        turbo::Result<size_t> write_able_size() const;

        turbo::Result<DeprecateBufferLease *> borrow();

        turbo::Result<DeprecateBufferLease *> borrow(size_t byte_size, std::optional<int> combine = std::nullopt);

        void commit(DeprecateBufferLease *lease);

        turbo::Result<size_t> shrink();

        turbo::Result<size_t> shrink_immutable();


        [[nodiscard]] std::string flatten() const;

        template<size_t SA = Alignment>
        AlignedString<SA> flatten_aligned() const;

        size_t pop_front(size_t n, std::string *result = nullptr);

        /// @brief Return the `idx`-th logical block as a string_view (read‑only).
        [[nodiscard]] std::string_view block_view(size_t idx) const;

        const block_view_type *peek(size_t idx) const;

        const block_view_type *readable_peek(size_t idx) const;

        [[nodiscard]] bool is_borrowing() const;

        [[nodiscard]] size_t size() const;

        [[nodiscard]] size_t blocks() const;

        [[nodiscard]] size_t readable_blocks() const;

        [[nodiscard]] size_t prepend_blocks() const;

        turbo::Status custom(size_t n);

        turbo::Status append(std::string_view data);

        turbo::Status append(const void *data, size_t size);


        template<size_t OA, size_t OB>
        turbo::Status append_copy_to(IOBuf<OA, OB> &rhs, std::optional<size_t> block_size = std::nullopt) const;

        template<size_t OA, size_t OB>
        turbo::Status append_share_to(IOBuf<OA, OB> &rhs) const;

        template<size_t OB>
        turbo::Status append_move_to(IOBuf<Alignment, OB> &rhs);

        template<size_t OB>
        turbo::Status append_to(IOBuf<Alignment, OB> &rhs, std::optional<size_t> block_size = std::nullopt) const;

        template<size_t OA, size_t OB, std::enable_if_t<OA != Alignment, int> = 0>
        turbo::Status append_to(IOBuf<OA, OB> &rhs, std::optional<size_t> block_size = std::nullopt) const;

        template<size_t OB>
        turbo::Status append(IOBuf<Alignment, OB> &&lhs);

        template<size_t OA, size_t OB>
        turbo::Status prepend_copy_to(IOBuf<OA, OB> &lhs, std::optional<size_t> block_size = std::nullopt) const;

        template<size_t OA, size_t OB>
        turbo::Status prepend_share_to(IOBuf<OA, OB> &lhs) const;

        template<size_t OB>
        turbo::Status prepend_move_to(IOBuf<Alignment, OB> &lhs);

        /// automic select copy or share
        template<size_t OB>
        turbo::Status prepend_to(IOBuf<Alignment, OB> &lhs, std::optional<size_t> block_size = std::nullopt) const;

        template<size_t OA, size_t OB, std::enable_if_t<OA != Alignment, int> = 0>
        turbo::Status prepend_to(IOBuf<OA, OB> &lhs, std::optional<size_t> block_size = std::nullopt) const;

        template<size_t OB>
        turbo::Status prepend(IOBuf<Alignment, OB> &&lhs);

        template<size_t OA, size_t OB>
        turbo::Result<IOBuf<OA, OB> > copy_to(size_t block_size = OB) const;

        template<size_t OB>
        turbo::Result<IOBuf<Alignment, OB> > share_to() const;

    protected:
        size_t do_shrink_immutable();

        size_t do_shrink();

        void do_release();

        block_type *create_block(size_t nblock);

        void release_block(block_type *block);

        void alloc_block(size_t n, bool combine);

        turbo::Result<DeprecateBufferLease *> get_combine_write_able(size_t byte_size, int combine);

    protected:
        std::vector<block_view_type> _views; ///< All block views (including Umount ones).
        size_t _total_size{0}; ///< Total logical bytes stored.
        size_t _index{0}; ///< Index of the view currently being written (next commit position).
        size_t _view_start{0}; ///< First logical block index (skip Umount prefix).
        DeprecateBufferLease _lease; ///< Active lease, if any.
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines
    ///
    ///////////////////////////////////////////////////////////////////////////
    ///
    template<size_t Alignment, size_t BlockSize>
    inline turbo::Result<size_t> IOBuf<Alignment, BlockSize>::shrink() {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink();
    }

    template<size_t Alignment, size_t BlockSize>
    inline turbo::Result<size_t> IOBuf<Alignment, BlockSize>::shrink_immutable() {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink_immutable();
    }


    /// @brief Access a specific block as a string_view for read-only inspection.
    template<size_t Alignment, size_t BlockSize>
    inline std::string_view IOBuf<Alignment, BlockSize>::block_view(size_t idx) const {
        auto ridx = idx + _view_start;
        if (ridx >= _views.size() || _views[ridx].length == 0) {
            return {};
        }

        const auto &v = _views[ridx];
        return {v.block->data + v.offset, v.length};
    }

    template<size_t Alignment, size_t BlockSize>
    inline const typename IOBuf<Alignment, BlockSize>::block_view_type *IOBuf<Alignment, BlockSize>::peek(
        size_t idx) const {
        if (idx >= _views.size()) {
            return nullptr;
        }

        const auto &v = _views[idx];
        return &v;
    }

    template<size_t Alignment, size_t BlockSize>
    inline const typename IOBuf<Alignment, BlockSize>::block_view_type *IOBuf<Alignment, BlockSize>::readable_peek(
        size_t idx) const {
        auto ridx = idx + _view_start;
        if (idx >= _views.size() || ridx >= _views.size() || _views[ridx].length == 0) {
            return nullptr;
        }

        const auto &v = _views[ridx];
        return &v;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    inline bool IOBuf<Alignment, BlockSize>::share_able_to(const IOBuf<OA, OB> &rhs) const {
        TURBO_UNUSED(rhs);
        return (Alignment == OA) && (BlockSize % OB == 0);
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t IOBuf<Alignment, BlockSize>::block_size() const {
        return BlockSize;
    }

    template<size_t Alignment, size_t BlockSize>
    inline bool IOBuf<Alignment, BlockSize>::is_borrowing() const {
        return _lease.borrowed();
    }

    /// @brief Total logical bytes in the IOBuf.
    template<size_t Alignment, size_t BlockSize>
    inline size_t IOBuf<Alignment, BlockSize>::size() const {
        return _total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t IOBuf<Alignment, BlockSize>::blocks() const {
        return _views.size() - _view_start;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t IOBuf<Alignment, BlockSize>::readable_blocks() const {
        if (_index == _views.size() || _views[_index].length == 0) {
            return _index - _view_start;
        }
        return _index - _view_start + 1;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t IOBuf<Alignment, BlockSize>::prepend_blocks() const {
        return _view_start;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t IOBuf<Alignment, BlockSize>::alignment() const {
        return Alignment;
    }

    template<size_t Alignment, size_t BlockSize>
    typename IOBuf<Alignment, BlockSize>::block_type *IOBuf<Alignment,
        BlockSize>::create_block(size_t nblock) {
        auto *b = TieredAllocator<block_type, 0>::pooled_alloc(nullptr);
        ::new(static_cast<void *>(b)) block_type();
        b->buffer.resize(nblock * BlockSize);
        return b;
    }


    template<size_t Alignment, size_t BlockSize>
    void IOBuf<Alignment, BlockSize>::release_block(block_type *block) {
        // Use acq_rel to ensure visibility across threads
        if (block->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // 1. First, call the destructor to free 'data'
            std::destroy_at(block);
            TieredAllocator<block_type, 0>::pooled_free(block, 1);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void IOBuf<Alignment, BlockSize>::do_release() {
        /// 1. Iterate through all views and release their corresponding blocks.
        /// Each call to block->release() handles the ref_count and memory deallocation.
        for (auto &v: _views) {
            if (v.block) {
                release_block(v.block);
                v.block = nullptr;
            }
        }

        /// 2. Clear the vector to prevent dangling pointers.
        _views.clear();

        /// 3. Reset logical metadata to initial state.
        _total_size = 0;
        _index = 0;
        _view_start = 0;
    }

    template<size_t Alignment, size_t BlockSize>
    void IOBuf<Alignment, BlockSize>::alloc_block(size_t n, bool combine) {
        if (combine) {
            auto b = create_block(n);
            b->share();
            block_view_type view;
            view.block = b;
            _views.push_back(view);
            return;
        }

        for (size_t i = 0; i < n; i++) {
            auto b = create_block(1);
            b->share();
            block_view_type view;
            view.block = b;
            _views.push_back(view);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::custom(size_t n) {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (n > _total_size) {
            return turbo::out_of_range_error("not enough buffer to custom [", _total_size, " vs ", n, "]");
        }
        size_t remin = n;
        while (remin > 0) {
            auto &v = _views[_view_start];
            if (remin > v.length) {
                remin -= v.length;
                release_block(v.block);
                v.block = nullptr;
                v.status = BlockStatus::Umount;
                ++_view_start;
            } else if (remin == v.length) {
                /// this core is most complex, carefull
                /// 1. the latst immutable and custom all
                /// 2. the data end with _index some block
                v.length = 0;
                v.offset += remin;
                remin = 0;
                if (_index > _view_start) {
                    release_block(v.block);
                    v.block = nullptr;
                    v.status = BlockStatus::Umount;
                    ++_view_start;
                }
                /// _index == _view_start
                /// still have space to write,
                /// do not release this
            } else {
                v.offset += remin;
                v.length -= remin;
                remin = 0;
            }
        }
        _total_size -= n;
        if (_total_size == 0 && _view_start == _index && _view_start != 0) {
            /// all customed
            auto bn = _views.size() - _view_start;
            for (size_t i = 0; i < bn; ++i) {
                _views[i] = _views[_view_start + i];
            }
            _views.resize(bn);
            _view_start = 0;
            _index = 0;
        }

        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    void IOBuf<Alignment, BlockSize>::commit(DeprecateBufferLease *l) {
        KCHECK(l == &_lease) << "Fatal: Lease does not belong to this IOBuf. "
                     << "Expected internal lease address " << &_lease
                     << ", got " << l << ". Possible double commit or foreign lease.";
        if (!_lease.borrowed() || _index == _views.size()) {
            return;
        }
        auto n = _lease.size();
        /// Starting point must be a writable owner block.
        /// If append() happened before commit(), it's a caller's logical error.
        if (n > 0) {
            while (n > 0) {
                DKCHECK(_index < _views.size()) << "Commit exceeds buffer boundaries";
                auto &v = _views[_index];

                /// Invariant: Current index for committing MUST be an owner.
                /// We don't skip; we error out because the sequence is broken.
                DKCHECK(
                    v.status == BlockStatus::
                    Borrowing) << "Logic error: Attempting to commit into a read-only (shared) view";

                uint32_t remaining = v.remaining();

                uint32_t consume = static_cast<uint32_t>(std::min((size_t) remaining, n));

                v.length += consume;
                _total_size += consume;
                n -= consume;

                if (v.full_fill()) {
                    v.status = BlockStatus::Immutable;
                    _index++;
                } else {
                    /// not full
                    v.status = BlockStatus::Writeable;
                }
            }
        }

        _lease.clear_lease();
        for (auto bindex = _index; bindex < _views.size(); bindex++) {
            auto &v = _views[bindex];
            switch (v.status) {
                case BlockStatus::Borrowing: {
                    v.status = BlockStatus::Writeable;
                    break;
                }
                case BlockStatus::Writeable: {
                    return;
                }
                default:
                    TURBO_UNREACHABLE();
            }
        }
    }

    template<size_t Alignment, size_t BlockSize>
    size_t IOBuf<Alignment, BlockSize>::do_shrink() {
        if (_views.empty()) {
            return 0;
        }

        size_t ret = 0;
        bool done = false;
        while (!done && !_views.empty()) {
            auto &v = _views.back();
            switch (v.status) {
                case BlockStatus::Writeable: {
                    if (v.size() == 0) {
                        ret += v.capacity();
                        release_block(v.block);
                        _views.pop_back();
                        break;
                    }
                    done = true;
                    break;
                }
                case BlockStatus::Immutable:
                case BlockStatus::Reference: {
                    done = true;
                    break;
                }
                case BlockStatus::Borrowing:
                case BlockStatus::Umount:
                default:
                    TURBO_UNREACHABLE();
            }
        }
        if (!_views.empty() && _views.back().status == BlockStatus::Writeable) {
            _index = _views.size() - 1;
        } else {
            _index = _views.size();
        }
        return ret;
    }

    template<size_t Alignment, size_t BlockSize>
    size_t IOBuf<Alignment, BlockSize>::do_shrink_immutable() {
        if (_views.empty()) {
            return 0;
        }

        size_t ret = 0;
        bool done = false;
        while (!done && !_views.empty()) {
            auto &v = _views.back();
            switch (v.status) {
                case BlockStatus::Writeable: {
                    if (v.size() == 0) {
                        ret += v.capacity();
                        release_block(v.block);
                        _views.pop_back();
                        break;
                    }
                    v.status = BlockStatus::Immutable;
                    done = true;
                    break;
                }
                case BlockStatus::Immutable:
                case BlockStatus::Reference: {
                    done = true;
                    break;
                }
                case BlockStatus::Borrowing:
                case BlockStatus::Umount:
                default:
                    TURBO_UNREACHABLE();
            }
        }
        _index = _views.size();
        return ret;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<DeprecateBufferLease *> IOBuf<Alignment,
        BlockSize>::get_combine_write_able(
        size_t byte_size, int combine) {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        size_t n = (combine + BlockSize - 1) / BlockSize;

        /// no holds
        do_shrink_immutable();
        size_t wra{0};
        while (wra < byte_size) {
            alloc_block(n, true);
            wra += BlockSize * n;
        }
        wra = 0;
        std::vector<turbo::span<char> > vec;
        for (size_t i = _index; wra < byte_size && i < _views.size(); i++) {
            KCHECK(_views[i].status == BlockStatus::Writeable);
            _views[i].status = BlockStatus::Borrowing;
            auto s = _views[i].write_able();
            wra += s.size();
            vec.push_back(s);
        }
        _lease.set(vec);
        return &_lease;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<DeprecateBufferLease *> IOBuf<Alignment, BlockSize>::borrow(
        size_t byte_size, std::optional<int> combine) {
        /// *combine > BlockSize go to big block mode
        if (combine.has_value() && *combine > BlockSize) {
            return get_combine_write_able(byte_size, *combine);
        }

        TURBO_MOVE_OR_RAISE(auto wra, write_able_size());
        while (wra < byte_size) {
            alloc_block(1, false);
            wra += BlockSize;
        }
        wra = 0;
        std::vector<turbo::span<char> > vec;
        for (size_t i = _index; wra < byte_size && i < _views.size(); i++) {
            KCHECK(_views[i].status == BlockStatus::Writeable);
            _views[i].status = BlockStatus::Borrowing;
            auto s = _views[i].write_able();
            wra += s.size();
            vec.push_back(s);
        }
        _lease.set(vec);
        return &_lease;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<DeprecateBufferLease *> IOBuf<Alignment, BlockSize>::borrow() {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        TURBO_MOVE_OR_RAISE(auto wra, write_able_size());
        if (wra == 0) {
            alloc_block(1, false);
        }
        KCHECK(_views[_index].status == BlockStatus::Writeable) << " " << static_cast<int>(_views[_index].status);
        _views[_index].status = BlockStatus::Borrowing;
        _lease.set(_views[_index].write_able());
        return &_lease;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> IOBuf<Alignment, BlockSize>::write_able_size() const {
        if (_index == _views.size()) {
            return 0;
        }
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        size_t total_size{0};

        for (size_t i = _index; i < _views.size(); i++) {
            total_size += _views[i].write_able_size();
        }
        return total_size;
    }


    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend_copy_to(IOBuf<OA, OB> &lhs,
                                                               std::optional<size_t> block_size) const {
        auto s = OB;
        if (block_size.has_value() && *block_size > OB) {
            s = *block_size;
        }
        TURBO_MOVE_OR_RAISE(auto tmp_buf, (this->copy_to<OA, OB>(s)));
        // 2. O(1) zero-copy ownership splicing to the head of lhs
        return lhs.prepend(std::move(tmp_buf));
    }

    /// @brief Share current assets to the front of lhs and seal local write access.
    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend_share_to(IOBuf<OA, OB> &lhs) const {
        KCHECK(OA == Alignment) << "append_share_to requires same Alignment (source=" << Alignment
        << ", target=" << OA << ")";
        TURBO_MOVE_OR_RAISE(auto tmp, share_to<OB>());
        return lhs.prepend(std::move(tmp));
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend_move_to(IOBuf<Alignment, OB> &lhs) {
        return lhs.prepend(std::move(*this));
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend_to(IOBuf<Alignment, OB> &lhs,
                                                          std::optional<size_t> block_size) const {
        auto s = OB;
        if (block_size.has_value() && *block_size > OB) {
            s = *block_size;
        }

        if (share_able_to(lhs)) {
            return prepend_share_to(lhs);
        } else {
            return prepend_copy_to(lhs, s);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB, std::enable_if_t<OA != Alignment, int> >
    turbo::Status IOBuf<Alignment, BlockSize>::prepend_to(IOBuf<OA, OB> &lhs, std::optional<size_t> block_size) const {
        auto s = OB;
        if (block_size.has_value() && *block_size > OB) {
            s = *block_size;
        }
        return prepend_copy_to(lhs, s);
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::append_copy_to(IOBuf<OA, OB> &rhs,
                                                              std::optional<size_t> block_size) const {
        auto s = OB;
        if (block_size.has_value() && *block_size > OB) {
            s = *block_size;
        }
        TURBO_MOVE_OR_RAISE(auto tmp, (copy_to<OA, OB>(s)));
        return rhs.append(std::move(tmp));
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::append_share_to(IOBuf<OA, OB> &rhs) const {
        KCHECK(OA == Alignment) << "append_share_to requires same Alignment (source=" << Alignment
        << ", target=" << OA << ")";
        auto tmp = share_to<OB>();
        if (!tmp.ok()) return tmp.status();
        return rhs.append(std::move(*tmp));
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::append_move_to(IOBuf<Alignment, OB> &rhs) {
        return rhs.append(std::move(*this));
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    turbo::Result<IOBuf<OA, OB> > IOBuf<Alignment, BlockSize>::copy_to(size_t block_size) const {
        // 1. Instantiate the shell of the new target buffer
        IOBuf<OA, OB> target;

        if (_total_size == 0) {
            return std::move(target);
        }

        size_t src_idx = 0;
        size_t total_remaining = _total_size;
        auto lease = target.borrow(_total_size, block_size).value_or_die();
        // 2. Core Copy Loop entirely driven by target's exact borrow/commit API
        turbo::Status st;
        while (total_remaining > 0) {
            auto const *view = readable_peek(src_idx);
            st = lease->write(view->data(), view->size());
            if (!st.ok()) {
                break;
            }
            total_remaining -= view->size();
            src_idx++;
        }
        if (!st.ok()) {
            lease->clear();
            target.commit(lease);
            return st;
        }
        target.commit(lease);
        // Returns via RVO, target remains fully functional, writeable, and appendable for subsequent operations.
        return std::move(target);
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Result<IOBuf<Alignment, OB> > IOBuf<Alignment, BlockSize>::share_to() const {
        static_assert(BlockSize % OB == 0, "share_to requires BlockSize % OB == 0");
        if (_index != _views.size()) {
            return turbo::unavailable_error("IOBuf has writable tail blocks; call shrink_immutable() before share_to");
        }
        IOBuf<Alignment, OB> target;
        if (_total_size == 0) return target;

        for (size_t i = _view_start; i < _views.size(); ++i) {
            const auto &v = _views[i];
            if (v.length == 0) continue;
            v.block->share();

            typename IOBuf<Alignment, OB>::block_view_type new_view;
            new_view.block = reinterpret_cast<typename IOBuf<Alignment, OB>::block_type *>(v.block);
            new_view.offset = v.offset;
            new_view.length = v.length;
            new_view.status = BlockStatus::Reference;
            target._views.push_back(new_view);
        }
        target._total_size = _total_size;
        target._index = target._views.size();
        return target;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::append_to(IOBuf<Alignment, OB> &rhs,
                                                         std::optional<size_t> block_size) const {
        auto s = OB;
        if (block_size.has_value() && *block_size > OB) {
            s = *block_size;
        }
        if (share_able_to(rhs)) {
            return append_share_to(rhs);
        } else {
            return append_copy_to(rhs, s);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB, std::enable_if_t<OA != Alignment, int> >
    turbo::Status IOBuf<Alignment, BlockSize>::append_to(IOBuf<OA, OB> &rhs, std::optional<size_t> block_size) const {
        auto s = OB;
        if (block_size.has_value() && *block_size > OB) {
            s = *block_size;
        }
        return append_copy_to(rhs, s);
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend(IOBuf<Alignment, OB> &&lhs) {
        /// 1. Transaction & Identity Check.
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._lease.borrowed()) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }
        if (!lhs.share_able_to(*this)) {
            return turbo::invalid_argument_error("alignment size mismatch, cannot share blocks[this:", BlockSize,
                                                 " vs lhs:", BlockSize, "]");
        }

        if (static_cast<const void *>(this) == static_cast<const void *>(&lhs) || _total_size == 0)
            return
                    turbo::OkStatus();

        /// 1. Settle lhs to reclaim empty trailing blocks.
        lhs.do_shrink_immutable();
        size_t lhs_view_count = lhs._views.size() - lhs._view_start;
        auto li = _index - _view_start;

        turbo::span<block_view_type> views;
        turbo::span<block_view_type> local_views;
        std::vector<block_view_type> new_views;

        if (lhs_view_count < _view_start) {
            views = turbo::span<block_view_type>(_views.data() + _view_start - lhs_view_count, lhs_view_count);
        } else {
            new_views.resize(lhs_view_count + _views.size() - _view_start);
            views = turbo::span<block_view_type>(new_views.data(), lhs_view_count);
            local_views = turbo::span<block_view_type>(new_views.data() + lhs_view_count, _views.size() - _view_start);
        }

        for (size_t i = 0; i < views.size(); i++) {
            auto &v = lhs._views[i + lhs._view_start];
            views[i] = v;
        }

        for (size_t i = 0; i < local_views.size(); i++) {
            auto &v = _views[i + _view_start];
            local_views[i] = v;
        }

        if (local_views.empty()) {
            _view_start = _view_start - lhs_view_count;
        } else {
            _views.swap(new_views);
            _view_start = 0;
            _index = li + lhs_view_count;
        }
        _total_size += lhs._total_size;

        /// 5. Reset lhs carcass.
        lhs._views.clear();
        lhs._total_size = 0;
        lhs._index = 0;
        lhs._view_start = 0;
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OB>
    turbo::Status IOBuf<Alignment, BlockSize>::append(IOBuf<Alignment, OB> &&lhs) {
        static_assert(OB % BlockSize == 0, "block must aligned to this");
        /// 1. Transaction & Identity Check.
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._lease.borrowed()) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }

        if (!lhs.share_able_to(*this)) {
            return turbo::invalid_argument_error("alignment size mismatch, cannot share blocks[this:", BlockSize,
                                                 " vs lhs:", OB, "]");
        }
        if (static_cast<const void *>(this) == static_cast<const void *>(&lhs) || _total_size == 0)
            return
                    turbo::OkStatus();

        /// 2. Pre-process: Close local gaps to ensure contiguous logical flow.
        this->do_shrink_immutable();
        lhs.do_shrink();

        /// 3. Asset Seizure Loop.
        for (auto &v: lhs._views) {
            /// Move ownership: No share(), no release().
            /// We keep the original status (Writeable/Immutable) as we are the new master.
            if (v.status == BlockStatus::Umount) {
                continue;
            }
            _views.push_back(v);

            /// IMPORTANT: Nullify the source pointer so lhs won't release it.
            v.block = nullptr;
        }

        _total_size += lhs._total_size;

        /// 4. Reset write index.
        /// Since we just appended data, we reset _index to force a fresh look
        /// or a new allocation on the next write attempt.
        _index = _views.size();
        if (_index > 0) {
            auto &v = _views.back();
            if (v.set_write_able()) {
                _index = _views.size() - 1;
            }
        }

        /// 5. Clean up the carcass of lhs.
        /// do_release will now only free the empty/uncommitted blocks we left behind.
        lhs.do_release();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::append(std::string_view data) {
        return append(const_cast<char *>(data.data()), data.size());
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::append(const void *data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();

        /// INVARIANT: No append allowed during an active borrowing session.
        DKCHECK(!_lease.borrowed()) << "logic error: appending during borrowing";

        const char *src = static_cast<const char *>(data);


        TURBO_MOVE_OR_RAISE(auto span, borrow(size));
        //LeaseClearGuard(this, span);
        auto rs = span->write(src, size);
        if (!rs.ok()) {
            span->clear();
            commit(span);
            TURBO_RETURN_NOT_OK(rs);
        }

        commit(span);
        return turbo::OkStatus();
    }


    /// total = block_reserve + prefix_reserve
    template<size_t Alignment, size_t BlockSize>
    IOBuf<Alignment, BlockSize>::IOBuf(size_t block_reserve, size_t prefix_reserve) {
        _views.reserve(prefix_reserve + block_reserve);
        for (size_t i = 0; i < prefix_reserve; ++i) {
            block_view_type view;
            view.block = nullptr;
            view.status = BlockStatus::Umount;
            _views.push_back(view);
        }
        _view_start = prefix_reserve;
        _index = prefix_reserve;
    }

    template<size_t Alignment, size_t BlockSize>
    std::string IOBuf<Alignment, BlockSize>::flatten() const {
        std::string result;
        if (_views.empty() || _total_size == 0) return result;

        /// 1. Reserve memory at once to avoid multiple reallocations.
        result.reserve(_total_size);
        auto max_index = _index == _views.size() ? _views.size() : _index + 1;
        /// 2. Linear copy from all committed views.
        for (size_t i = _view_start; i < max_index; ++i) {
            auto &v = _views[i];
            result.append(v.data(), v.size());
        }
        return result;
    }


    /// @brief Flatten all fragment views into a single contiguous AlignedString.
    template<size_t Alignment, size_t BlockSize>
    template<size_t SA>
    AlignedString<SA> IOBuf<Alignment, BlockSize>::flatten_aligned() const {
        AlignedString<SA> result;
        if (_views.empty() || _total_size == 0) return result;

        /// 1. Reserve memory at once to avoid multiple reallocations.
        result.reserve(_total_size);

        /// 2. Linear copy from all committed views.
        auto max_index = _index == _views.size() ? _views.size() : _index + 1;
        for (size_t i = _view_start; i < max_index; ++i) {
            auto &v = _views[i];
            result.append(v.data(), v.size());
        }
        return result;
    }

    /// @brief Consume n bytes from the head of the IOBuf.
    /// @param n Number of bytes to consume.
    /// @param result Optional string to store the consumed data.
    /// @return Actual number of bytes consumed.
    template<size_t Alignment, size_t BlockSize>
    size_t IOBuf<Alignment, BlockSize>::pop_front(size_t n, std::string *result) {
        if (n == 0 || _total_size == 0) return 0;

        /// INVARIANT: Cannot modify the logical head during an active borrowing session.
        DKCHECK(!_lease.borrowed()) << "logic error: pop_front during borrowing";

        size_t to_pop = std::min(n, _total_size);
        size_t left = to_pop;

        if (result) {
            result->reserve(result->size() + to_pop);
        }

        /// Use a logical window (_view_start) to achieve O(1) consumption.
        while (left > 0 && _view_start < _views.size()) {
            auto &v = _views[_view_start];
            size_t consume = std::min((size_t) v.size(), left);

            if (result) {
                result->append(v.data(), consume);
            }

            if (consume == v.size()) {
                /// Case: Full View Umount.
                /// 1. Release the physical block's reference.
                release_block(v.block);
                v.block = nullptr;

                /// 2. Mark the status as Umount (Unloaded).
                v.status = BlockStatus::Umount;

                /// 3. Advance the logical start index.
                _view_start++;

                /// 4. Sync the write index to ensure it doesn't lag behind the head.
                if (_index < _view_start) {
                    _index = _view_start;
                }
            } else {
                /// Case: Partial consumption within a block.
                /// The view remains active, so status stays unchanged.
                v.offset += static_cast<uint32_t>(consume);
                v.length -= static_cast<uint32_t>(consume);
            }

            _total_size -= consume;
            left -= consume;
        }

        /// Final cleanup: if the buffer is entirely empty, reset the container to reclaim space.
        if (_total_size == 0) {
            _views.clear();
            _view_start = 0;
            _index = 0;
        }

        return to_pop;
    }

    ////////////////////////////////////////////////////
    ///
    template<typename T>
    struct is_iobuf : std::false_type {
        static constexpr size_t alignment = 0;
        static constexpr size_t block_size = 0;
    };

    template<size_t Alignment, size_t BlockSize>
    struct is_iobuf<fermat::IOBuf<Alignment, BlockSize> > : std::true_type {
        static constexpr size_t alignment = Alignment;
        static constexpr size_t block_size = BlockSize;
    };

    template<typename T>
    inline constexpr bool is_iobuf_v = is_iobuf<T>::value;
} // namespace fermat
