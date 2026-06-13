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
#include <fermat/io/reader_writer.h>
#include <fermat/container/receiver.h>
#include <fermat/container/deque.h>
#include <turbo/log/logging.h>
#include <memory>
#include <new>
#include <deque>

/// 10g may enough for a single CordBuffer
#ifndef MAX_SINGLE_CORD_SIZE
#define MAX_SINGLE_CORD_SIZE (10UL * 1024UL * 1024UL * 1024UL)
#endif

namespace fermat {
    template<size_t Alignment>
    struct BufferView {
        struct Range {
            uint32_t offset{0};
            uint32_t length{0};
        };

        static constexpr size_t kTlsCacheCapacity = 8192;

        BufferView() = default;

        ~BufferView() = default;

        BufferView(const BufferView &rhs) = default;

        BufferView &operator=(const BufferView &rhs) = default;

        BufferView(BufferView &&rhs) noexcept {
            buffer = std::move(rhs.buffer);
            range = rhs.range;
            rhs.range = {};
            rhs.buffer.clear();
        }

        BufferView &operator=(BufferView &&rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }
            buffer = std::move(rhs.buffer);
            range = rhs.range;
            rhs.range = {};
            rhs.buffer.reset();
            return *this;
        }

        static BufferView create_write_able(size_t n) {
            BufferView ref;
            ref.buffer.resize_uninitialized(n);

            return ref;
        }

        static BufferView setup_write_able(Buffer<char, Alignment>  &&b) {
            BufferView ref;
            ref.buffer = std::move(b);
            return std::move(ref);
        }

        static BufferView setup_write_able(Buffer<char, Alignment>  &&b, uint32_t off, uint32_t len) {
            BufferView ref;
            ref.buffer = std::move(b);
            ref.range.length = len;
            ref.range.offset = off;
            return std::move(ref);
        }

        [[nodiscard]] size_t write_able() const {
            return buffer.size() - range.length - range.offset;
        }

        [[nodiscard]] size_t capacity() const {
            return buffer.size() - range.offset;
        }

        [[nodiscard]] size_t size() const {
            return range.length;
        }

        [[nodiscard]] const char *data() const {
            return buffer.data() + range.offset;
        }

        [[nodiscard]] size_t offset() const {
            return range.offset;
        }

        size_t append(const void *data, size_t size) {
            auto n = std::min(write_able(), size);
            std::memcpy(buffer.data() + range.offset + range.length, data, n);
            range.length += n;
            return n;
        }

        bool borrow(void **out, int *size) {
            if (TURBO_UNLIKELY(buffer.use_count() > 1)) {
                KLOG(INFO) << buffer.use_count();
                return false;
            }

            *size = buffer.size() - range.length - range.offset;
            if (TURBO_UNLIKELY(*size == 0)) {
                KLOG(INFO) << "*size:" << *size;
                return false;
            }
            *out = buffer.data() + range.offset + range.length;
            range.length += *size;
            return true;
        }

        size_t backup(size_t size) {
            KCHECK(buffer.use_count() == 1);
            if (size > range.length) {
                auto ret = range.length;
                range.length = 0;
                return ret;
            }
            range.length -= size;
            return size;
        }

        size_t pop_front(size_t n) {
            if (range.length <= n) {
                auto ret = range.length;
                range.offset += range.length;
                range.length = 0;
                return ret;
            }
            range.length -= n;
            range.offset += n;
            return n;
        }

        size_t pop_back(size_t n) {
            if (range.length <= n) {
                auto ret = range.length;
                range.length = 0;
                return ret;
            }
            range.length -= n;
            return n;
        }

        [[nodiscard]] bool is_unique() const {
            return buffer.use_count() == 1;
        }

