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
    class IOBufBase {
    public:
        struct Block {
            Block(char *d, size_t cap) : data(d), capacity(cap) {
            }

            /// @brief Constructor only handles internal state
            Block() = default;

            bool is_shared() const noexcept {
                return ref_count.load(std::memory_order_acquire) > 1;
            }

            /// @brief Increment reference for sharing
            void share() {
                ref_count.fetch_add(1, std::memory_order_relaxed);
            }

            std::atomic<uint32_t> ref_count{0};
            uint32_t capacity{0};
            char *data{nullptr};
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
                KLOG(INFO) << block->capacity;
                return turbo::span<char>(block->data + length + offset, block->capacity - length - offset);
            }
        };

    public:
        virtual ~IOBufBase() = default;

        [[nodiscard]] bool share_able_to(const IOBufBase &rhs) {
            return _alignment == rhs._alignment && _block_size >= rhs._block_size;
        }

        [[nodiscard]] size_t block_size() {
            return _block_size;
        }

        [[nodiscard]] size_t alignment() {
            return _alignment;
        }

        turbo::Result<size_t> write_able_size() const;

        turbo::Result<turbo::span<char> > borrow();

        turbo::Result<std::vector<turbo::span<char> > > borrow(size_t byte_size, std::optional<int> combine);

        void commit(size_t n);


        turbo::Result<size_t> shrink();

        turbo::Result<size_t> shrink_immutable();

        turbo::Status append(std::string_view data);

        turbo::Status append(void *data, size_t size);


        std::string_view block_view(size_t idx) const;

        size_t prepend_blocks() const {
            return _view_start;
        }

        /// @brief Total logical bytes in the IOBuf.
        [[nodiscard]] size_t size() const {
            return _total_size;
        }


        [[nodiscard]] size_t blocks() const {
            return _views.size() - _view_start;
        }

        turbo::Status prepend_to(IOBufBase &lhs);

        turbo::Status prepend(IOBufBase &&lhs);

        turbo::Status append(IOBufBase &&lhs);

        turbo::Status append_to(IOBufBase &rhs);

        const BlockView *peek(size_t idx) const;

        turbo::Status custom(size_t n);

    protected:
        static Allocator<Block> alloc;

        Block *create_block(size_t nblock);

        void release_block(Block *block);

        void alloc_block(size_t n, bool combine);

        size_t do_shrink_immutable();

        size_t do_shrink();

        void do_release();

        turbo::Result<std::vector<turbo::span<char> > > get_combine_write_able(size_t byte_size, int combine);

        turbo::Status prepend_share_to(IOBufBase &lhs);

        turbo::Status prepend_copy_to(IOBufBase &lhs);

        turbo::Status append_share_to(IOBufBase &rhs);

        turbo::Status append_copy_to(IOBufBase &rhs);

    protected:
        size_t _alignment{0};
        size_t _block_size{0};

    protected:
        std::vector<BlockView> _views;
        size_t _total_size{0};
        size_t _index{0};
        size_t _view_start{0};
        bool _borrowing{false};
    };

    ///////////////////////////////////////////////////////////////////////////
    ///

    inline turbo::Result<size_t> IOBufBase::shrink() {
        if (_borrowing) {
            return turbo::unavailable_error("resource locked: block is currently under borrowing transaction");
        }
        return do_shrink();
    }

    inline turbo::Result<size_t> IOBufBase::shrink_immutable() {
        if (_borrowing) {
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
        auto ridx = idx + _view_start;
        if (ridx >= _views.size() || _views[ridx].length == 0) {
            return nullptr;
        }

        const auto &v = _views[ridx];
        return &v;
    }

} // namespace fermat
