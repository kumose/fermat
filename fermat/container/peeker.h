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

#include <fermat/container/cord_buffer.h>
#include <fermat/container/string.h>
#include <fermat/container/vector.h>
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

    inline std::ostream &operator<<(std::ostream &os, const Position &pos) {
        os << "index:" << pos.index << ", offset:" << pos.offset;
        return os;
    }

    template<typename Source, typename Enabler = void>
    class Peeker;


    template<typename Source>
    class Peeker<Source, std::enable_if_t<is_cord_buffer_v<Source> > > {
    public:
        static constexpr size_t kNPos = std::numeric_limits<size_t>::max();
        static constexpr Position npos = Position(kNPos, kNPos);

        static constexpr size_t kAlignment = is_cord_buffer<Source>::alignment;
        static constexpr size_t kBlockSize = is_cord_buffer<Source>::block_size;

    public:
        Peeker() {
            init_buffer();
        }

        Peeker(const Source *buf) : _buffer(buf) {
            init_buffer();
        }


        Peeker &set_buffer(const Source *buf) {
            _buffer = buf;
            init_buffer();
            return *this;
        }

        Peeker &reset() {
            init_buffer();
            return *this;
        }

        [[nodiscard]] Position end() const {
            return npos;
        }

        [[nodiscard]] Position tellg() const {
            return _positions;
        }

        /// @brief Seek to an absolute logical position.
        /// @param pos The logical byte offset from the start.
        /// @return The new physical position.
        Position seek_to(size_t pos) {
            if (pos > _has_read) {
                seek_end(pos - _has_read);
            } else if (pos < _has_read) {
                seek_start(_has_read - pos);
            }
            return _positions;
        }

        Peeker &seek_to(Position pos) {
            if (_buffer == nullptr || pos == npos) {
                set_to_npos();
                return *this;
            }
            const size_t total_readable = _buffer->readable_blocks();
            if (pos.index >= total_readable) {
                set_to_npos();
                return *this;
            }
            const auto *view = _buffer->readable_peek(pos.index);
            if (pos.offset > view->length) {
                set_to_npos();
                return *this;
            }

            size_t consumed = 0;
            for (size_t i = 0; i < pos.index; ++i) {
                const auto *v = _buffer->readable_peek(i);
                if (v) consumed += v->size();
            }
            consumed += pos.offset;
            _positions = pos;
            _view = view;
            _current = std::string_view(view->data(), view->size());
            _has_read = consumed;
            return *this;
        }

        Peeker &seek_end(size_t n = std::numeric_limits<size_t>::max()) {
            if (n == 0 || _positions == npos) return *this;
            if (n >= (_buffer->size() - _has_read)) {
                set_to_npos();
                return *this;
            }
            size_t remaining = n;
            while (remaining > 0) {
                size_t available = _current.size() - _positions.offset;
                if (remaining < available) {
                    _positions.offset += remaining;
                    _has_read += remaining;
                    break;
                }
                remaining -= available;
                _has_read += available;
                ++_positions.index;
                _positions.offset = 0;
                _view = _buffer->readable_peek(_positions.index);
                _current = std::string_view(_view->data(), _view->size());
            }
            return *this;
        }

        /// @brief Rewind the cursor position by n bytes.
        /// @param n Number of bytes to move backward.
        /// @return Reference to this peeker.
        Peeker &seek_start(size_t n = std::numeric_limits<size_t>::max()) {
            if (n == 0) return *this;
            if (n >= _has_read) {
                init_buffer();
                return *this;
            }
            size_t remaining = n;
            while (remaining > 0) {
                if (remaining <= _positions.offset) {
                    _positions.offset -= remaining;
                    _has_read -= remaining;
                    break;
                }
                remaining -= _positions.offset;
                _has_read -= _positions.offset;
                --_positions.index;
                _view = _buffer->readable_peek(_positions.index);
                _current = std::string_view(_view->data(), _view->size());
                _positions.offset = _current.size();
            }
            return *this;
        }

        Position find_first_position(std::string_view chars);

        Position find_first_position(char c);

        size_t find_first_offset(std::string_view chars);

        size_t find_first_offset(char chars);

        char operator*() const {
            DKCHECK(_view != nullptr);
            DKCHECK(!_current.empty() && _current.size() > _positions.offset);
            return _current[_positions.offset];
        }

        Peeker &operator++() {
            return seek_end(1);
        }

        Peeker operator++(int) {
            Peeker ret = *this;
            ret.seek_end(1);
            return ret;
        }

        Peeker &operator--() {
            return seek_start(1);
        }

        Peeker operator--(int) {
            Peeker ret = *this;
            seek_start(1);
            return ret;
        }

        Peeker operator+(size_t n) {
            Peeker ret = *this;
            ret.seek_end(n);
            return ret;
        }

        Peeker &operator+=(size_t n) {
            return seek_end(n);
        }

        Peeker operator-(size_t n) {
            Peeker ret = *this;
            ret.seek_start(n);
            return ret;
        }

        Peeker &operator-=(size_t n) {
            return seek_start(n);
        }

        /// @brief Get the remaining data in the current block as a string_view.
        /// @return The string_view of the current segment.
        operator std::string_view() {
            if (_positions == npos) {
                return {};
            }
            return _current.substr(_positions.offset);
        }

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
        std::optional<std::string_view> readn(size_t n) {
            if (_positions == npos) {
                return std::nullopt;
            }
            size_t available = _current.size() - _positions.offset;
            size_t take = std::min(n, available);
            std::string_view result(_current.data() + _positions.offset, take);
            seek_end(take);
            return result;
        }

        [[nodiscard]] bool eof() const {
            return _positions == npos;
        }

        operator bool() const {
            return _positions != npos;
        }

        [[nodiscard]] size_t has_read() const {
            return _has_read;
        }

    private:
        void init_buffer();

        void set_to_npos() {
            _positions = npos;
            _view = nullptr;
            _current = {};
            _has_read = 0;
            if (_buffer) {
                _has_read = _buffer->size();
            }
        }

    private:
        const Source *_buffer{nullptr};
        size_t _has_read{0};
        Position _positions{kNPos, kNPos};
        const typename Source::block_view_type *_view{nullptr};
        std::string_view _current;
    };

    template<typename Source>
    Position Peeker<Source, std::enable_if_t<is_cord_buffer_v<Source> > >::find_first_position(std::string_view chars) {
        if (_buffer == nullptr || _positions == npos || chars.empty()) return npos;
        size_t idx = _positions.index;
        size_t off = _positions.offset;
        while (idx < _buffer->readable_blocks()) {
            const auto *view = _buffer->readable_peek(idx);
            if (view == nullptr) break;
            std::string_view seg(view->data(), view->size());
            size_t pos = seg.find_first_of(chars, off);
            if (pos != std::string_view::npos) {
                return Position(idx, pos);
            }
            ++idx;
            off = 0;
        }
        return npos;
    }

    template<typename Source>
    Position Peeker<Source, std::enable_if_t<is_cord_buffer_v<Source> > >::find_first_position(char chars) {
        if (_buffer == nullptr || _positions == npos) return npos;
        size_t idx = _positions.index;
        size_t off = _positions.offset;
        while (idx < _buffer->readable_blocks()) {
            const auto *view = _buffer->readable_peek(idx);
            if (view == nullptr) break;
            std::string_view seg(view->data(), view->size());
            size_t pos = seg.find_first_of(chars, off);
            if (pos != std::string_view::npos) {
                return Position(idx, pos);
            }
            ++idx;
            off = 0;
        }
        return npos;
    }

    template<typename Source>
    size_t Peeker<Source, std::enable_if_t<is_cord_buffer_v<Source> > >::find_first_offset(std::string_view chars) {
        if (_buffer == nullptr || _positions == npos || chars.empty()) return kNPos;
        size_t idx = _positions.index;
        size_t off = _positions.offset;
        size_t offset = 0;
        while (idx < _buffer->readable_blocks()) {
            const auto *view = _buffer->readable_peek(idx);
            std::string_view seg(view->data(), view->size());
            size_t pos = seg.find_first_of(chars, off);
            if (pos != std::string_view::npos) {
                return offset + pos - off + _has_read;
            }
            ++idx;
            offset += (seg.size() - off);
        }
        return kNPos;
    }

    template<typename Source>
    size_t Peeker<Source, std::enable_if_t<is_cord_buffer_v<Source> > >::find_first_offset(char chars) {
        if (_buffer == nullptr || _positions == npos) return kNPos;
        size_t idx = _positions.index;
        size_t off = _positions.offset;
        size_t offset = 0;
        while (idx < _buffer->readable_blocks()) {
            const auto *view = _buffer->readable_peek(idx);
            std::string_view seg(view->data(), view->size());
            size_t pos = seg.find_first_of(chars, off);
            if (pos != std::string_view::npos) {
                return offset + pos - off + _has_read;
            }
            ++idx;
            offset += (seg.size() - off);
        }
        return kNPos;
    }

    template<typename Source>
    void Peeker<Source, std::enable_if_t<is_cord_buffer_v<Source> > >::init_buffer() {
        _has_read = 0;
        _positions = {kNPos, kNPos};
        _view = nullptr;
        _current = {};
        if (!_buffer) return;
        if (_buffer->readable_blocks() > 0) {
            _positions.index = 0;
            _positions.offset = 0;
            _view = _buffer->readable_peek(0);
            _current = std::string_view(_view->data(), _view->size());
        }
    }

    template<typename T>
    inline constexpr bool is_contiguous_string_like_v = is_contiguous_string_visitor<T>::value;

    /// @brief Peeker specialization for contiguous string types (std::string, std::string_view).
    template<typename Source>
    class Peeker<Source, std::enable_if_t<is_contiguous_string_like_v<Source> > > {
    public:
        static constexpr size_t kNPos = std::numeric_limits<size_t>::max();
        static constexpr Position npos = Position(kNPos, kNPos);
        static constexpr size_t kAlignment = 0;
        static constexpr size_t kBlockSize = 0;

        Peeker() = default;

        Peeker(const Source &data) : _buffer(data) { init_buffer(); }

        Peeker &set_buffer(const Source &data) {
            _buffer = data;
            init_buffer();
            return *this;
        }

        Peeker &reset() {
            init_buffer();
            return *this;
        }

        Position end() const { return npos; }
        Position tellg() const { return _pos; }

        Position seek_to(size_t byte_offset) {
            if (byte_offset > _buffer.size()) {
                set_to_npos();
                return _pos;
            }
            _pos = Position(0, byte_offset);
            _has_read = byte_offset;
            return _pos;
        }

        Peeker &seek_to(Position pos) {
            if (_buffer.empty() || pos == npos) {
                set_to_npos();
                return *this;
            }
            if (pos.index != 0 || pos.offset > _buffer.size()) {
                set_to_npos();
                return *this;
            }
            _pos = pos;
            _has_read = pos.offset;
            return *this;
        }

        Peeker &seek_end(size_t n = std::numeric_limits<size_t>::max()) {
            if (n == 0 || _pos == npos) return *this;
            size_t remaining = _buffer.size() - _has_read;
            if (n >= remaining) {
                set_to_npos();
                return *this;
            }
            _pos.offset += n;
            _has_read += n;
            return *this;
        }

        Peeker &seek_start(size_t n = std::numeric_limits<size_t>::max()) {
            if (n == 0) return *this;
            if (n >= _has_read) {
                init_buffer();
                return *this;
            }
            _pos.offset -= n;
            _has_read -= n;
            return *this;
        }

        [[nodiscard]] Position find_first_position(std::string_view chars) const {
            if (_pos == npos) return npos;
            auto sv = std::string_view(_buffer.data(), _buffer.size());
            size_t found = sv.find_first_of(chars, _pos.offset);
            if (found == std::string_view::npos) return npos;
            return Position(0, found);
        }

        [[nodiscard]] Position find_first_position(char chars) const {
            if (_pos == npos) return npos;
            auto sv = std::string_view(_buffer.data(), _buffer.size());
            size_t found = sv.find_first_of(chars, _pos.offset);
            if (found == std::string_view::npos) return npos;
            return Position(0, found);
        }

        [[nodiscard]] size_t find_first_offset(char chars) const {
            Position p = find_first_position(chars);
            return (p == npos) ? kNPos : p.offset;
        }

        [[nodiscard]] size_t find_first_offset(std::string_view chars) const {
            Position p = find_first_position(chars);
            return (p == npos) ? kNPos : p.offset;
        }

        char operator*() const {
            DKCHECK(_pos != npos && _pos.offset < _buffer.size());
            return _buffer[_pos.offset];
        }

        Peeker &operator++() { return seek_end(1); }

        Peeker operator++(int) {
            Peeker ret = *this;
            ret.seek_end(1);
            return ret;
        }

        Peeker &operator--() { return seek_start(1); }

        Peeker operator--(int) {
            Peeker ret = *this;
            ret.seek_start(1);
            return ret;
        }

        Peeker operator+(size_t n) const {
            Peeker ret = *this;
            ret.seek_end(n);
            return ret;
        }

        Peeker &operator+=(size_t n) { return seek_end(n); }

        Peeker operator-(size_t n) const {
            Peeker ret = *this;
            ret.seek_start(n);
            return ret;
        }

        Peeker &operator-=(size_t n) { return seek_start(n); }

        operator std::string_view() const {
            if (_pos == npos) return {};
            return std::string_view(_buffer.data() + _pos.offset, _buffer.size() - _pos.offset);
        }

        std::optional<std::string_view> readn(size_t n) {
            if (_pos == npos) return std::nullopt;
            size_t take = std::min(n, _buffer.size() - _pos.offset);
            std::string_view result(_buffer.data() + _pos.offset, take);
            seek_end(take);
            return result;
        }

        [[nodiscard]] bool eof() const { return _pos == npos; }
        explicit operator bool() const { return _pos != npos; }
        [[nodiscard]] size_t has_read() const { return _has_read; }

    private:
        void init_buffer() {
            _has_read = 0;
            if (_buffer.empty()) {
                _pos = npos;
            } else {
                _pos = Position(0, 0);
            }
        }

        void set_to_npos() {
            _pos = npos;
            _has_read = _buffer.size();
        }

    private:
        Source _buffer;
        Position _pos = npos;
        size_t _has_read = 0;
    };

    using StringViewPeeker = Peeker<std::string_view>;
    using StringPeeker = Peeker<std::string>;
} // namespace fermat
