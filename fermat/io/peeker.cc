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

#include <fermat/io/peeker.h>
#include  <turbo/numeric/bits.h>

namespace fermat {
    void Peeker::init_buffer() {
        _has_read = 0;
        _positions = {kNPos, kNPos};
        _view = nullptr;
        _current = {};
        if (!_buffer) return;
        if (_buffer->readable_blocks() > 0) {
            _positions.index = 0;
            _positions.offset = 0;
            _view = _buffer->readable_peek(0);
            _current = std::string_view(_view->block->data + _view->offset, _view->length);
        }
    }

    Position Peeker::tellg() const {
        return _positions;
    }

    char Peeker::operator*() const {
        DKCHECK(_view != nullptr);
        DKCHECK(!_current.empty() && _current.size() > _positions.offset);
        return _current[_positions.offset];
    }

    Peeker &Peeker::operator++() {
        return seek_end(1);
    }

    Peeker Peeker::operator++(int) {
        Peeker ret = *this;
        ret.seek_end(1);
        return ret;
    }

    Peeker &Peeker::operator--() {
        return seek_start(1);
    }

    Peeker Peeker::operator--(int) {
        Peeker ret = *this;
        ret.seek_start(1);
        return ret;
    }

    Peeker &Peeker::operator+=(size_t n) {
        return seek_end(n);
    }

    Peeker Peeker::operator+(size_t n) {
        Peeker ret = *this;
        ret.seek_end(n);
        return ret;
    }

    Peeker &Peeker::operator-=(size_t n) {
        return seek_start(n);
    }

    Peeker Peeker::operator-(size_t n) {
        Peeker ret = *this;
        ret.seek_start(n);
        return ret;
    }

    void Peeker::set_to_npos() {
        _positions = npos;
        _view = nullptr;
        _current = {};
        _has_read = 0;
        if (_buffer) {
            _has_read = _buffer->size();
        }
    }

    /// @brief Advance the cursor position by n bytes.
    /// @param n Number of bytes to move forward.
    /// @return Reference to this peeker.
    Peeker &Peeker::seek_end(size_t n) {
        if (n == 0 || _positions == npos) return *this;
        if (n >= _buffer->size() - _has_read) {
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
            _current = std::string_view(_view->block->data + _view->offset, _view->length);
        }
        return *this;
    }


    Peeker &Peeker::seek_start(size_t n) {
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
            _current = std::string_view(_view->block->data + _view->offset, _view->length);
            _positions.offset = _current.size();
        }
        return *this;
    }

    Position Peeker::seek_to(size_t pos) {
        if (pos > _has_read) {
            seek_end(pos - _has_read);
        } else if (pos < _has_read) {
            seek_start(_has_read - pos);
        }
        return _positions;
    }


    Peeker &Peeker::seek_to(Position pos) {
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
            if (v) consumed += v->length;
        }
        consumed += pos.offset;
        _positions = pos;
        _view = view;
        _current = std::string_view(view->block->data + view->offset, view->length);
        _has_read = consumed;
        return *this;
    }

    Peeker::operator std::string_view() {
        if (_positions == npos) {
            return {};
        }
        return _current.substr(_positions.offset);
    }

    std::optional<std::string_view> Peeker::readn(size_t n) {
        if (_positions == npos) {
            return std::nullopt;
        }
        size_t available = _current.size() - _positions.offset;
        size_t take = std::min(n, available);
        std::string_view result(_current.data() + _positions.offset, take);
        seek_end(take);
        return result;
    }

    bool Peeker::eof() const {
        return _positions == npos;
    }

    Peeker::operator bool() const {
        return _positions == npos;
    }

} // namespace fermat
