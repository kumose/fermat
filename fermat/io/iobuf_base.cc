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

#include <fermat/io/iobuf_base.h>

namespace fermat {
    void Lease::set(std::vector<turbo::span<char> > sp) {
        _spans = std::move(sp);
        for (const auto &s: _spans) {
            _capacity += s.size();
        }
    }

    void Lease::set(turbo::span<char> sp) {
        _capacity = sp.size();
        _spans.push_back(sp);
    }

    /// @brief Sequential write that automatically handles crossing span boundaries.
    turbo::Status Lease::write(const char *data, size_t len) {
        if (len == 0) return turbo::OkStatus();
        if (_total_size + len > _capacity) {
            return turbo::out_of_range_error("Lease capacity exceeded, size:", _total_size, " len:", len, " capacity:", _capacity);
        }

        size_t written = 0;
        while (written < len && _index < _spans.size()) {
            auto &current = _spans[_index];
            size_t available = current.size() - _offset;
            size_t can_copy = std::min(available, len - written);

            if (can_copy > 0) {
                std::memcpy(current.data() + _offset, data + written, can_copy);
                _offset += can_copy;
                _total_size += can_copy;
                written += can_copy;
            }

            if (_offset == current.size()) {
                _index++;
                _offset = 0;
            }
        }
        return turbo::OkStatus();
    }

    /// @brief Rewind the internal write cursor and total size.
    void Lease::pop_back(size_t n) {
        if (n == 0 || _total_size == 0) return;

        size_t to_remove = std::min(n, _total_size);
        _total_size -= to_remove;

        while (to_remove > 0) {
            if (to_remove <= _offset) {
                _offset -= to_remove;
                break;
            }
            to_remove -= _offset;
            _index--;
            _offset = _spans[_index].size();
        }
    }

    /// @brief Reset all internal write markers.
    void Lease::clear() {
        _total_size = 0;
        _index = 0;
        _offset = 0;
    }

    void Lease::clear_lease() {
        clear();
        _capacity = 0;
        _spans.clear();
    }

    void Lease::visit_remaining(const VisitorCallback &visitor) const {
        if (_index >= _spans.size()) return;

        size_t current_idx = _index;
        size_t current_off = _offset;

        while (current_idx < _spans.size()) {
            const auto &span = _spans[current_idx];
            // Amount of writable space in this span starting from current offset
            size_t available = span.size() - current_off;
            if (available > 0) {
                // Provide the raw pointer to the caller
                char *ptr = const_cast<char *>(span.data() + current_off);
                // If visitor returns false, stop iterating further spans
                if (!visitor(ptr, available)) break;
            }
            // Move to next span, reset offset to 0
            ++current_idx;
            current_off = 0;
        }
    }

    void Lease::advance(size_t n) {
        if (n == 0 || _total_size + n > _capacity) return;

        size_t to_move = n;
        while (to_move > 0 && _index < _spans.size()) {
            size_t available = _spans[_index].size() - _offset;
            size_t can_advance = std::min(available, to_move);

            _offset += can_advance;
            _total_size += can_advance;
            to_move -= can_advance;

            if (_offset == _spans[_index].size()) {
                _index++;
                _offset = 0;
            }
        }
    }

    Allocator<IOBufBase::Block> IOBufBase::alloc;

