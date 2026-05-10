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

#include <fermat/io/iobuf_base.h>
#include <turbo/log/logging.h>

namespace fermat {
    struct Position {
        constexpr Position(size_t idx, size_t off) : index(idx), offset(off) {
        }

        constexpr bool operator==(const Position &rhs) const {
            return index == rhs.index && offset == rhs.offset;
        }

        constexpr bool operator!=(const Position &rhs) const {
            return index != rhs.index || offset != rhs.offset;
        }


        constexpr bool operator>(const Position &rhs) const {
            if (index > rhs.index) return true;
            if (index < rhs.index) return false;
            return offset > rhs.offset;
        }

        constexpr bool operator>=(const Position &rhs) const {
            if (index > rhs.index) return true;
            if (index < rhs.index) return false;
            return offset >= rhs.offset;
        }

        constexpr bool operator<(const Position &rhs) const {
            return rhs >= *this;
        }

        constexpr bool operator<=(const Position &rhs) const {
            return !(*this > rhs);
        }

        size_t index;
        size_t offset;
    };

    class Peeker {
    public:
        static constexpr size_t kNPos = std::numeric_limits<size_t>::max();
        static constexpr Position npos = Position(kNPos, kNPos);

    public:
        Peeker() {
            init_buffer();
        }

        Peeker(const IOBufBase *buf) : _buffer(buf) {
            init_buffer();
        }

        Peeker &set_buffer(const IOBufBase *buf) {
            _buffer = buf;
            init_buffer();
            return *this;
        }

        Peeker &reset() {
            init_buffer();
            return *this;
        }

        Position end() const {
            return npos;
        }

        Position tellg() const;

        /// @brief Seek to an absolute logical position.
        /// @param pos The logical byte offset from the start.
        /// @return The new physical position.
        Position seek_to(size_t pos);

        Peeker &seek_to(Position pos);

        Peeker &seek_end(size_t n = std::numeric_limits<size_t>::max());

        /// @brief Rewind the cursor position by n bytes.
        /// @param n Number of bytes to move backward.
        /// @return Reference to this peeker.
        Peeker &seek_start(size_t n = std::numeric_limits<size_t>::max());

        template<typename T>
        Position find_first_position(T chars);
        template<typename T>
        size_t find_first_offset(T chars);

        char operator*() const;

        Peeker &operator++();

        Peeker operator++(int);

        Peeker &operator--();

        Peeker operator--(int);

        Peeker operator+(size_t n);

        Peeker &operator+=(size_t n);

        Peeker operator-(size_t n);

        Peeker &operator-=(size_t n);

        /// @brief Get the remaining data in the current block as a string_view.
        /// @return The string_view of the current segment.
        operator std::string_view();

        /// @brief Reads at most @p n bytes from the current position and returns them
        ///        as a contiguous string view (zero‑copy, no allocation). The view
        ///        points directly into the internal buffer of the current block.
        ///
        /// The method never crosses block boundaries. If the current block contains
        /// fewer than @p n readable bytes, the returned view includes all of the
        /// remaining bytes in that block (so the actual length may be smaller than
        /// @p n). After the call, the internal cursor advances by the number of bytes
        /// returned.
        ///
        /// The caller is expected to call `readn` repeatedly until it returns
        /// `std::nullopt` (end of data) or until the desired amount of data has been
        /// collected. Higher‑level functions (such as `FlattenCustomer`) may use this
        /// primitive to assemble a logical chunk across block boundaries.
        ///
        /// @param n Maximum number of bytes to read.
        /// @return An optional `std::string_view` containing the next contiguous
        ///         segment of the data (length ≤ n). `std::nullopt` is returned when
        ///         there is no more data (i.e., the end of the IOBuf has been reached).
        ///
        /// @par Example: Read exactly 4096 bytes (or less at EOF) into a user buffer.
        /// @code
        /// // Suppose `peeker` already points to the start of data.
        /// std::vector<char> buffer;
        /// buffer.reserve(4096);
        ///
        /// while (buffer.size() < 4096) {
        ///     auto chunk = peeker.readn(4096 - buffer.size());
        ///     if (!chunk) break;               // EOF, no more data
        ///     buffer.insert(buffer.end(), chunk->begin(), chunk->end());
        /// }
        /// // Now `buffer` contains up to 4096 bytes.
        /// @endcode
        ///
        /// @par Example: Process data in chunks without copying.
        /// @code
        /// // Process all data in chunks of at most 1024 bytes.
        /// while (auto chunk = peeker.readn(1024)) {
        ///     // Directly use the chunk (zero‑copy)
        ///     process_data_without_copy(*chunk);
        /// }
        /// @endcode
        ///
        /// @par Example: Read a large header that may span block boundaries.
        /// @code
        /// // Read a 4‑byte integer that could be split across two blocks.
        /// uint32_t value = 0;
        /// size_t bytes_read = 0;
        /// while (bytes_read < sizeof(value)) {
        ///     auto chunk = peeker.readn(sizeof(value) - bytes_read);
        ///     if (!chunk) return -1;          // unexpected EOF
        ///     std::memcpy(reinterpret_cast<char*>(&value) + bytes_read,
        ///                 chunk->data(), chunk->size());
        ///     bytes_read += chunk->size();
        /// }
        /// @endcode
        std::optional<std::string_view> readn(size_t n);

        bool eof() const;
        operator bool() const;

        size_t has_read() const {
            return _has_read;
        }
    private:
        void init_buffer();

        void set_to_npos();

    private:
        const IOBufBase *_buffer{nullptr};
        size_t _has_read{0};
        Position _positions{kNPos, kNPos};
        const IOBufBase::BlockView *_view{nullptr};
        std::string_view _current;
    };

    template<typename T>
    Position Peeker::find_first_position(T chars) {
        if (_buffer == nullptr || _positions == npos || chars.empty()) return npos;
        size_t idx = _positions.index;
        size_t off = _positions.offset;
        while (idx < _buffer->readable_blocks()) {
            const auto *view = _buffer->readable_peek(idx);
            if (view == nullptr) break;
            std::string_view seg(view->block->data + view->offset, view->length);
            size_t pos = seg.find_first_of(chars, off);
            if (pos != std::string_view::npos) {
                return Position(idx, pos);
            }
            ++idx;
            off = 0;
        }
        return npos;
    }

    template<typename T>
    size_t Peeker::find_first_offset(T c) {
        if (_buffer == nullptr || _positions == npos) return kNPos;
        size_t idx = _positions.index;
        size_t off = _positions.offset;
        size_t offset = 0;
        while (idx < _buffer->readable_blocks()) {
            const auto *view = _buffer->readable_peek(idx);
            std::string_view seg(view->block->data + view->offset, view->length);
            size_t pos = seg.find_first_of(c, off);
            if (pos != std::string_view::npos) {
                return offset+pos;
            }
            ++idx;
            offset += view->length;
        }
        return kNPos;
    }

} // namespace fermat
