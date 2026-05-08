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
#include <fermat/io/iobuf_base.h>

namespace fermat {

    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class IOBuf : public IOBufBase {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr bool is_iobuf = true;
        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;

        /// @brief Compile-time selector for allocator type.
        /// If Alignment is 0, use general-purpose Allocator.
        /// If Alignment > 0, use AlignedAllocator with specified Alignment.
        using allocator_type = typename std::conditional<
            Alignment == 0,
            fermat::Allocator<char>,
            fermat::AlignedAllocator<char, Alignment>
        >::type;

        struct Block {
            static Allocator<Block> alloc;
            static allocator_type data_alloc;

            Block(char *d, size_t cap) : data(d), capacity(cap) {
            }

            /// @brief Constructor only handles internal state
            Block() = default;

            /// @brief Destructor ONLY cleans up the data buffer
            ~Block() {
                if (data) {
                    data_alloc.deallocate(data, capacity);
                    data = nullptr;
                }
            }

            bool is_shared() const noexcept {
                return ref_count.load(std::memory_order_acquire) > 1;
            }

            /// @brief The actual life-cycle controller
            void release() {
                // Use acq_rel to ensure visibility across threads
                if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    // 1. First, call the destructor to free 'data'
                    this->~Block();
                    // 2. Then, return the memory of the Header (this) to the pool
                    alloc.deallocate(this, 1);
                }
            }

            /// @brief Increment reference for sharing
            void share() {
                ref_count.fetch_add(1, std::memory_order_relaxed);
            }

            std::atomic<uint32_t> ref_count{0};
            uint32_t capacity{0};
            char *data{nullptr};

            /// @brief Factory for dynamic sizing.
            /// n n blocks
            static Block *acquire(size_t n) {
                auto *b = alloc.allocate(1);
                auto ptr = data_alloc.allocate(n * BlockSize);
                Allocator<Block>::construct(b, ptr, n * BlockSize);
                return b;
            }
        };

        enum class BlockStatus {
            /// owner write able
            Writeable = 0,
            /// Borrowing out
            Borrowing = 1,
            /// ref from other
            Reference = 2,
            /// shared and mark Immutable
            Immutable = 3,
            /// Umount
            Umount = 4
        };

        struct BlockView {
            Block *block{nullptr};
            uint32_t offset{0};
            uint32_t length{0};
            BlockStatus status{BlockStatus::Writeable};
            uint32_t end_offset() const { return offset + length; }

            turbo::span<char> write_able() {
                return turbo::span<char>(block->data + length + offset, block->capacity - length - offset);
            }
        };

    public:
        IOBuf() = default;

        /// total = block_reserve + prefix_reserve
        IOBuf(size_t block_reserve, size_t prefix_reserve = 0);

        IOBuf(const IOBuf&) = delete;
        IOBuf &operator=(const IOBuf&) = delete;

        IOBuf(IOBuf && rhs) noexcept = delete;
        IOBuf& operator=(IOBuf&& rhs) = delete;

        virtual ~IOBuf() {
            do_release();
        }

        /// default max block size
        turbo::Result<turbo::span<char> > borrow();

        void commit(size_t n);

        turbo::Result<std::vector<turbo::span<char> > > borrow(size_t byte_size, std::optional<int> combine);

        turbo::Result<size_t> shrink();

        turbo::Result<size_t> shrink_immutable();

        turbo::Result<size_t> write_able_size() const;

        turbo::Status prepend_to(IOBuf &lhs);

        turbo::Status prepend(IOBuf &&lhs);

        turbo::Status append_to(IOBuf &rhs);

        turbo::Status append(IOBuf &&lhs);

        turbo::Status append(std::string_view data);

        turbo::Status append(void *data, size_t size);

        std::string flatten() const;

        template<size_t SA = Alignment>
        AlignedString<SA> flatten_aligned() const;

        size_t size() const;

        size_t block_size() const;

        size_t prepend_blocks() const {
            return _view_start;
        }

        std::string_view block_view(size_t idx) const;

        size_t pop_front(size_t n, std::string *result = nullptr);

    protected:
        void do_release();

        size_t do_shrink();

        size_t do_shrink_immutable();

        void alloc_block(size_t n, bool combine);

