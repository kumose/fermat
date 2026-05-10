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

#include <fermat/memory/arena.h>
#include <fermat/memory/malloc.h>

namespace fermat {
    Arena::Arena(uint32_t initial_block, uint32_t alignment)
        : _block_size(initial_block) {
        add_chunk(initial_block);
    }

    Arena::~Arena() {
        for (auto &c: _chunks) {
            mi_free(c.data);
            c.data = nullptr;
        }
    }

    void Arena::add_chunk(size_t size) {
        Chunk c;
        c.data = mi_malloc(size);
        if (!c.data) throw std::bad_alloc();
        c.capacity = size;
        c.offset = 0;

        _chunks.push_back(c);
        _current = &_chunks.back();
    }

    void *Arena::alloc(size_t n) {
        // Case 1: current chunk has enough free space
        if (_current->capacity - _current->offset >= n) {
            void *ptr = static_cast<char *>(_current->data) + _current->offset;
            _current->offset += static_cast<uint32_t>(n);
            return ptr;
        }

        // Case 2: current chunk insufficient
        if (n > _block_size) {
            // Oversized allocation: allocate a dedicated chunk, do NOT replace _current.
            Chunk big;

            big.data = mi_malloc(n);

            if (!big.data) throw std::bad_alloc();
            big.capacity = static_cast<uint32_t>(n);
            big.offset = static_cast<uint32_t>(n); // entire chunk consumed
            _chunks.push_back(big);
            return big.data;
        }
        // Normal allocation: allocate a fresh standard-sized chunk and make it current
        add_chunk(_block_size); // _current updated to the new chunk
        void *ptr = _current->data; // offset is 0 after add_chunk
        _current->offset = static_cast<uint32_t>(n);
        return ptr;
    }

    size_t Arena::used() const noexcept {
        size_t total = 0;
        for (const auto& chunk : _chunks) {
            total += chunk.offset;
        }
        return total;
    }
} // namespace fermat