    void IOBufBase::do_release() {
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

    IOBufBase::Block *IOBufBase::create_block(size_t nblock) {
        auto *b = alloc.allocate(1);
        auto n = nblock * _block_size;
        auto ptr = static_cast<char *>(mi_aligned_alloc(_alignment, n));
        Allocator<Block>::construct(b, ptr, n);
        return b;
    }

    void IOBufBase::release_block(Block *block) {
        // Use acq_rel to ensure visibility across threads
        if (block->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // 1. First, call the destructor to free 'data'
            if (block->data) {
                mi_free_size_aligned(block->data, block->capacity, _alignment);
                block->data = nullptr;
            }
            // 2. Then, return the memory of the Header (this) to the pool
            alloc.deallocate(block, 1);
        }
    }

    void IOBufBase::alloc_block(size_t n, bool combine) {
        if (combine) {
            auto b = create_block(n);
            b->share();
            BlockView view;
            view.block = b;
            _views.push_back(view);
            return;
        }

        for (size_t i = 0; i < n; i++) {
            auto b = create_block(1);
            b->share();
            BlockView view;
            view.block = b;
            _views.push_back(view);
        }
    }

    turbo::Result<size_t> IOBufBase::write_able_size() const {
        if (_index == _views.size()) {
            return 0;
        }
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        size_t total_size{0};

        for (size_t i = _index; i < _views.size(); i++) {
            total_size += _views[i].block->capacity - _views[i].length - _views[i].offset;
        }
        return total_size;
    }

    turbo::Result<Lease *> IOBufBase::borrow() {
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

    turbo::Result<Lease *> IOBufBase::borrow(
        size_t byte_size, std::optional<int> combine) {
        /// *combine > BlockSize go to big block mode
        if (combine.has_value() && *combine > _block_size) {
            return get_combine_write_able(byte_size, *combine);
        }

        TURBO_MOVE_OR_RAISE(auto wra, write_able_size());
        while (wra < byte_size) {
            alloc_block(1, false);
            wra += _block_size;
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


    turbo::Result<Lease *> IOBufBase::get_combine_write_able(
        size_t byte_size, int combine) {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        size_t n = (combine + _block_size - 1) / _block_size;

        /// no holds
        do_shrink_immutable();
        size_t wra{0};
        while (wra < byte_size) {
            alloc_block(n, true);
            wra += _block_size * n;
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

    size_t IOBufBase::do_shrink_immutable() {
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


    size_t IOBufBase::do_shrink() {
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

    void IOBufBase::commit(Lease *l) {
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

    /// @brief Append data from string_view.
    turbo::Status IOBufBase::append(std::string_view data) {
        return append(const_cast<char *>(data.data()), data.size());
    }

    /// @brief Raw append: copy data into IOBuf.
    turbo::Status IOBufBase::append(void *data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();

        /// INVARIANT: No append allowed during an active borrowing session.
        DKCHECK(!_lease.borrowed()) << "logic error: appending during borrowing";

        char *src = static_cast<char *>(data);


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

    turbo::Status IOBufBase::prepend_to(IOBufBase &lhs) {
        if (share_able_to(lhs)) {
            return prepend_share_to(lhs);
        } else {
            return prepend_copy_to(lhs);
        }
    }

    /// @brief Share current assets to the front of lhs and seal local write access.
    turbo::Status IOBufBase::prepend_share_to(IOBufBase &lhs) {
        /// 1. Transaction & Identity Check.
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._lease.borrowed()) {
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


    turbo::Status IOBufBase::prepend_copy_to(IOBufBase &lhs) {
        // 1. Transaction checks
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._lease.borrowed()) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }
        if (this == &lhs || _total_size == 0) {
            return turbo::OkStatus();
        }

        // 2. Determine how many standard blocks we need to copy the data
        size_t data_size = _total_size;
        size_t block_sz = lhs._block_size;
        size_t num_blocks = (data_size + block_sz - 1) / block_sz;

        // 3. Build a temporary vector of new BlockView objects (each referring to a newly allocated block)
        std::vector<BlockView> new_blocks;
        new_blocks.reserve(num_blocks);

        // Helper to iterate source IOBuf data without flattening
        size_t remaining = data_size;
        size_t src_idx = _view_start;
        size_t src_offset = 0; // offset inside current source block

        while (remaining > 0) {
            // Allocate a new block from lhs (size = block_sz)
            Block *new_block = lhs.create_block(1);
            new_block->share();
            char *dest = new_block->data;
            size_t copy_size = std::min(remaining, block_sz);
            size_t copied = 0;
            while (copied < copy_size) {
                const auto &v = _views[src_idx];
                size_t avail = v.length - src_offset;
                size_t to_copy = std::min(copy_size - copied, avail);
                std::memcpy(dest + copied, v.block->data + v.offset + src_offset, to_copy);
                copied += to_copy;
                src_offset += to_copy;
                if (src_offset >= v.length) {
                    ++src_idx;
                    src_offset = 0;
                }
            }
            // Create BlockView for this new block
            BlockView view;
            view.block = new_block;
            view.offset = 0;
            view.length = static_cast<uint32_t>(copy_size);
            view.status = BlockStatus::Immutable; // copied data is read-only
            new_blocks.push_back(view);
            remaining -= copy_size;
        }

        // 4. Prepend these new blocks to lhs (similar logic as prepend_share_to, but with n = new_blocks.size())
        size_t n = new_blocks.size();
        size_t ln = lhs._views.size() - lhs._view_start;
        size_t li = lhs._index - lhs._view_start;
        std::vector<BlockView> final_views;

        if (n <= lhs._view_start) {
            // Reuse head Umount slots: overwrite them directly
            for (size_t i = 0; i < n; ++i) {
                lhs._views[lhs._view_start - n + i] = new_blocks[i];
            }
            lhs._view_start -= n;
            // lhs._index remains unchanged (absolute index still valid)
        } else {
            // Need to construct new view vector
            final_views.reserve(n + ln);
            // Insert new blocks
            final_views.insert(final_views.end(), new_blocks.begin(), new_blocks.end());
            // Append existing effective views from lhs
            for (size_t i = lhs._view_start; i < lhs._views.size(); ++i) {
                final_views.push_back(lhs._views[i]);
            }
            lhs._views.swap(final_views);
            lhs._view_start = 0;
            lhs._index = n + li;
        }

        lhs._total_size += data_size;

        // 5. Current object unchanged
        return turbo::OkStatus();
    }

    /// --- Preappend (Move) ---
    turbo::Status IOBufBase::prepend(IOBufBase &&lhs) {
        /// 1. Transaction & Identity Check.
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._lease.borrowed()) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }
        if (!lhs.share_able_to(*this)) {
            return turbo::invalid_argument_error("alignment size mismatch, cannot share blocks[this:", _alignment,
                                                 " vs lhs:", lhs._alignment, "]");
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


    /// @brief Steal committed assets from lhs (Move semantics).
    turbo::Status IOBufBase::append(IOBufBase &&lhs) {
        /// 1. Transaction & Identity Check.
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (lhs._lease.borrowed()) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }

        if (!lhs.share_able_to(*this)) {
            return turbo::invalid_argument_error("alignment size mismatch, cannot share blocks[this:", _alignment,
                                                 " vs lhs:", lhs._alignment, "]");
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


    turbo::Status IOBufBase::append_to(IOBufBase &rhs) {
        if (share_able_to(rhs)) {
            return append_share_to(rhs);
        } else {
            return append_copy_to(rhs);
        }
    }

    /// --- Append (Const Ref) ---
    /// @brief Share current committed assets to rhs and seal local write access.
    turbo::Status IOBufBase::append_share_to(IOBufBase &rhs) {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (rhs._lease.borrowed()) {
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

    turbo::Status IOBufBase::append_copy_to(IOBufBase &rhs) {
        if (_lease.borrowed()) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        if (rhs._lease.borrowed()) {
            return turbo::unavailable_error("target locked: block is currently under borrowing transaction");
        }
        if (this == &rhs || _total_size == 0) {
            return turbo::OkStatus();
        }

        // Iterate over all committed blocks of this IOBuf
        for (size_t i = _view_start; i < _views.size(); ++i) {
            const auto &v = _views[i];
            if (v.length == 0) break; // no more data
            // Append the block's data to rhs (copy)
            auto status = rhs.append(v.block->data + v.offset, v.length);
            if (!status.ok()) return status;
        }
        return turbo::OkStatus();
    }


    turbo::Status IOBufBase::custom(size_t n) {
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
} // namespace fermat
