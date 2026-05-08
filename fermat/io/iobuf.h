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

    public:
        IOBuf() {
            _alignment = Alignment;
            _block_size = BlockSize;
        }

        /// total = block_reserve + prefix_reserve
        IOBuf(size_t block_reserve, size_t prefix_reserve = 0);

        IOBuf(const IOBuf &) = delete;

        IOBuf &operator=(const IOBuf &) = delete;

        IOBuf(IOBuf &&rhs) noexcept = delete;

        IOBuf &operator=(IOBuf &&rhs) = delete;

        ~IOBuf() override {
            do_release();
        }

        using IOBufBase::block_size;

        using IOBufBase::alignment;

        using IOBufBase::write_able_size;

        using IOBufBase::borrow;

        using IOBufBase::commit;

        using IOBufBase::shrink;

        using IOBufBase::shrink_immutable;

        using IOBufBase::append;

        using IOBufBase::block_view;

        using IOBufBase::prepend_blocks;

        using IOBufBase::size;

        using IOBufBase::blocks;

        std::string flatten() const;

        template<size_t SA = Alignment>
        AlignedString<SA> flatten_aligned() const;

        size_t pop_front(size_t n, std::string *result = nullptr);

    protected:
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines

    /// total = block_reserve + prefix_reserve
    template<size_t Alignment, size_t BlockSize>
    IOBuf<Alignment, BlockSize>::IOBuf(size_t block_reserve, size_t prefix_reserve) {
        _alignment = Alignment;
        _block_size = BlockSize;
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

} // namespace fermat
