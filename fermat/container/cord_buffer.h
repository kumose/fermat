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
#include <fermat/container/vector.h>
#include <fermat/container/reader_writer.h>
#include <fermat/container/receiver.h>
#include <fermat/container/deque.h>
#include <deque>
#include <list>
#include <memory>
#include <new>

namespace fermat {
    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class CordBufferBase {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr bool is_iobuf = true;
        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;
        static constexpr size_t kMaxReadVSpans = 32;

        struct BufferRef {
            std::shared_ptr<Buffer<char, Alignment> > buffer;
            std::optional<turbo::span<char> > view;
        };

    public:
        using value_type = char;
        using buffer_type = Buffer<value_type, Alignment>;

        using vector_type = Deque<BufferRef>;
        using const_pointer = const value_type *;
        using const_reference = const value_type &;
        using size_type = typename buffer_type::size_type;

        using const_buffer_iterator = typename vector_type::const_iterator;

        class CordIterator {
        public:
            ~CordIterator() = default;

            value_type operator*() const {
                return *_view_begin;
            }

            CordIterator &operator++() {
                if (_index_read >= _cord->size()) {
                    return *this;
                }

                if (_view_begin != _view.end()) {
                    ++_view_begin;
                } else {
                    ++_cur;
                    if (_cur != _cord->buffer_end()) {
                        _view = turbo::span<value_type>(_cur->buffer->data(), _cur->buffer->size());
                        _view_begin = _view.begin();
                    } else {
                        _view = turbo::span<value_type>{};
                        _view_begin = _view.end();
                    }
                }


                ++_index_read;
                return *this;
            }

            CordIterator operator++(int) {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const CordIterator &rhs) const {
                return _cord == rhs._cord && _index_read == rhs._index_read;
            }

            bool operator!=(const CordIterator &rhs) const {
                return _cord != rhs._cord || _index_read != rhs._index_read;
            }

        private:
            friend class CordBufferBase;

            CordIterator(const CordBufferBase *cord) : _cord(cord), _cur(_cord->buffer_begin()) {
                if (_cur != _cord->buffer_end()) {
                    _view = turbo::span<char>(_cur->buffer->data(), _cur->buffer->size());
                    _view_begin = _view.begin();
                }
            }

            CordIterator(const CordBufferBase *cord, bool) : _cord(cord), _cur(_cord->buffer_end()), _view(),
                                                             _view_begin(_view.end()) {
            }

            size_type has_read() const {
                return _index_read + 1;
            }

            size_type remaining() const {
                return _cord->size() - _index_read - 1;
            }

        private:
            const CordBufferBase *_cord;
            const_buffer_iterator _cur;
            turbo::span<value_type> _view;
            turbo::span<value_type>::const_iterator _view_begin;
            size_type _index_read{0};
        };

        using const_iterator = CordIterator;

        /// ZeroCopyInputStream adapter for CordBufferBase.
        /// Provides sequential read access to the underlying concatenated buffers,
        /// respecting any per‑segment views (sub‑ranges).
        class InputStream {
        public:
            explicit InputStream(const CordBufferBase *cord)
                : _cord(cord) {
                _skip_empty();
            }

            /// Obtains the next chunk of data.
            /// The chunk corresponds to the current segment (respecting any view).
            /// After a successful call, the caller may read up to *size bytes from *data.
            /// @param data  Output pointer to the beginning of the chunk.
            /// @param size  Output size of the chunk in bytes.
            /// @return true if there is more data, false at end of stream.
            bool next(const void **data, int *size) {
                // Find a segment with remaining data.
                while (_index < _cord->_views.size()) {
                    const auto &ref = _cord->_views[_index];
                    // Obtain the effective span (view if present, otherwise full buffer)
                    turbo::span<char> span = ref.view
                                                 ? *ref.view
                                                 : turbo::span<char>(ref.buffer->data(), ref.buffer->size());
                    if (span.size() > _offset) {
                        // There is data to return.
                        *data = span.data() + _offset;
                        *size = static_cast<int>(span.size() - _offset);
                        // Record the chunk size for possible back_up.
                        _last_chunk_size = *size;
                        ++_index;
                        _byte_count += *size;
                        _offset = 0;
                        return true;
                    }
                    // Current segment is empty; move to the next one.
                    ++_index;
                    _offset = 0;
                }
                return false;
            }

            /// Backs up a number of bytes, which must not exceed the size of the last chunk
            /// returned by next(). After backing up, the next call to next() will return the
            /// same chunk again, starting from the new offset.
            /// @param count Number of bytes to back up (must be <= size of last returned chunk).
            void back_up(int count) {
                if (count <= 0) return;
                // count must not exceed the size of the last returned chunk.
                KCHECK(static_cast<size_t>(count) <= _last_chunk_size) << "back_up: count exceeds last chunk size";
                // Move the stream position backwards by `count` bytes.
                // Since we advanced _index after the last next(), we need to go back to the
                // segment that provided the last chunk.
                if (_index == 0) {
                    // Last chunk came from the first segment.
                    const auto &ref = _cord->_views[0];
                    turbo::span<char> span = ref.view
                                                 ? *ref.view
                                                 : turbo::span<char>(ref.buffer->data(), ref.buffer->size());
                    _offset = span.size() - static_cast<size_t>(count);
                    _index = 0; // stay at the first segment
                } else {
                    // Last chunk came from the segment at index (_index - 1).
                    --_index;
                    const auto &ref = _cord->_views[_index];
                    turbo::span<char> span = ref.view
                                                 ? *ref.view
                                                 : turbo::span<char>(ref.buffer->data(), ref.buffer->size());
                    _offset = span.size() - static_cast<size_t>(count);
                }
                _byte_count -= count;
                _last_chunk_size = 0; // After back_up, the "last chunk" is invalidated.
            }

            /// Skips up to `count` bytes. May cross multiple segments.
            /// @param count Number of bytes to skip.
            /// @return true if exactly `count` bytes were skipped, false if not enough data.
            [[nodiscard]] bool skip(int count) {
                if (count <= 0) return true;
                // Check if enough data remains.
                if (static_cast<size_t>(count) > _cord->size() - _byte_count)
                    return false;
                // Perform the skip – guaranteed to succeed.
                size_t remaining = static_cast<size_t>(count);
                size_t idx = _index;
                size_t off = _offset;
                while (remaining > 0) {
                    const auto &ref = _cord->_views[idx];
                    turbo::span<char> span = ref.view
                                                 ? *ref.view
                                                 : turbo::span<char>(ref.buffer->data(), ref.buffer->size());
                    size_t avail = span.size() - off;
                    size_t take = std::min(remaining, avail);
                    remaining -= take;
                    off += take;
                    if (off == span.size()) {
                        ++idx;
                        off = 0;
                    }
                }
                _index = idx;
                _offset = off;
                _byte_count += count;
                _last_chunk_size = 0; // After skip, the last chunk is no longer valid.
                _skip_empty();
                return true;
            }

            /// Returns the total number of bytes read (or skipped) so far.
            [[nodiscard]] int64_t byte_count() const {
                return _byte_count;
            }

            [[nodiscard]] size_t remain() const {
                return _cord->size() - _byte_count;
            }

        private:
            /// Advances to the first non‑empty segment (non‑zero effective size).
            void _skip_empty() {
                while (_index < _cord->_views.size()) {
                    const auto &ref = _cord->_views[_index];
                    turbo::span<char> span = ref.view
                                                 ? *ref.view
                                                 : turbo::span<char>(ref.buffer->data(), ref.buffer->size());
                    if (!span.empty()) break;
                    ++_index;
                }
                _offset = 0;
            }

            const CordBufferBase *_cord;
            size_t _index{0}; ///< Current segment index.
            size_t _offset{0}; ///< Offset within the current segment.
            int64_t _byte_count{0}; ///< Total bytes consumed.
            size_t _last_chunk_size{0}; ///< Size of the last chunk returned by next() (for back_up validation).
        };

    public:
        CordBufferBase() = default;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        CordBufferBase(CordBufferBase &&rhs) noexcept
            : _views(std::move(rhs._views)),
              _total_size(std::exchange(rhs._total_size, 0)) {
        }

        ~CordBufferBase() = default;

        const_buffer_iterator buffer_begin() const {
            return _views.begin();
        }

        const_buffer_iterator buffer_end() const {
            return _views.end();
        }

        const_iterator begin() const {
            return CordIterator(this);
        }

        const_iterator end() const {
            return CordIterator(this, true);
        }


        InputStream input_stream() const {
            return InputStream(this);
        }

        template<typename... Args>
        void format(const turbo::FormatSpec<Args...> &fmt, const Args &... args) {
            append(turbo::str_format(fmt, args...)).ignore_error();
        }

        template<typename... Args>
        void cat(const Args &... args) {
            append(turbo::str_cat(args...)).ignore_error();
        }

        [[nodiscard]] size_t block_size() const;

        [[nodiscard]] size_t alignment() const;


        [[nodiscard]] size_t size() const;

        [[nodiscard]] size_t blocks() const;

        turbo::Status append(const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset, size_t length);

        turbo::Status append(std::shared_ptr<Buffer<value_type, Alignment> > &&buf);

        turbo::Status append(std::vector<std::shared_ptr<Buffer<value_type, Alignment> > > &&bufs);

        turbo::Status prepend(const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset, size_t length);

        /// Prepends a single buffer to the front of the cord by taking ownership (move semantics).
        /// The writable buffer is updated only if the cord was empty before (so the new buffer
        /// becomes both head and tail); otherwise the original tail remains the writable buffer.
        /// @param buf The buffer to take ownership of (moved).
        /// @return OkStatus on success.
        turbo::Status prepend(std::shared_ptr<Buffer<value_type, Alignment> > &&buf);

        /// Prepends multiple buffers to the front of the cord.
        /// The buffers are prepended in the order they appear in the vector (the first element
        /// becomes the new first segment). After insertion, the writable buffer is set to the
        /// current tail (the last segment), which may be a newly added buffer if the cord was
        /// empty, or the original tail if the cord was non‑empty.
        turbo::Status prepend(std::vector<std::shared_ptr<Buffer<value_type, Alignment> > > &&bufs);

        /// Creates empty buffers whose total capacity is at least `bytes_needed`.
        /// New buffers use `BlockSize` as the default reserve granularity (recommended, not required
        /// for externally appended segments, which may use any aligned pool tier capacity).
        static std::vector<std::shared_ptr<buffer_type> > create_buffers(size_t bytes_needed);

        /// Returns a span of the current writable area (the entire remaining capacity of the tail buffer).
        /// The tail buffer's size is immediately advanced to its capacity, assuming the caller will
        /// write data into the entire span. If the caller writes less, they must call output_back_up()
        /// to reduce the logical size.
        /// @return A span covering the writable area. May be empty if the cord cannot allocate more space.
        turbo::span<char> output_next();


        /// Traditional interface: returns a pointer to a writable block and its size.
        /// The tail buffer's size is advanced to its capacity.
        /// @param out Output pointer to the beginning of the writable area.
        /// @param size Output size of the writable area in bytes.
        bool output_next(void **out, size_t *size);

        /// Rolls back the last `n` bytes that were claimed by output_next() but not actually written.
        /// The `n` can be as large as the total size of the cord. If it exceeds the size of the last
        /// buffer, it will continue rolling back through previous buffers, and empty buffers are removed.
        /// @param n The number of bytes to roll back (must be <= total size).
        void output_backup(int n);

        turbo::Status append(std::string_view data);

        turbo::Status append(const void * TURBO_RESTRICT data, size_t size);

        turbo::Result<size_t> append_by_readv(IOReader &reader, size_t max_limited, bool restart_block = false);

        turbo::Result<size_t> append_by_read(IOReader &reader, size_t max_limited);

        turbo::Result<size_t> append_by_pread(IOReader &reader, size_t offset, size_t max_limited);

        /////////////////////////////////////////////
        /// operators

        /// Appends all segments from another CordBufferBase to this one.
        /// The segments are copied (shared ownership) – no deep copy of buffer data.
        /// @param rhs The source cord.
        void append(const CordBufferBase &rhs);

        void append(CordBufferBase &&rhs);

        void prepend(const CordBufferBase &rhs);

        /// Moves all segments from rhs to the front of this cord.
        /// After the operation, rhs is left empty.
        /// The writable buffer is set to the new tail (the last segment of rhs).
        void prepend(CordBufferBase &&rhs);

        /// Creates a new CordBufferBase that shares ownership of all underlying buffers
        /// with the current object. The new object is independent but refers to the same data.
        /// The writable buffer is set to the sentinel (kZeroBuffer) to prevent accidental writes
        /// into shared memory.
        /// @return A new CordBufferBase sharing the same data.
        CordBufferBase share() const;

        CordBufferBase copy() const;

        /// Copy assignment operator. Uses copy-and-swap idiom.
        /// @param rhs The source cord.
        /// @return Reference to this object.
        CordBufferBase &operator=(const CordBufferBase &rhs);

        // Move assignment: release current resources, then transfer from rhs.
        CordBufferBase &operator=(CordBufferBase &&rhs) noexcept;

        CordBufferBase &operator=(std::string_view str);

        CordBufferBase &operator=(turbo::span<char> span);

        CordBufferBase &operator=(const std::vector<char> &data);

        template<size_t BA>
        CordBufferBase &operator=(const Buffer<char, BA> &data);

        CordBufferBase &operator<<(std::string_view str);

        CordBufferBase &operator<<(turbo::span<char> span);

        CordBufferBase &operator<<(const std::vector<char> &data);

        template<size_t BA>
        CordBufferBase &operator<<(const Buffer<char, BA> &data);

        template<typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
        CordBufferBase &operator<<(T v);


        /// Swaps the contents of this CordBufferBase with another.
        /// @param other The other buffer to swap with.
        void swap(CordBufferBase &other) noexcept;

        void clear() noexcept;

        void pop_front(size_t n) noexcept;

        void pop_front_buffer() noexcept;

        const BufferRef &front_buffer() const noexcept;

        void pop_back(size_t n) noexcept;

        void pop_back_buffer() noexcept;

        const BufferRef &back_buffer() const noexcept;

    protected:
        Buffer<char, Alignment> *get_write_able_buffer();

    protected:
        static const buffer_type kZeroBuffer;
        buffer_type *_write_buffer{const_cast<buffer_type *>(&kZeroBuffer)};
        ///< All block views (including Umount ones).
        vector_type _views;
        ///< Total logical bytes stored.
        size_t _total_size{0};
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines
    ///
    ///////////////////////////////////////////////////////////////////////////
    ///
    template<size_t Alignment, size_t BlockSize>
    const typename CordBufferBase<Alignment, BlockSize>::buffer_type CordBufferBase<Alignment,
        BlockSize>::kZeroBuffer;


    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBufferBase<Alignment, BlockSize>::block_size() const {
        return BlockSize;
    }


    /// @brief Total logical bytes in the CordBufferBase.
    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBufferBase<Alignment, BlockSize>::size() const {
        return _total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBufferBase<Alignment, BlockSize>::blocks() const {
        return _views.size();
    }


    template<size_t Alignment, size_t BlockSize>
    inline size_t CordBufferBase<Alignment, BlockSize>::alignment() const {
        return Alignment;
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(std::string_view data) {
        return append(data.data(), data.size());
    }

    template<size_t Alignment, size_t BlockSize>
    Buffer<char, Alignment> *CordBufferBase<Alignment, BlockSize>::get_write_able_buffer() {
        if (_write_buffer->capacity() > _write_buffer->size()) {
            return _write_buffer;
        }
        BufferRef ref;
        ref.buffer = std::make_shared<buffer_type>();
        ref.buffer->reserve(BlockSize);
        _write_buffer = ref.buffer.get();
        _views.push_back(std::move(ref));
        return _write_buffer;
    }


    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::swap(CordBufferBase &other) noexcept {
        using std::swap;
        swap(_views, other._views);
        swap(_total_size, other._total_size);
        swap(_write_buffer, other._write_buffer);
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(std::shared_ptr<Buffer<value_type, Alignment> > &&buf) {
        if (!buf || buf->empty()) return turbo::OkStatus();
        _total_size += buf->size();
        BufferRef ref;
        ref.buffer = std::move(buf);
        _views.push_back(std::move(ref));
        // Always set _write_buffer to the newly appended buffer (the tail).
        _write_buffer = _views.back().buffer.get();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(
        const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset, size_t length) {
        if (!buf || buf->empty()) return turbo::OkStatus();
        if (offset + length > buf->size()) {
            return turbo::invalid_argument_error("overflow buffer size");
        }
        _total_size += buf->size();
        BufferRef ref;
        ref.buffer = buf;
        ref.view = turbo::span<char>(buf->data() + offset, length);
        _views.push_back(std::move(ref));
        // Always set _write_buffer to the newly appended buffer (the tail).
        _write_buffer = _views.back().buffer.get();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(
        std::vector<std::shared_ptr<Buffer<value_type, Alignment> > > &&bufs) {
        if (bufs.empty()) {
            return turbo::OkStatus();
        }

        size_t added_size = 0;
        for (auto &buf: bufs) {
            if (!buf || buf->empty()) {
                continue; // skip null or empty buffers
            }
            added_size += buf->size();
            BufferRef ref;
            ref.buffer = std::move(buf);
            // owned buffer, no view needed
            _views.push_back(std::move(ref));
        }

        if (added_size == 0) {
            return turbo::OkStatus(); // nothing was appended
        }

        _total_size += added_size;
        // Set writable buffer to the last appended segment (the new tail)

        _write_buffer = _views.back().buffer.get();

        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append(const CordBufferBase &rhs) {
        for (const auto &ref: rhs._views) {
            BufferRef new_ref;
            new_ref.buffer = ref.buffer;
            if (ref.view) {
                new_ref.view = *ref.view;
            } else {
                new_ref.view = turbo::span<char>(ref.buffer->data(), ref.buffer->size());
            }
            _views.push_back(std::move(new_ref));
        }
        _total_size += rhs._total_size;
        _write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append(CordBufferBase &&rhs) {
        if (rhs._views.empty()) return;
        _views.insert(_views.end(),
                      std::make_move_iterator(rhs._views.begin()),
                      std::make_move_iterator(rhs._views.end()));
        _total_size += rhs._total_size;
        rhs._views.clear();
        rhs._total_size = 0;
        rhs._write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
        const auto &last = _views.back();
        _write_buffer = last.buffer.get();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::prepend(const CordBufferBase &rhs) {
        if (rhs._views.empty()) return;
        // Insert copies of rhs segments at the front in reverse order to preserve original order.
        for (auto it = rhs._views.rbegin(); it != rhs._views.rend(); ++it) {
            const auto &ref = *it;
            BufferRef new_ref;
            new_ref.buffer = ref.buffer;
            if (ref.view) {
                new_ref.view = *ref.view;
            } else {
                new_ref.view = turbo::span<char>(ref.buffer->data(), ref.buffer->size());
            }
            _views.push_front(std::move(new_ref));
        }
        _total_size += rhs._total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::prepend(CordBufferBase &&rhs) {
        if (rhs._views.empty()) return;
        // Insert segments at the front in reverse order to maintain the original order of rhs.
        for (auto it = rhs._views.rbegin(); it != rhs._views.rend(); ++it) {
            _views.push_front(std::move(*it));
        }
        _total_size += rhs._total_size;
        // Clear rhs to a valid empty state.
        rhs._views.clear();
        rhs._total_size = 0;
        rhs._write_buffer = const_cast<buffer_type *>(&kZeroBuffer);

        _write_buffer = _views.back().buffer.get();
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> CordBufferBase<Alignment, BlockSize>::share() const {
        CordBufferBase result;
        for (const auto &ref: _views) {
            BufferRef new_ref;
            // Share the underlying buffer (increase reference count)
            new_ref.buffer = ref.buffer;
            // Copy the view: if the original has a view, use it; otherwise create a full‑buffer view.
            if (ref.view) {
                new_ref.view = *ref.view;
            } else {
                new_ref.view = turbo::span<char>(ref.buffer->data(), ref.buffer->size());
            }
            result._views.push_back(std::move(new_ref));
        }
        result._total_size = _total_size;
        // Shared copy must not write into the original buffers; reset the writable pointer.
        result._write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> CordBufferBase<Alignment, BlockSize>::copy() const {
        CordBufferBase result;
        for (const auto &ref: _views) {
            const char *data = ref.view ? ref.view->data() : ref.buffer->data();
            size_t len = ref.view ? ref.view->size() : ref.buffer->size();
            result.append(data, len).ignore_error();
        }
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(const CordBufferBase &rhs) {
        if (this != &rhs) {
            CordBufferBase temp = rhs.share(); // deep copy (or shallow if copy is shallow? Actually copy() is deep)
            swap(temp);
        }
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(CordBufferBase &&rhs) noexcept {
        if (this != &rhs) {
            // Transfer ownership of the views container.
            _views = std::move(rhs._views);
            // Transfer total size and clear rhs.
            _total_size = rhs._total_size;
            rhs._total_size = 0;
            // Transfer writable buffer pointer and reset rhs to the sentinel.
            _write_buffer = rhs._write_buffer;
            rhs._write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
        }
        return *this;
    }

    /// Assigns the cord to a copy of the data from a string view.
    /// The cord is first cleared, then the data is appended.
    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(std::string_view str) {
        clear();
        append(str).ignore_error();
        return *this;
    }

    /// Assigns the cord to a copy of the data from a span of characters.
    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(turbo::span<char> span) {
        clear();
        append(span.data(), span.size()).ignore_error();
        return *this;
    }

    /// Assigns the cord to a copy of the data from a vector of characters.
    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(const std::vector<char> &data) {
        clear();
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Assigns the cord to a copy of the data from a Buffer.
    /// The entire buffer's content (from its beginning to its current size) is copied.
    /// The buffer's view (if any) is ignored; only the actual data is copied.
    template<size_t Alignment, size_t BlockSize>
    template<size_t BA>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(const Buffer<char, BA> &data) {
        clear();
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends a string view to the cord.
    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator<<(std::string_view str) {
        append(str).ignore_error();
        return *this;
    }

    /// Appends a span of characters.
    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator<<(turbo::span<char> span) {
        append(span.data(), span.size()).ignore_error();
        return *this;
    }

    /// Appends the content of a vector.
    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator<<(const std::vector<char> &data) {
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends the content of a Buffer.
    template<size_t Alignment, size_t BlockSize>
    template<size_t BA>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator<<(const Buffer<char, BA> &data) {
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends an arithmetic value formatted as a string.
    template<size_t Alignment, size_t BlockSize>
    template<typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, int> >
    CordBufferBase<Alignment, BlockSize> &CordBufferBase<Alignment, BlockSize>::operator<<(T v) {
        cat(v);
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::clear() noexcept {
        _views.clear();
        _total_size = 0;
        _write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend(
        const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset, size_t length) {
        if (!buf || buf->empty()) {
            return turbo::OkStatus();
        }

        if (offset + length > _total_size) {
            return turbo::invalid_argument_error("overflow buffer");
        }

        BufferRef ref;
        ref.buffer = buf;
        ref.view = turbo::span<char>(buf->data() + offset, length);
        _views.push_front(std::move(ref));
        _total_size += _views.front().buffer->size();

        if (!_views.empty()) {
            // The new buffer is the only segment, so it is also the tail.
            _write_buffer = _views.back().buffer.get();
        }
        // Otherwise the existing tail remains the writable buffer; do nothing.

        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend(std::shared_ptr<Buffer<value_type, Alignment> > &&buf) {
        if (!buf || buf->empty()) {
            return turbo::OkStatus();
        }

        BufferRef ref;
        ref.buffer = std::move(buf);
        _views.push_front(std::move(ref));
        _total_size += _views.front().buffer->size();

        if (!_views.empty()) {
            // The new buffer is the only segment, so it is also the tail.
            _write_buffer = _views.back().buffer.get();
        }
        // Otherwise the existing tail remains the writable buffer; do nothing.

        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend(
        std::vector<std::shared_ptr<Buffer<value_type, Alignment> > > &&bufs) {
        if (bufs.empty()) {
            return turbo::OkStatus();
        }

        size_t added_size = 0;
        // Insert in reverse order so that bufs[0] becomes the new front.
        for (auto it = bufs.rbegin(); it != bufs.rend(); ++it) {
            auto &buf = *it;
            if (!buf || buf->empty()) {
                continue;
            }
            added_size += buf->size();
            BufferRef ref;
            ref.buffer = std::move(buf);
            _views.push_front(std::move(ref));
        }

        if (added_size == 0) {
            return turbo::OkStatus();
        }

        _total_size += added_size;
        // Set writable buffer to the tail (last segment) unconditionally.
        _write_buffer = _views.back().buffer.get();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    std::vector<std::shared_ptr<typename CordBufferBase<Alignment, BlockSize>::buffer_type> >
    CordBufferBase<Alignment, BlockSize>::create_buffers(size_t bytes_needed) {
        std::vector<std::shared_ptr<buffer_type> > result;
        if (bytes_needed == 0) {
            return result;
        }
        size_t remaining = bytes_needed;
        while (remaining > 0) {
            auto buf = std::make_shared<buffer_type>();
            buf->reserve(BlockSize);
            result.push_back(std::move(buf));
            if (remaining > BlockSize) {
                remaining -= BlockSize;
            } else {
                remaining = 0;
            }
        }
        return result;
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(const void * TURBO_RESTRICT data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();
        const char *src = static_cast<const char *>(data);
        auto remain = size;
        do {
            auto *b = get_write_able_buffer();
            auto app_len = std::min(remain, b->capacity() - b->size());
            b->append_confident(src, app_len);
            remain -= app_len;
            src += app_len;
        } while (remain > 0);
        _total_size += size;
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::span<char> CordBufferBase<Alignment, BlockSize>::output_next() {
        buffer_type *buf = get_write_able_buffer();
        char *start = buf->data() + buf->size();
        size_t available = _write_buffer->capacity() - buf->size();
        // Pretend the whole remaining space is used immediately.
        buf->resize(_write_buffer->capacity());
        _total_size += available;
        return turbo::span<char>{start, available};
    }

    template<size_t Alignment, size_t BlockSize>
    bool CordBufferBase<Alignment, BlockSize>::output_next(void **out, size_t *size) {
        buffer_type *buf = get_write_able_buffer();
        char *start = buf->data() + buf->size();
        size_t available = _write_buffer->capacity() - buf->size();
        // Pretend the whole remaining space is used immediately.
        buf->resize(_write_buffer->capacity());
        _total_size += available;
        *out = start;
        *size = available;
        return true;
    }


    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::output_backup(int n) {
        if (n <= 0) return;
        auto to_remove = static_cast<size_t>(n);
        if (to_remove >= _total_size) {
            CordBufferBase empty;
            swap(empty);
            return;
        }
        size_t remaining = to_remove;
        while (remaining > 0) {
            BufferRef &ref = _views.back();
            buffer_type *buf = ref.buffer.get();
            size_t sz = buf->size();
            size_t take = std::min(remaining, sz);
            buf->resize(sz - take);
            if (buf->size() == 0) {
                _views.pop_back();
            }
            remaining -= take;
        }
        _total_size -= to_remove;
        _write_buffer = _views.back().buffer.get();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::pop_front(size_t n) noexcept {
        if (n == 0) return;
        if (n >= _total_size) {
            clear();
            return;
        }
        size_t remaining = n;
        while (remaining > 0) {
            BufferRef &front = _views.front();
            // Determine the effective size of the front segment (view or full buffer)
            size_t seg_size = front.view ? front.view->size() : front.buffer->size();
            if (remaining >= seg_size) {
                // Remove the entire segment
                remaining -= seg_size;
                _views.pop_front();
            } else {
                // Partially consume the front segment: adjust its view (or shrink buffer)
                if (front.view) {
                    // Create a new view that skips the consumed bytes
                    front.view = front.view->subspan(remaining);
                } else {
                    // No view; we could either create a view or adjust buffer pointer?
                    // Simplest: replace the buffer ref with a view subspan.
                    front.view = turbo::span<char>(front.buffer->data() + remaining,
                                                   front.buffer->size() - remaining);
                }
                remaining = 0;
            }
        }
        _total_size -= n;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::pop_front_buffer() noexcept {
        if (_views.empty()) {
            return;
        }
        const auto &front = _views.front();
        size_t seg_size = front.view ? front.view->size() : front.buffer->size();
        _total_size -= seg_size;
        _views.pop_front();
        if (_views.empty()) {
            _write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    const typename CordBufferBase<Alignment, BlockSize>::BufferRef &
    CordBufferBase<Alignment, BlockSize>::front_buffer() const noexcept {
        KCHECK(!_views.empty()) << "CordBufferBase::front_buffer() called on empty cord";
        return _views.front();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::pop_back(size_t n) noexcept {
        if (n == 0) return;
        if (n >= _total_size) {
            clear();
            return;
        }
        size_t remaining = n;
        while (remaining > 0) {
            BufferRef &back = _views.back();
            size_t seg_size = back.view ? back.view->size() : back.buffer->size();
            if (remaining >= seg_size) {
                // Remove the entire segment
                remaining -= seg_size;
                _views.pop_back();
            } else {
                // Partially consume the back segment: truncate its view (or buffer) from the end
                size_t new_size = seg_size - remaining;
                if (back.view) {
                    // Create a view that drops the last 'remaining' bytes
                    back.view = back.view->subspan(0, new_size);
                } else {
                    // No view; create a view covering only the first new_size bytes of the buffer
                    back.view = turbo::span<char>(back.buffer->data(), new_size);
                }
                remaining = 0;
            }
        }
        _total_size -= n;
        if (_views.empty()) {
            _write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::pop_back_buffer() noexcept {
        if (_views.empty()) {
            return;
        }
        const auto &back = _views.back();
        size_t seg_size = back.view ? back.view->size() : back.buffer->size();
        _total_size -= seg_size;
        _views.pop_back();
        if (_views.empty()) {
            _write_buffer = const_cast<buffer_type *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    const typename CordBufferBase<Alignment, BlockSize>::BufferRef &
    CordBufferBase<Alignment, BlockSize>::back_buffer() const noexcept {
        KCHECK(!_views.empty()) << "back_buffer on empty cord";
        return _views.back();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> CordBufferBase<Alignment, BlockSize>::append_by_readv(
        IOReader &reader, size_t max_limited, bool restart_block) {
        bool reach_read_zero = false;
        if (max_limited == 0) {
            return 0;
        }

        std::vector<turbo::span<char> > vecs;
        vecs.reserve(kMaxReadVSpans);
        size_t collect = 0;
        if (!restart_block) {
            auto span = output_next();
            collect += span.size();
            vecs.emplace_back(span);
        }
        std::vector<std::shared_ptr<Buffer<value_type, Alignment> > > buffers;
        if (collect < max_limited) {
            auto bsize = (max_limited - collect) / BlockSize + 1;
            size_t n;
            if (!restart_block) {
                n = std::min(bsize, kMaxReadVSpans - 1);
            } else {
                n = std::min(bsize, kMaxReadVSpans);
            }

            buffers = std::move(create_buffers(n * BlockSize));
            for (size_t i = 0; i < buffers.size() - 1; i++) {
                vecs.emplace_back(buffers[i]->data(), buffers[i]->capacity());
                collect += buffers[i]->capacity();
            }
            auto last = std::min(max_limited - collect, buffers.back()->capacity());
            collect += last;
            vecs.emplace_back(buffers.back()->data(), last);
        }

        auto r = reader.readv(vecs, collect);
        if (!r.ok()) {
            if (!restart_block) {
                output_backup(vecs.front().size());
            }
            return r;
        }
        auto rsize = r.value_or_die();
        size_t span_size = 0;
        if (!restart_block) {
            span_size = vecs.front().size();
            if (rsize <= span_size) {
                output_backup(span_size - rsize);
                return rsize;
            }
        }
        auto it = buffers.begin();
        while (span_size < rsize) {
            size_t s = std::min(rsize - span_size, (*it)->capacity());
            (*it++)->resize(s);
            span_size += s;
        }

        append(buffers).ignore_error();
        return rsize;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> CordBufferBase<Alignment, BlockSize>::append_by_read(
        IOReader &reader, size_t max_limited) {
        if (max_limited == 0) return 0ul;

        size_t total_read = 0;
        size_t remaining = max_limited;

        while (remaining > 0) {
            // Get a writable span (the whole remaining capacity of the current tail buffer)
            turbo::span<char> span = output_next();
            size_t to_read = std::min(span.size(), remaining);
            auto rspan = span.subspan(0, to_read);
            auto r = reader.read(rspan);
            if (!r.ok()) {
                // Read failed: roll back the entire span that was reserved via output_next()
                output_backup(static_cast<int>(span.size()));
                return r.status(); // propagate the error
            }
            size_t nread = r.value_or_die();
            total_read += nread;
            remaining -= nread;
            if (nread < to_read) {
                // Partial read: roll back the unused part of the span
                output_backup(static_cast<int>(to_read - nread));
                break;
            }
            // The span was completely filled; continue to next block.
        }

        return total_read;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> CordBufferBase<Alignment, BlockSize>::append_by_pread(
        IOReader &reader, size_t offset, size_t max_limited) {
        size_t total = 0;
        size_t cur_offset = offset;
        while (total < max_limited) {
            auto span = output_next();
            size_t want = std::min(span.size(), max_limited - total);
            auto rspan = span.first(want);
            auto r = reader.pread(rspan, cur_offset);
            if (!r.ok()) {
                output_backup(want);
                return r;
            }
            size_t got = r.value_or_die();
            total += got;
            cur_offset += got;
            if (got < want) {
                output_backup(want - got);
                break;
            }
        }
        return total;
    }

    /// Specialization of ContainerAppender for CordBufferBase (non‑contiguous, segmented buffer).
    template<size_t Alignment, size_t BlockSize>
    class ContainerAppender<CordBufferBase<Alignment, BlockSize> > : public Receiver {
    public:
        using cord_type = CordBufferBase<Alignment, BlockSize>;

        explicit ContainerAppender(cord_type &c) : _c(c) {
        }

        void clear() noexcept override {
            _c.clear();
        }

        turbo::Status resize(size_t n) override {
            // CordBuffer does not support arbitrary resize; ignore.
            (void) n;
            return turbo::OkStatus();
        }

        turbo::Status reserve(size_t n) override {
            // Reserve enough capacity: allocate full blocks as needed.
            (void) n;
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data && len != 0) return turbo::invalid_argument_error("nullptr");
            return _c.append(data, len);
        }

        [[nodiscard]] bool is_dynamic() const noexcept override {
            return true;
        }

        [[nodiscard]] size_t size() const noexcept override {
            return _c.size();
        }

        [[nodiscard]] size_t capacity() const noexcept override {
            return std::numeric_limits<size_t>::max();
        }

    private:
        cord_type &_c;
    };

    /// Specialization of ContainerReceiver for CordBufferBase (non‑contiguous, segmented buffer).
    template<size_t Alignment, size_t BlockSize>
    class ContainerReceiver<CordBufferBase<Alignment, BlockSize> > : public Receiver {
    public:
        using cord_type = CordBufferBase<Alignment, BlockSize>;

        ContainerReceiver() = default;

        explicit ContainerReceiver(cord_type &&c) : storage(std::move(c)) {
        }

        void clear() noexcept override {
            storage.clear();
        }

        turbo::Status resize(size_t n) override {
            // CordBuffer does not support arbitrary resize; ignore.
            TURBO_UNUSED(n);
            return turbo::OkStatus();
        }

        turbo::Status reserve(size_t n) override {
            TURBO_UNUSED(n);
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data && len != 0) return turbo::invalid_argument_error("nullptr");
            return storage.append(data, len);
        }

        [[nodiscard]] bool is_dynamic() const noexcept override { return true; }
        [[nodiscard]] size_t size() const noexcept override { return storage.size(); }

        [[nodiscard]] size_t capacity() const noexcept override {
            // Total allocated capacity = blocks * BlockSize
            return std::numeric_limits<size_t>::max();
        }

        cord_type release() { return std::move(storage); }
        const cord_type &container() const noexcept { return storage; }

    public:
        cord_type storage;
    };

    template<typename T>
    struct is_cord_buffer : std::false_type {
        static constexpr size_t alignment = 0;
        static constexpr size_t block_size = 0;
    };

    template<size_t Alignment, size_t BlockSize>
    struct is_cord_buffer<fermat::CordBufferBase<Alignment, BlockSize> > : std::true_type {
        static constexpr size_t alignment = Alignment;
        static constexpr size_t block_size = BlockSize;
    };

    template<typename T>
    inline constexpr bool is_cord_buffer_v = is_cord_buffer<T>::value;
} // namespace fermat
