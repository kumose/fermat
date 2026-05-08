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
    template<typename C>
    class Appender {
    public:
        using container_type = C;

        virtual ~Appender() = default;

        virtual turbo::Status reserve(size_t n) = 0;

        virtual turbo::Status append(const void *data, size_t n) = 0;

        turbo::Status append(std::string_view data) {
            return append(data.data(), data.size());
        }
    };


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
        virtual ~IOBufBase() = default;

        bool share_able_to(const IOBufBase &rhs) {
            return _alignment == rhs._alignment && _block_size >= rhs._block_size;
        }

    protected:
        static Allocator<Block> alloc;

        Block *create_block(size_t nblock);

        void release_block(Block *block);

    protected:
        size_t _alignment{0};
        size_t _block_size{0};
    };
} // namespace fermat
