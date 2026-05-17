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
#include <memory>
#include <new>

namespace fermat {
    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class CordBuffer {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr bool is_iobuf = true;
        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;

    public:
        CordBuffer() = default;

        CordBuffer(const CordBuffer &) = delete;

        CordBuffer &operator=(const CordBuffer &) = delete;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        CordBuffer(CordBuffer &&rhs) noexcept
            : _views(std::move(rhs._views)),
              _total_size(std::exchange(rhs._total_size, 0)) {
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
            }
            return *this;
        }

        ~CordBuffer() = default;

        [[nodiscard]] size_t block_size() const;

        [[nodiscard]] size_t alignment() const;


        [[nodiscard]] size_t size() const;

        [[nodiscard]] size_t blocks() const;

        turbo::Status append(std::string_view data);

        turbo::Status append(const void *data, size_t size);

        void reserve_writeable(size_t n);

        void reserve(size_t n) {
            if (n <= _writeable + _total_size) {
                return;
            }
            reserve_writeable(n - _total_size);
        }

        BufferLease *borrow(size_t byte_size);

        void commit(BufferLease *l);

        void build();

        std::vector<Buffer<char, Alignment>> release();

        const std::vector<Buffer<char, Alignment>> &buffers() const;

    protected:
        std::list<Buffer<char, Alignment> > _current;
        ///< All block views (including Umount ones).
        std::list<Buffer<char, Alignment> > _views;
        ///< Total logical bytes stored.
        size_t _total_size{0};
        size_t _writeable{0};
        BufferLease _lease;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines
    ///
    ///////////////////////////////////////////////////////////////////////////
    ///


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
        return _views.size() + (_current.size() > 0 ? 1 : 0);
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
    void CordBuffer<Alignment, BlockSize>::reserve_writeable(size_t n) {
        while (n > _writeable) {
            Buffer<char, Alignment> new_buffer;
            new_buffer.reserve(BlockSize);
            _current.push_back(std::move(new_buffer));
            _writeable += BlockSize;
        }
    }

    template<size_t Alignment, size_t BlockSize>
    BufferLease *CordBuffer<Alignment, BlockSize>::borrow(size_t byte_size) {
        KCHECK(!_lease.borrowed());
        if (byte_size == 0) return nullptr;
        reserve_writeable(byte_size);
        std::vector<turbo::span<char> > vec;
        size_t remain = byte_size;
        for (auto &b: _current) {
            auto write_able_size = b.capacity() - b.size();
            if (remain > write_able_size) {
                vec.emplace_back(b.data() + b.size(), write_able_size);
                remain -= write_able_size;
            } else {
                vec.emplace_back(b.data() + b.size(), remain);
                remain = 0;
                break;
            }
        }
        _lease.set(vec);
        return &_lease;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBuffer<Alignment, BlockSize>::commit(BufferLease *l) {
        KCHECK(l == &_lease) << "Fatal: Lease does not belong to this IOBuf. "
                     << "Expected internal lease address " << &_lease
                     << ", got " << l << ". Possible double commit or foreign lease.";
        if (!_lease.borrowed()) {
            return;
        }
        auto n = _lease.size();
        while (n > 0) {
            DKCHECK(!_views.empty());
            auto &b = _current.front();
            auto sz = b.size();
            if (n >= b.capacity() - sz) {
                auto tmp = std::move(_current.front());
                _current.pop_front();
                tmp.resize(tmp.capacity());
                _views.push_back(std::move(tmp));
                n -= (b.capacity() - sz);
            } else {
                b.resize(sz + n);
                n = 0;
            }
        }
        _total_size += _lease.size();
        _writeable -= _lease.size();
        _lease.clear();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBuffer<Alignment, BlockSize>::build() {
        if (!_current.empty()) {
            auto &b = _current.front();
            if (b.size() == 0) return;
            auto tmp = std::move(_current.front());
            _current.pop_front();
            _views.push_back(std::move(tmp));
        }
        /// lease should commit it self, just make the last block to views;
    }

    template<size_t Alignment, size_t BlockSize>
    std::vector<Buffer<char, Alignment>> CordBuffer<Alignment, BlockSize>::release() {
        return std::move(_views);
    }

    template<size_t Alignment, size_t BlockSize>
    const std::vector<Buffer<char, Alignment>> &CordBuffer<Alignment, BlockSize>::buffers() const {
        return _views;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBuffer<Alignment, BlockSize>::append(const void *data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();
        reserve_writeable(size);
        const char *src = static_cast<const char *>(data);
        auto remnain = size;
        while (remnain > 0) {
            auto &b = _current.front();
            auto app_len = std::min(remnain, b.capacity() - b.size());
            b.append(src, app_len);
            remnain -= app_len;
            src += app_len;
            if (b.size() == b.capacity()) {
                auto tmp = std::move(_current.front());
                _current.pop_front();
                _views.push_back(std::move(tmp));
            }
        }
        _total_size += size;
        _writeable -= size;
        return turbo::OkStatus();
    }
} // namespace fermat
