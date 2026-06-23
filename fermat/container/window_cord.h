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
#include <fermat/container/deque.h>

namespace fermat {

    /// FIFO queue of physically separate CordBuffer chunks.
    ///
    /// Each push() enqueues one complete chunk (typically built by the caller via readv,
    /// split, cache, etc.). WindowCord does not append bytes, seal boundaries, or parse
    /// protocols — it only holds and schedules chunks.
    ///
    /// Access rules:
    ///   - push(): enqueue at tail (one CordBuffer per call).
    ///   - share_at(i) / share_front(): non-destructive read-only view of any chunk.
    ///   - operator[]: in-place mutable access to a queued chunk (caller controls chunking).
    ///   - take_front() / pop_front(): consume **head only** (FIFO). No middle or tail removal.
    ///
    /// byte_size() and chunk_count() are syntactic sugar over a simple traversal.
    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class WindowCord {
    public:
        using chunk_type = CordBufferBase<Alignment, BlockSize>;
        using deque_type = Deque<chunk_type>;
        using size_type = typename deque_type::size_type;
        using iterator = typename deque_type::iterator;
        using const_iterator = typename deque_type::const_iterator;

    public:
        WindowCord() = default;

        WindowCord(WindowCord &&) noexcept = default;
        WindowCord &operator=(WindowCord &&) noexcept = default;

        WindowCord(const WindowCord &) = delete;
        WindowCord &operator=(const WindowCord &) = delete;

        // ---------------------------------------------------------------------
        // Enqueue
        // ---------------------------------------------------------------------

        /// Takes ownership of @p chunk and enqueues it.
        void push(chunk_type &&chunk) {
            _chunks.push_back(std::move(chunk));
        }

        /// Enqueues a shared copy of @p chunk (segments alias the source buffers).
        void push(const chunk_type &chunk) {
            _chunks.push_back(chunk.share());
        }

        // ---------------------------------------------------------------------
        // Introspection (traversal sugar)
        // ---------------------------------------------------------------------

        [[nodiscard]] bool empty() const noexcept {
            return _chunks.empty();
        }

        [[nodiscard]] size_type chunk_count() const noexcept {
            return _chunks.size();
        }

        /// Sum of chunk_type::size() over all queued chunks.
        [[nodiscard]] size_t byte_size() const noexcept {
            size_t total = 0;
            for (const auto &chunk: _chunks) {
                total += chunk.size();
            }
            return total;
        }

        // ---------------------------------------------------------------------
        // Random access / iteration
        // ---------------------------------------------------------------------

        [[nodiscard]] chunk_type &operator[](size_type index) {
            KCHECK(index < _chunks.size());
            return _chunks[index];
        }

        [[nodiscard]] const chunk_type &operator[](size_type index) const {
            KCHECK(index < _chunks.size());
            return _chunks[index];
        }

        [[nodiscard]] chunk_type &front() {
            KCHECK(!_chunks.empty());
            return _chunks.front();
        }

        [[nodiscard]] const chunk_type &front() const {
            KCHECK(!_chunks.empty());
            return _chunks.front();
        }

        [[nodiscard]] iterator begin() noexcept {
            return _chunks.begin();
        }

        [[nodiscard]] iterator end() noexcept {
            return _chunks.end();
        }

        [[nodiscard]] const_iterator begin() const noexcept {
            return _chunks.begin();
        }

        [[nodiscard]] const_iterator end() const noexcept {
            return _chunks.end();
        }

        [[nodiscard]] const_iterator cbegin() const noexcept {
            return _chunks.cbegin();
        }

        [[nodiscard]] const_iterator cend() const noexcept {
            return _chunks.cend();
        }

        // ---------------------------------------------------------------------
        // Non-destructive share (any index)
        // ---------------------------------------------------------------------

        /// Shared view of chunk @p index. The queue is unchanged.
        [[nodiscard]] chunk_type share_at(size_type index) const {
            KCHECK(index < _chunks.size());
            return _chunks[index].share();
        }

        /// Shared view of the front chunk.
        [[nodiscard]] chunk_type share_front() const {
            KCHECK(!_chunks.empty());
            return _chunks.front().share();
        }

        // ---------------------------------------------------------------------
        // FIFO consumption (head only)
        // ---------------------------------------------------------------------

        /// Removes and returns the front chunk.
        [[nodiscard]] chunk_type take_front() {
            KCHECK(!_chunks.empty());
            chunk_type chunk = std::move(_chunks.front());
            _chunks.pop_front();
            return chunk;
        }

        /// Discards the front chunk without returning it.
        void pop_front() noexcept {
            KCHECK(!_chunks.empty());
            _chunks.pop_front();
        }

        void clear() noexcept {
            _chunks.clear();
        }

    private:
        deque_type _chunks;
    };

} // namespace fermat
