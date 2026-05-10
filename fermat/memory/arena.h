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

#include <deque>
#include <cstddef>
#include <cstdint>

namespace fermat {
    class Arena {
    public:
        struct Chunk {
            void *data{nullptr};
            uint32_t capacity{0};
            uint32_t offset{0};
        };

    public:
        Arena(uint32_t initial_block = 4096, uint32_t alignment = 0);

        ~Arena();

        Arena(const Arena &) = delete;

        Arena &operator=(const Arena &) = delete;

        // --------------------------------------------------
        // Allocation
        // --------------------------------------------------
        void *alloc(size_t n);

        template<typename T, typename... Args>
        T *create(Args &&... args);

        size_t used() const noexcept;

    private:
        void add_chunk(size_t size);

    private:
        std::deque<Chunk> _chunks;
        Chunk *_current = nullptr;
        uint32_t _block_size{4096};
    };

    /// @brief Allocates memory for an object of type `T` and constructs it with the given arguments.
    /// @tparam T The type of object to create.
    /// @param args Arguments to forward to `T`'s constructor.
    /// @return Pointer to the constructed object (the memory is managed by the arena).
    /// @throws std::bad_alloc if memory allocation fails, or any exception thrown by `T`'s constructor.
    template<typename T, typename... Args>
    T* Arena::create(Args&&... args) {
        void* mem = alloc(sizeof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }
} // namespace fermat