        Buffer<char, Alignment> buffer;
        Range range;
    };

    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class BufferQueueBase {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;
        static constexpr size_t kMaxReadVSpans = 32;
        static constexpr size_t kMaxSingleCordSize = MAX_SINGLE_CORD_SIZE;

    public:
        using value_type = char;
        using buffer_type = Buffer<value_type, Alignment>;

        using vector_type = Deque<BufferView<Alignment> >;
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
                if (_view_begin != _view.end()) {
                    ++_view_begin;
                }
                ++_index_read;

                if (_index_read >= _cord->size()) {
                    return *this;
                }


                if (_view_begin == _view.end()) {
                    ++_cur;
                    _view = turbo::span<value_type>(_cur->buffer.data() + _cur->range.offset, _cur->range.length);
                    _view_begin = _view.begin();
                }

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
            friend class BufferQueueBase;

            CordIterator(const BufferQueueBase *cord) : _cord(cord), _cur(_cord->buffer_begin()) {
                if (_cur != _cord->buffer_end()) {
                    _view = turbo::span<char>(_cur->buffer.data() + _cur->range.offset, _cur->range.length);
                    _view_begin = _view.begin();
                }
            }

            CordIterator(const BufferQueueBase *cord, bool) : _cord(cord), _cur(_cord->buffer_end()), _view(),
                                                             _view_begin(_view.end()), _index_read(_cord->size()) {
            }

            size_type has_read() const {
                return _index_read + 1;
            }

            size_type remaining() const {
                return _cord->size() - _index_read - 1;
            }

        private:
            const BufferQueueBase *_cord;
            const_buffer_iterator _cur;
            turbo::span<value_type> _view;
            turbo::span<value_type>::const_iterator _view_begin;
            size_type _index_read{0};
        };

        using const_iterator = CordIterator;

        /// ZeroCopyInputStream adapter for BufferQueueBase.
        /// Provides sequential read access to the underlying concatenated buffers,
        /// respecting any per‑segment views (sub‑ranges).
        class InputStream {
        public:
            explicit InputStream(const BufferQueueBase *cord)
                : _cord(cord) {
                _skip_empty();
            }

            /// Obtains the next chunk of data.
            /// The chunk corresponds to the current segment (respecting any range).
            /// After a successful call, the caller may read up to *size bytes from *data.
            /// @param data  Output pointer to the beginning of the chunk.
            /// @param size  Output size of the chunk in bytes.
            /// @return true if there is more data, false at end of stream.
            bool next(const void **data, int *size) {
                // Find a segment with remaining data.
                while (_index < _cord->_views.size()) {
                    const auto &ref = _cord->_views[_index];
                    // Obtain the effective span (range if present, otherwise full buffer)
                    turbo::span<char> span = turbo::span<char>(ref.buffer.data() + ref.range.offset, ref.range.length);
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
                    turbo::span<char> span = turbo::span<char>(ref.buffer.data() + ref.range.offset, ref.range.length);
                    _offset = span.size() - static_cast<size_t>(count);
                    _index = 0; // stay at the first segment
                } else {
                    // Last chunk came from the segment at index (_index - 1).
                    --_index;
                    const auto &ref = _cord->_views[_index];
                    turbo::span<char> span = turbo::span<char>(ref.buffer.data() + ref.range.offset, ref.range.length);
                    _offset = span.size() - static_cast<size_t>(count);
                }
                _byte_count -= count;
                _last_chunk_size -= count; // After back_up, the "last chunk" is invalidated.
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
                auto remaining = static_cast<size_t>(count);
                size_t idx = _index;
                size_t off = _offset;
                while (remaining > 0) {
                    const auto &ref = _cord->_views[idx];
                    turbo::span<char> span = turbo::span<char>(ref.buffer.data() + ref.range.offset, ref.range.length);
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
                // After skip, the last chunk is no longer valid.
                _last_chunk_size = 0;
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
                    turbo::span<char> span = turbo::span<char>(ref.buffer.data() + ref.range.offset, ref.range.length);
                    if (!span.empty()) break;
                    ++_index;
                }
                _offset = 0;
            }

            const BufferQueueBase *_cord;
            /// Current segment index.
            size_t _index{0};
            /// Offset within the current segment.
            size_t _offset{0};
            /// Total bytes consumed.
            int64_t _byte_count{0};
            /// Size of the last chunk returned by next() (for back_up validation).
            size_t _last_chunk_size{0};
        };

    public:
        BufferQueueBase() = default;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        BufferQueueBase(BufferQueueBase &&rhs) noexcept
            : _views(std::move(rhs._views)),
              _total_size(rhs._total_size),
              _write_buffer(_views.empty() ? const_cast<BufferView<Alignment> *>(&kZeroBuffer) : &_views.back()) {
            rhs._views.clear();
            rhs._total_size = 0;
            rhs._write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }

        ~BufferQueueBase() = default;

        const_buffer_iterator buffer_begin() const {
            return _views.begin();
        }

        const BufferView<Alignment> *buffer_at(size_t idx) const {
            if (idx < _views.size()) {
                return &_views.at(idx);
            }
            return nullptr;
        }

        BufferView<Alignment> *buffer_at(size_t idx) {
            if (idx < _views.size()) {
                return &_views.at(idx);
            }
            return nullptr;
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

        [[nodiscard]] size_t buffer_count() const;

        turbo::Status append_reference(const BufferView<Alignment> &ref);

        turbo::Status append_reference(BufferView<Alignment> &&ref);

        turbo::Status append_reference(const std::vector<BufferView<Alignment> > &refs);

        turbo::Status append_reference(std::vector<BufferView<Alignment> > &&refs);

        turbo::Status prepend_reference(const BufferView<Alignment> &ref);

        turbo::Status prepend_reference(BufferView<Alignment> &&ref);

        turbo::Status prepend_reference(const std::vector<BufferView<Alignment> > &refs);

        turbo::Status prepend_reference(std::vector<BufferView<Alignment> > &&refs);

        turbo::Status append_writeable(BufferView<Alignment> &&ref);

        turbo::Status append_writeable(std::vector<BufferView<Alignment> > &&refs);

        /// Creates empty buffers whose total capacity is at least `bytes_needed`.
        /// New buffers use `BlockSize` as the default reserve granularity (recommended, not required
        /// for externally appended segments, which may use any aligned pool tier capacity).
        static std::vector<BufferView<Alignment> > create_buffers(size_t bytes_needed);

        /// merge blcoks in one
        static BufferView<Alignment> create_big_buffer(size_t bytes_needed);

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
        bool output_next(void **out, int *size);

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

        /// Appends all segments from another BufferQueueBase to this one.
        /// The segments are copied (shared ownership) – no deep copy of buffer data.
        /// @param rhs The source cord.
        void append(const BufferQueueBase &rhs);

        void append(BufferQueueBase &&rhs);

        void prepend(const BufferQueueBase &rhs);

        /// Moves all segments from rhs to the front of this cord.
        /// After the operation, rhs is left empty.
        /// The writable buffer is set to the new tail (the last segment of rhs).
        void prepend(BufferQueueBase &&rhs);


        BufferQueueBase copy() const;
        /// Uses copy-and-swap. For an independent copy of bytes, use copy().
        /// @param rhs The source cord.
        /// @return Reference to this object.
        BufferQueueBase &operator=(const BufferQueueBase &rhs);

        // Move assignment: release current resources, then transfer from rhs.
        BufferQueueBase &operator=(BufferQueueBase &&rhs) noexcept;

        BufferQueueBase &operator=(std::string_view str);

        BufferQueueBase &operator=(turbo::span<char> span);

        BufferQueueBase &operator=(const std::vector<char> &data);

        template<size_t BA>
        BufferQueueBase &operator=(const Buffer<char, BA> &data);

        BufferQueueBase &operator<<(std::string_view str);

        BufferQueueBase &operator<<(turbo::span<char> span);

        BufferQueueBase &operator<<(const std::vector<char> &data);

        template<size_t BA>
        BufferQueueBase &operator<<(const Buffer<char, BA> &data);

        template<typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
        BufferQueueBase &operator<<(T v);


        /// Swaps the contents of this BufferQueueBase with another.
        /// @param other The other buffer to swap with.
        void swap(BufferQueueBase &other) noexcept;

        void clear() noexcept;

        void pop_front(size_t n) noexcept;

        void pop_front_buffer() noexcept;

        const BufferView<Alignment> &front_buffer() const noexcept;

        void pop_back(size_t n) noexcept;

        void pop_back_buffer() noexcept;

        const BufferView<Alignment> &back_buffer() const noexcept;

        turbo::Status flatten(Receiver &recv) const;

        template<typename String = std::string, std::enable_if_t<is_contiguous_string_receiver<String>::value, int> = 0>
        String flatten() const;

    protected:
        BufferView<Alignment> *get_write_able_buffer();

    protected:
        static const BufferView<Alignment> kZeroBuffer;
        mutable BufferView<Alignment> *_write_buffer{const_cast<BufferView<Alignment> *>(&kZeroBuffer)};
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
    const BufferView<Alignment> BufferQueueBase<Alignment,
                BlockSize>::kZeroBuffer = []() {
                BufferView<Alignment> r;
                return r;
            }();


    template<size_t Alignment, size_t BlockSize>
    inline size_t BufferQueueBase<Alignment, BlockSize>::block_size() const {
        return BlockSize;
    }


    /// @brief Total logical bytes in the BufferQueueBase.
    template<size_t Alignment, size_t BlockSize>
    inline size_t BufferQueueBase<Alignment, BlockSize>::size() const {
        return _total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    inline size_t BufferQueueBase<Alignment, BlockSize>::buffer_count() const {
        return _views.size();
    }


    template<size_t Alignment, size_t BlockSize>
    inline size_t BufferQueueBase<Alignment, BlockSize>::alignment() const {
        return Alignment;
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append(std::string_view data) {
        return append(data.data(), data.size());
    }

    template<size_t Alignment, size_t BlockSize>
    BufferView<Alignment> *BufferQueueBase<Alignment, BlockSize>::get_write_able_buffer() {
        if (_write_buffer->write_able()) {
            return _write_buffer;
        }
        auto ref = BufferView<Alignment>::create_write_able(BlockSize);
        _views.push_back(std::move(ref));
        _write_buffer = &_views.back();
        return _write_buffer;
    }


    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::swap(BufferQueueBase &other) noexcept {
        using std::swap;
        swap(_views, other._views);
        swap(_total_size, other._total_size);
        swap(_write_buffer, other._write_buffer);
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::flatten(Receiver &recv) const {
        TURBO_RETURN_NOT_OK(recv.reserve(_total_size));
        for (auto &ref: _views) {
            TURBO_RETURN_NOT_OK(recv.append(ref.data(), ref.size()));
        }
        return turbo::OkStatus();
    }


    template<size_t Alignment, size_t BlockSize>
    template<typename String, std::enable_if_t<is_contiguous_string_receiver<String>::value, int> >
    String BufferQueueBase<Alignment, BlockSize>::flatten() const {
        String result;
        result.reserve(_total_size);
        for (auto &ref: _views) {
            result.append(ref.data(), ref.size());
        }
        return std::move(result);
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append_reference(const BufferView<Alignment> &ref) {
        _views.push_back(ref);
        _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        _total_size += ref.size();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append_reference(BufferView<Alignment> &&ref) {
        _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        _total_size += ref.size();
        _views.push_back(std::move(ref));
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment,
        BlockSize>::append_reference(const std::vector<BufferView<Alignment> > &refs) {
        for (auto &ref: refs) {
            _views.push_back(ref);
            _total_size += ref.size();
        }
        _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append_reference(std::vector<BufferView<Alignment> > &&refs) {
        for (auto &ref: refs) {
            _total_size += ref.size();
            _views.push_back(std::move(ref));
        }
        _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::prepend_reference(const BufferView<Alignment> &ref) {
        _views.push_front(ref);
        _total_size += ref.size();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::prepend_reference(BufferView<Alignment> &&ref) {
        _total_size += ref.size();
        _views.push_front(std::move(ref));
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::prepend_reference(
        const std::vector<BufferView<Alignment> > &refs) {
        for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
            _views.push_front(*it);
            _total_size += (*it).size();
        }
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::prepend_reference(std::vector<BufferView<Alignment> > &&refs) {
        for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
            _total_size += (*it).size();
            _views.push_front(std::move(*it));
        }
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append_writeable(BufferView<Alignment> &&ref) {
        KCHECK(ref.is_unique());
        if (ref.size() == 0) {
            return turbo::OkStatus();
        }
        _total_size += ref.size();
        _views.push_back(std::move(ref));
        _write_buffer = &_views.back();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append_writeable(std::vector<BufferView<Alignment> > &&refs) {
        size_t added = 0;
        for (auto &ref: refs) {
            if (ref.size() == 0) {
                break;
            }
            added += ref.size();
            _views.push_back(std::move(ref));
        }
        if (added) {
            _write_buffer = &_views.back();
        }
        _total_size += added;
        return turbo::OkStatus();
    }


    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::append(const BufferQueueBase &rhs) {
        for (const auto &ref: rhs._views) {
            BufferView<Alignment> new_ref;
            new_ref.buffer = ref.buffer;
            new_ref.range = ref.range;
            _views.push_back(std::move(new_ref));
        }
        _total_size += rhs._total_size;
        _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::append(BufferQueueBase &&rhs) {
        if (rhs._views.empty()) return;
        for (auto &it: rhs._views) {
            _views.push_back(std::move(it));
        }
        _total_size += rhs._total_size;
        rhs._views.clear();
        rhs._total_size = 0;
        rhs._write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        const auto &last = _views.back();
        if (_views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::prepend(const BufferQueueBase &rhs) {
        if (rhs._views.empty()) return;
        // Insert copies of rhs segments at the front in reverse order to preserve original order.
        for (auto it = rhs._views.rbegin(); it != rhs._views.rend(); ++it) {
            const auto &ref = *it;
            BufferView<Alignment> new_ref;
            new_ref.buffer = ref.buffer;
            new_ref.range = ref.range;
            _views.push_front(std::move(new_ref));
        }
        _total_size += rhs._total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::prepend(BufferQueueBase &&rhs) {
        if (rhs._views.empty()) return;
        // Insert segments at the front in reverse order to maintain the original order of rhs.
        bool empty = _views.empty();
        for (auto it = rhs._views.rbegin(); it != rhs._views.rend(); ++it) {
            _views.push_front(std::move(*it));
        }
        _total_size += rhs._total_size;
        // Clear rhs to a valid empty state.
        rhs._views.clear();
        rhs._total_size = 0;

        if (empty && _views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            rhs._write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
    }


    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> BufferQueueBase<Alignment, BlockSize>::copy() const {
        BufferQueueBase result;
        for (const auto &ref: _views) {
            const char *data = ref.buffer.data() + ref.range.offset;
            result.append(data, ref.range.length).ignore_error();
        }
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator=(const BufferQueueBase &rhs) {
        if (this != &rhs) {
            BufferQueueBase temp = rhs.copy();
            swap(temp);
        }
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator=(BufferQueueBase &&rhs) noexcept {
        if (this != &rhs) {
            // Transfer ownership of the views container.
            _views = std::move(rhs._views);
            // Transfer total size and clear rhs.
            _total_size = rhs._total_size;
            rhs._total_size = 0;
            rhs._views.clear();
            // Transfer writable buffer pointer and reset rhs to the sentinel.
            _write_buffer = rhs._write_buffer;
            rhs._write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
        return *this;
    }

    /// Assigns the cord to a copy of the data from a string range.
    /// The cord is first cleared, then the data is appended.
    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator=(std::string_view str) {
        clear();
        append(str).ignore_error();
        return *this;
    }

    /// Assigns the cord to a copy of the data from a span of characters.
    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator=(turbo::span<char> span) {
        clear();
        append(span.data(), span.size()).ignore_error();
        return *this;
    }

    /// Assigns the cord to a copy of the data from a vector of characters.
    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator=(const std::vector<char> &data) {
        clear();
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Assigns the cord to a copy of the data from a Buffer.
    /// The entire buffer's content (from its beginning to its current size) is copied.
    /// The buffer's range (if any) is ignored; only the actual data is copied.
    template<size_t Alignment, size_t BlockSize>
    template<size_t BA>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator=(const Buffer<char, BA> &data) {
        clear();
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends a string range to the cord.
    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator<<(std::string_view str) {
        append(str).ignore_error();
        return *this;
    }

    /// Appends a span of characters.
    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator<<(turbo::span<char> span) {
        append(span.data(), span.size()).ignore_error();
        return *this;
    }

    /// Appends the content of a vector.
    template<size_t Alignment, size_t BlockSize>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator<<(const std::vector<char> &data) {
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends the content of a Buffer.
    template<size_t Alignment, size_t BlockSize>
    template<size_t BA>
    BufferQueueBase<Alignment, BlockSize> &
    BufferQueueBase<Alignment, BlockSize>::operator<<(const Buffer<char, BA> &data) {
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends an arithmetic value formatted as a string.
    template<size_t Alignment, size_t BlockSize>
    template<typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, int> >
    BufferQueueBase<Alignment, BlockSize> &BufferQueueBase<Alignment, BlockSize>::operator<<(T v) {
        cat(v);
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::clear() noexcept {
        _views.clear();
        _total_size = 0;
        _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
    }


    template<size_t Alignment, size_t BlockSize>
    std::vector<BufferView<Alignment> >
    BufferQueueBase<Alignment, BlockSize>::create_buffers(size_t bytes_needed) {
        if (bytes_needed >= kMaxSingleCordSize) {
            throw std::out_of_range("BufferQueueBase::create_buffers(size_t bytes_needed)");
        }
        std::vector<BufferView<Alignment> > result;
        if (bytes_needed == 0) {
            return result;
        }
        size_t remaining = bytes_needed;
        while (remaining > 0) {
            auto buf = BufferView<Alignment>::create_write_able(BlockSize);
            result.push_back(std::move(buf));
            if (remaining > BlockSize) {
                remaining -= BlockSize;
            } else {
                remaining = 0;
            }
        }
        return std::move(result);
    }


    template<size_t Alignment, size_t BlockSize>
    BufferView<Alignment>
    BufferQueueBase<Alignment, BlockSize>::create_big_buffer(size_t bytes_needed) {
        if (bytes_needed >= kMaxSingleCordSize) {
            throw std::out_of_range("BufferQueueBase::create_buffers(size_t bytes_needed)");
        }
        BufferView<Alignment> result;
        if (bytes_needed == 0) {
            return result;
        }
        auto n = (bytes_needed + BlockSize - 1) / BlockSize;
        result = BufferView<Alignment>::create_write_able(BlockSize * n);
        return std::move(result);
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status BufferQueueBase<Alignment, BlockSize>::append(const void * TURBO_RESTRICT data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();
        const char *src = static_cast<const char *>(data);
        auto remain = size;
        do {
            auto *b = get_write_able_buffer();
            auto app_len = b->append(src, remain);
            remain -= app_len;
            src += app_len;
        } while (remain > 0);
        _total_size += size;
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::span<char> BufferQueueBase<Alignment, BlockSize>::output_next() {
        auto *buf = get_write_able_buffer();
        void *out;
        int available;
        auto r = buf->borrow(&out, &available);
        if (!r) {
            return {};
        }
        _total_size += available;
        return turbo::span<char>{reinterpret_cast<char *>(out), static_cast<size_t>(available)};
    }

    template<size_t Alignment, size_t BlockSize>
    bool BufferQueueBase<Alignment, BlockSize>::output_next(void **out, int *size) {
        auto *buf = get_write_able_buffer();
        auto r = buf->borrow(out, size);
        if (!r) return false;
        _total_size += *size;
        return true;
    }


    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::output_backup(int n) {
        if (n <= 0) return;
        auto to_remove = static_cast<size_t>(n);
        if (to_remove >= _total_size) {
            BufferQueueBase empty;
            swap(empty);
            return;
        }
        size_t remaining = to_remove;
        while (remaining > 0) {
            BufferView<Alignment> &ref = _views.back();
            remaining -= ref.backup(remaining);
            if (ref.size() == 0) {
                _views.pop_back();
            }
        }
        _total_size -= to_remove;
        if (!_views.empty()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::pop_front(size_t n) noexcept {
        if (n == 0) return;
        if (n >= _total_size) {
            clear();
            return;
        }
        size_t remaining = n;
        while (remaining > 0) {
            BufferView<Alignment> &front = _views.front();
            // Determine the effective size of the front segment (range or full buffer)
            remaining -= front.pop_front(remaining);
            if (front.size() == 0) {
                _views.pop_front();
            }
        }
        _total_size -= n;
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::pop_front_buffer() noexcept {
        if (_views.empty()) {
            return;
        }
        const auto &front = _views.front();
        _total_size -= front.range.length;
        _views.pop_front();
        if (_views.empty()) {
            _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    const BufferView<Alignment> &
    BufferQueueBase<Alignment, BlockSize>::front_buffer() const noexcept {
        KCHECK(!_views.empty()) << "BufferQueueBase::front_buffer() called on empty cord";
        return _views.front();
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::pop_back(size_t n) noexcept {
        if (n == 0) return;
        if (n >= _total_size) {
            clear();
            return;
        }
        size_t remaining = n;
        while (remaining > 0) {
            BufferView<Alignment> &back = _views.back();
            if (remaining >= back.range.length) {
                // Remove the entire segment
                remaining -= back.range.length;
                _views.pop_back();
            } else {
                back.range.length -= remaining;
                remaining = 0;
            }
        }
        _total_size -= n;
        if (!_views.empty() && _views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void BufferQueueBase<Alignment, BlockSize>::pop_back_buffer() noexcept {
        if (_views.empty()) {
            return;
        }
        const auto &back = _views.back();
        _total_size -= back.range.length;
        _views.pop_back();
        if (!_views.empty() && _views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferView<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    const BufferView<Alignment> &
    BufferQueueBase<Alignment, BlockSize>::back_buffer() const noexcept {
        KCHECK(!_views.empty()) << "back_buffer on empty cord";
        return _views.back();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> BufferQueueBase<Alignment, BlockSize>::append_by_readv(
        IOReader &reader, size_t max_limited, bool restart_block) {
        bool reach_read_zero = false;
        if (max_limited == 0) {
            return 0;
        }

        std::vector<IOVec> vecs;
        vecs.reserve(kMaxReadVSpans);
        size_t collect = 0;
        if (!restart_block) {
            auto span = output_next();
            collect += span.size();
            IOVec vec;
            vec.iov_base = span.data();
            vec.iov_len = span.size();
            vecs.push_back(vec);
        }
        std::vector<BufferView<Alignment> > buffers;
        if (collect < max_limited) {
            auto bsize = (max_limited - collect) / BlockSize + 1;
            size_t n;
            if (!restart_block) {
                n = std::min(bsize, kMaxReadVSpans - 1);
            } else {
                n = std::min(bsize, kMaxReadVSpans);
            }

            for (size_t i = 0; i < n - 1; i++) {
                auto ref = BufferView<Alignment>::create_write_able(BlockSize);
                vecs.emplace_back(ref.buffer.data(), ref.capacity());
                collect += ref.capacity();
                buffers.push_back(std::move(ref));
            }
            auto ref = BufferView<Alignment>::create_write_able(BlockSize);
            auto last = std::min(max_limited - collect, ref.capacity());
            collect += last;
            vecs.emplace_back(ref.buffer.data(), last);
            buffers.push_back(std::move(ref));
        }

        auto r = reader.readv(vecs, collect);
        if (!r.ok()) {
            if (!restart_block) {
                output_backup(vecs.front().iov_len);
            }
            return r;
        }
        auto rsize = r.value_or_die();
        size_t span_size = 0;
        if (!restart_block) {
            span_size = vecs.front().iov_len;
            if (rsize <= span_size) {
                output_backup(span_size - rsize);
                return rsize;
            }
        }
        auto it = buffers.begin();
        while (span_size < rsize) {
            size_t s = std::min(rsize - span_size, it->capacity());
            span_size += s;
            it->range.length = s;
            ++it;
        }

        append_writeable(std::move(buffers)).ignore_error();
        return rsize;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> BufferQueueBase<Alignment, BlockSize>::append_by_read(
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
            if (nread < span.size()) {
                // Partial read: roll back the unused part of the span
                output_backup(static_cast<int>(span.size() - nread));
                break;
            }
            // The span was completely filled; continue to next block.
        }

        return total_read;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<size_t> BufferQueueBase<Alignment, BlockSize>::append_by_pread(
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
            if (got < span.size()) {
                output_backup(span.size() - got);
                break;
            }
        }
        return total;
    }

    /// Specialization of ContainerAppender for BufferQueueBase (non‑contiguous, segmented buffer).
    template<size_t Alignment, size_t BlockSize>
    class ContainerAppender<BufferQueueBase<Alignment, BlockSize> > : public Receiver {
    public:
        using cord_type = BufferQueueBase<Alignment, BlockSize>;

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

    /// Specialization of ContainerReceiver for BufferQueueBase (non‑contiguous, segmented buffer).
    template<size_t Alignment, size_t BlockSize>
    class ContainerReceiver<BufferQueueBase<Alignment, BlockSize> > : public Receiver {
    public:
        using cord_type = BufferQueueBase<Alignment, BlockSize>;

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
    struct is_cord_buffer<fermat::BufferQueueBase<Alignment, BlockSize> > : std::true_type {
        static constexpr size_t alignment = Alignment;
        static constexpr size_t block_size = BlockSize;
    };

    template<typename T>
    inline constexpr bool is_cord_buffer_v = is_cord_buffer<T>::value;
} // namespace fermat
