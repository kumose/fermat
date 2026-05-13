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

#include <fermat/io/cord_buffer.h>

namespace fermat {
    void BufferLease::set(std::vector<turbo::span<char> > sp) {
        _spans = std::move(sp);
        for (const auto &s: _spans) {
            _capacity += s.size();
        }
    }

    void BufferLease::set(turbo::span<char> sp) {
        _capacity = sp.size();
        _spans.push_back(sp);
    }

    /// @brief Sequential write that automatically handles crossing span boundaries.
    turbo::Status BufferLease::write(const char *data, size_t len) {
        if (len == 0) return turbo::OkStatus();
        if (_total_size + len > _capacity) {
            return turbo::out_of_range_error("BufferLease capacity exceeded, size:", _total_size, " len:", len, " capacity:", _capacity);
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
    void BufferLease::pop_back(size_t n) {
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
    void BufferLease::clear() {
        _total_size = 0;
        _index = 0;
        _offset = 0;
    }

    void BufferLease::clear_lease() {
        clear();
        _capacity = 0;
        _spans.clear();
    }

    void BufferLease::visit_remaining(const VisitorCallback &visitor) const {
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

    void BufferLease::advance(size_t n) {
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


} // namespace fermat