        turbo::Result<std::vector<turbo::span<char> > > get_combine_write_able(size_t byte_size, int combine);

    protected:
        std::vector<BlockView> _views;
        size_t _total_size{0};
        size_t _index{0};
        size_t _view_start{0};
        bool _borrowing{false};
    };

    template<size_t Alignment, size_t BlockSize>
    Allocator<typename IOBuf<Alignment, BlockSize>::Block> IOBuf<Alignment, BlockSize>::Block::alloc;

    template<size_t Alignment, size_t BlockSize>
    typename IOBuf<Alignment, BlockSize>::allocator_type IOBuf<Alignment, BlockSize>::Block::data_alloc;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines

    /// total = block_reserve + prefix_reserve
    template<size_t Alignment, size_t BlockSize>
    IOBuf<Alignment, BlockSize>::IOBuf(size_t block_reserve, size_t prefix_reserve) {
        _views.reserve(prefix_reserve + block_reserve);
        for (size_t i = 0; i < prefix_reserve; ++i) {
            BlockView view;
            view.block = nullptr;
            view.status = BlockStatus::Umount;
            _views.push_back(view);
        }
        _view_start = prefix_reserve;
        _index = prefix_reserve;
    }

    template<size_t Alignment, size_t BlockSize>
    void IOBuf<Alignment, BlockSize>::alloc_block(size_t n, bool combine) {
        if (combine) {
            auto b = Block::acquire(n);
            b->share();
            BlockView view;
            view.block = b;
            _views.push_back(view);
            return;
        }

        for (size_t i = 0; i < n; i++) {
            auto b = Block::acquire(1);
            b->share();
            BlockView view;
            view.block = b;
            _views.push_back(view);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> IOBuf<Alignment, BlockSize>::write_able_size() const {
        if (_index == _views.size()) {
            return 0;
        }
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        size_t total_size{0};

        for (size_t i = _index; i < _views.size(); i++) {
            total_size += _views[i].block->capacity - _views[i].length - _views[i].offset;
        }
        return total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<turbo::span<char> > IOBuf<Alignment, BlockSize>::borrow() {
        TURBO_MOVE_OR_RAISE(auto wra, write_able_size());
        if (wra == 0) {
            alloc_block(1, false);
        }
        KCHECK(_views[_index].status == BlockStatus::Writeable) << " " << static_cast<int>(_views[_index].status);
        _views[_index].status = BlockStatus::Borrowing;
        _borrowing = true;
        return _views[_index].write_able();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<std::vector<turbo::span<char> > > IOBuf<Alignment, BlockSize>::borrow(
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
        _borrowing = true;
        return vec;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<std::vector<turbo::span<char> > > IOBuf<Alignment, BlockSize>::get_combine_write_able(
        size_t byte_size, int combine) {
        if (_borrowing) {
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
        _borrowing = true;
        return vec;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> IOBuf<Alignment, BlockSize>::shrink() {
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> IOBuf<Alignment, BlockSize>::shrink_immutable() {
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink_immutable();
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
                    if (v.length == 0) {
                        ret += v.block->capacity;
                        v.block->release();
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
                    if (v.length == 0) {
                        ret += v.block->capacity;
                        v.block->release();
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
    void IOBuf<Alignment, BlockSize>::commit(size_t n) {
        if (!_borrowing || _index == _views.size()) {
            return;
        }
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

                uint32_t remaining = v.block->capacity - v.length - v.offset;

                uint32_t consume = static_cast<uint32_t>(std::min((size_t) remaining, n));

                v.length += consume;
                _total_size += consume;
                n -= consume;

                if (v.length + v.offset == v.block->capacity) {
                    v.status = BlockStatus::Immutable;
                    _index++;
                } else {
                    /// not full
                    v.status = BlockStatus::Writeable;
                }
            }
        }

        _borrowing = false;
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
    void IOBuf<Alignment, BlockSize>::do_release() {
        /// 1. Iterate through all views and release their corresponding blocks.
        /// Each call to block->release() handles the ref_count and memory deallocation.
        for (auto &v: _views) {
            if (v.block) {
                v.block->release();
            }
        }

        /// 2. Clear the vector to prevent dangling pointers.
        _views.clear();

        /// 3. Reset logical metadata to initial state.
        _total_size = 0;
        _index = 0;
        _view_start = 0;
    }

    /// --- Append (Const Ref) ---
    /// @brief Share current committed assets to rhs and seal local write access.
    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::append_to(IOBuf &rhs) {
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (rhs._borrowing) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }
        /// 1. Safety Check: Transaction isolation.
        if (this == &rhs || _total_size == 0) return turbo::OkStatus();


        /// 2. Housekeeping: Reclaim local and remote trailing gaps.
        this->do_shrink_immutable();
        rhs.do_shrink_immutable();

        /// 3. Asset Transfer Loop.
        /// v.status == BlockStatus::Umount or v.status == BlockStatus::Immutable or BlockStatus::Reference
        for (size_t idx = _view_start; idx < _views.size(); idx++) {
            auto &v = _views[idx];
            if (v.status == BlockStatus::Umount) {
                continue;
            }

            /// Increase physical ref_count for the block.
            v.block->share();

            /// 4. Setup Receiver View: Mark as Reference (Read-only snapshot).
            BlockView shared_view = v;
            shared_view.status = BlockStatus::Reference;
            rhs._views.push_back(shared_view);
        }

        /// 6. Meta Synchronization.
        rhs._total_size += _total_size;

        /// 7. Index Alignment.
        /// Since all shared blocks are now Immutable, local _index must move to the end.
        /// Subsequent write operations will trigger a new block allocation.
        _index = _views.size();

        /// Receiver's index also points to its end of logical content.
        rhs._index = rhs._views.size();
        return turbo::OkStatus();
    }

    /// @brief Steal committed assets from lhs (Move semantics).
    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::append(IOBuf &&lhs) {
        /// 1. Transaction & Identity Check.
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._borrowing) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }
        if (this == &lhs || _total_size == 0) return turbo::OkStatus();

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
            if ((v.status == BlockStatus::Immutable || v.status == BlockStatus::Writeable) && v.length + v.offset < v.
                block->capacity) {
                v.status = BlockStatus::Writeable;
                _index = _views.size() - 1;
            }
        }

        /// 5. Clean up the carcass of lhs.
        /// do_release will now only free the empty/uncommitted blocks we left behind.
        lhs.do_release();
        return turbo::OkStatus();
    }


    /// @brief Share current assets to the front of lhs and seal local write access.
    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend_to(IOBuf &lhs) {
        /// 1. Transaction & Identity Check.
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._borrowing) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }

        if (this == &lhs || _total_size == 0) return turbo::OkStatus();


        /// 3. Close local gaps to ensure contiguous logical flow.
        this->do_shrink_immutable();

        /// 4. Prepare new view vector for lhs.


        auto n = _views.size() - _view_start;
        auto ln = lhs._views.size() - lhs._view_start;
        auto li = lhs._index - lhs._view_start;
        std::vector<BlockView> new_views;

        turbo::span<BlockView> views;
        turbo::span<BlockView> lhs_views;
        if (n < lhs._view_start) {
            views = turbo::span<BlockView>(lhs._views.data() + lhs._view_start - n, n);
        } else {
            new_views.resize(n + ln);
            views = turbo::span<BlockView>(new_views.data(), n);
            lhs_views = turbo::span<BlockView>(new_views.data() + n, ln);
        }

        for (size_t i = _view_start; i < _views.size(); i++) {
            auto &v = _views[i];
            v.block->share();

            /// 5. Setup View for Receiver (lhs): Mark as Reference.
            BlockView sv = v;
            sv.status = BlockStatus::Reference;
            views[i - _view_start] = sv;
        }

        /// 7. Combine with lhs's original views.
        for (size_t i = 0; i < lhs_views.size(); i++) {
            auto &v = lhs._views[i + lhs._view_start];
            lhs_views[i] = v;
        }

        if (n < lhs._view_start) {
            lhs._view_start = lhs._view_start - n;
            /// no need to move lhs _inde
        } else {
            lhs._views.swap(new_views);
            lhs._view_start = 0;
            lhs._index = n + li;
        }
        lhs._total_size += _total_size;
        return turbo::OkStatus();
    }


    /// --- Preappend (Move) ---
    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::prepend(IOBuf &&lhs) {
        /// 1. Transaction & Identity Check.
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._borrowing) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }

        if (this == &lhs || _total_size == 0) return turbo::OkStatus();

        /// 1. Settle lhs to reclaim empty trailing blocks.
        lhs.do_shrink_immutable();
        size_t lhs_view_count = lhs._views.size() - lhs._view_start;
        auto li = _index - _view_start;

        turbo::span<BlockView> views;
        turbo::span<BlockView> local_views;
        std::vector<BlockView> new_views;

        if (lhs_view_count < _view_start) {
            views = turbo::span<BlockView>(_views.data() + _view_start - lhs_view_count, lhs_view_count);
        } else {
            new_views.resize(lhs_view_count + _views.size() - _view_start);
            views = turbo::span<BlockView>(new_views.data(), lhs_view_count);
            local_views = turbo::span<BlockView>(new_views.data() + lhs_view_count, _views.size() - _view_start);
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

    /// @brief Append data from string_view.
    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::append(std::string_view data) {
        return append(const_cast<char *>(data.data()), data.size());
    }

    /// @brief Raw append: copy data into IOBuf.
    template<size_t Alignment, size_t BlockSize>
    turbo::Status IOBuf<Alignment, BlockSize>::append(void *data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();

        /// INVARIANT: No append allowed during an active borrowing session.
        DKCHECK(!_borrowing) << "logic error: appending during borrowing";

        size_t left = size;
        char *src = static_cast<char *>(data);

        while (left > 0) {
            /// 1. "Borrow" space from the current write index.
            /// If current block is full/Immutable/Reference, borrow() will handle allocation.
            TURBO_MOVE_OR_RAISE(auto span, borrow());

            size_t to_copy = std::min(span.size(), left);

            /// 2. Physical Copy.
            std::memcpy(span.data(), src, to_copy);

            /// 3. "Commit" the debt.
            commit(to_copy);

            src += to_copy;
            left -= to_copy;
        }
        return turbo::OkStatus();
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
            result.append(v.block->data + v.offset, v.length);
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
            result.append(v.block->data + v.offset, v.length);
        }
        return result;
    }

    /// @brief Total logical bytes in the IOBuf.
    template<size_t Alignment, size_t BlockSize>
    size_t IOBuf<Alignment, BlockSize>::size() const {
        return _total_size;
    }

    /// @brief Number of active views (not including borrowed/empty blocks).
    template<size_t Alignment, size_t BlockSize>
    size_t IOBuf<Alignment, BlockSize>::block_size() const {
        return _views.size() - _view_start;
    }

    /// @brief Access a specific block as a string_view for read-only inspection.
    template<size_t Alignment, size_t BlockSize>
    std::string_view IOBuf<Alignment, BlockSize>::block_view(size_t idx) const {
        auto ridx = idx + _view_start;
        if (ridx >= _views.size() || _views[ridx].length == 0) {
            return {};
        }

        const auto &v = _views[ridx];
        return {v.block->data + v.offset, v.length};
    }

    /// @brief Consume n bytes from the head of the IOBuf.
    /// @param n Number of bytes to consume.
    /// @param result Optional string to store the consumed data.
    /// @return Actual number of bytes consumed.
    template<size_t Alignment, size_t BlockSize>
    size_t IOBuf<Alignment, BlockSize>::pop_front(size_t n, std::string *result) {
        if (n == 0 || _total_size == 0) return 0;

        /// INVARIANT: Cannot modify the logical head during an active borrowing session.
        DKCHECK(!_borrowing) << "logic error: pop_front during borrowing";

        size_t to_pop = std::min(n, _total_size);
        size_t left = to_pop;

        if (result) {
            result->reserve(result->size() + to_pop);
        }

        /// Use a logical window (_view_start) to achieve O(1) consumption.
        while (left > 0 && _view_start < _views.size()) {
            auto &v = _views[_view_start];
            size_t consume = std::min((size_t) v.length, left);

            if (result) {
                result->append(v.block->data + v.offset, consume);
            }

            if (consume == v.length) {
                /// Case: Full View Umount.
                /// 1. Release the physical block's reference.
                v.block->release();
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
} // namespace fermat
