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
#include <fermat/container/buffer_queue.h>
#include <fermat/container/deque.h>
#include <turbo/log/logging.h>
#include <memory>
#include <new>
#include <deque>
#include <build/kmpkg_installed/x64-linux/include/absl/strings/string_view.h>

/// 10g may enough for a single CordBuffer
#ifndef MAX_SINGLE_CORD_SIZE
#define MAX_SINGLE_CORD_SIZE (10UL * 1024UL * 1024UL * 1024UL)
#endif

namespace fermat {
    template<size_t Alignment>
    struct BufferRef {
        struct Range {
            uint32_t offset{0};
            uint32_t length{0};
        };

        static constexpr size_t kTlsCacheCapacity = 8192;

        BufferRef() = default;

        ~BufferRef() = default;

        BufferRef(const BufferRef &rhs) = default;

        BufferRef &operator=(const BufferRef &rhs) = default;

        BufferRef(BufferRef &&rhs) noexcept {
            buffer = std::move(rhs.buffer);
            range = rhs.range;
            rhs.range = {};
            rhs.buffer.reset();
        }

        BufferRef &operator=(BufferRef &&rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }
            buffer = std::move(rhs.buffer);
            range = rhs.range;
            rhs.range = {};
            rhs.buffer.reset();
            return *this;
        }

        static BufferRef create_write_able(size_t n) {
            BufferRef ref;
            ref.buffer = std::make_shared<Buffer<char, Alignment> >();
            ref.buffer->resize_uninitialized(n);

            return ref;
        }

        static BufferRef setup_write_able(std::shared_ptr<Buffer<char, Alignment> > &&b) {
            BufferRef ref;
            ref.buffer = std::move(b);
            KCHECK(ref.buffer.use_count() == 1);
            return std::move(ref);
        }

        static BufferRef setup_write_able(std::shared_ptr<Buffer<char, Alignment> > &&b, uint32_t off, uint32_t len) {
            BufferRef ref;
            ref.buffer = std::move(b);
            ref.range.length = len;
            ref.range.offset = off;
            KCHECK(ref.buffer.use_count() == 1);
            return std::move(ref);
        }


        static BufferRef reference(const std::shared_ptr<Buffer<char, Alignment> > &b, uint32_t off, uint32_t len) {
            BufferRef ref;
            ref.buffer = b;
            ref.range.length = len;
            ref.range.offset = off;
            return std::move(ref);
        }


        static BufferRef assign(std::shared_ptr<Buffer<char, Alignment> > &&b, uint32_t off, uint32_t len) {
            BufferRef ref;
            ref.buffer = std::move(b);
            ref.range.length = len;
            ref.range.offset = off;
            return std::move(ref);
        }

        [[nodiscard]] size_t write_able() const {
            /*
            if (TURBO_UNLIKELY(buffer.use_count() > 1)) {
                return 0;
            }*/
            return buffer->size() - range.length - range.offset;
        }

        [[nodiscard]] size_t capacity() const {
            return buffer->size() - range.offset;
        }

        [[nodiscard]] size_t size() const {
            return range.length;
        }

        [[nodiscard]] const char *data() const {
            return buffer->data() + range.offset;
        }

        [[nodiscard]] size_t offset() const {
            return range.offset;
        }

        size_t append(const void *data, size_t size) {
            auto n = std::min(write_able(), size);
            std::memcpy(buffer->data() + range.offset + range.length, data, n);
            range.length += n;
            return n;
        }

        bool borrow(void **out, int *size) {
            if (TURBO_UNLIKELY(buffer.use_count() > 1)) {
                KLOG(INFO) << buffer.use_count();
                return false;
            }

            *size = buffer->size() - range.length - range.offset;
            if (TURBO_UNLIKELY(*size == 0)) {
                KLOG(INFO) << "*size:" << *size;
                return false;
            }
            *out = buffer->data() + range.offset + range.length;
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

        std::shared_ptr<Buffer<char, Alignment> > buffer;
        Range range;
    };

    template<size_t Alignment = 64, size_t BlockSize = 4096>
    class CordBufferBase {
    public:
        static_assert(Alignment == 0 || ((Alignment & (Alignment - 1)) == 0), "Alignment must be zero or power of 2");

        static constexpr size_t kBlockSize = BlockSize;
        static constexpr size_t kAlignment = Alignment;
        static constexpr size_t kMaxReadVSpans = 32;
        static constexpr size_t kMaxSingleCordSize = MAX_SINGLE_CORD_SIZE;

    public:
        using value_type = char;
        using buffer_type = Buffer<value_type, Alignment>;

        using vector_type = Deque<BufferRef<Alignment> >;
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
                    _view = turbo::span<value_type>(_cur->buffer->data() + _cur->range.offset, _cur->range.length);
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
            friend class CordBufferBase;

            CordIterator(const CordBufferBase *cord) : _cord(cord), _cur(_cord->buffer_begin()) {
                if (_cur != _cord->buffer_end()) {
                    _view = turbo::span<char>(_cur->buffer->data() + _cur->range.offset, _cur->range.length);
                    _view_begin = _view.begin();
                }
            }

            CordIterator(const CordBufferBase *cord, bool) : _cord(cord), _cur(_cord->buffer_end()), _view(),
                                                             _view_begin(_view.end()), _index_read(_cord->size()) {
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

        /// Result of split(): shared head and tail sub-cords.
        struct SplitView {
            CordBufferBase head;
            CordBufferBase tail;
        };

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
                    turbo::span<char> span = turbo::span<char>(ref.buffer->data() + ref.range.offset, ref.range.length);
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
                    turbo::span<char> span = turbo::span<char>(ref.buffer->data() + ref.range.offset, ref.range.length);
                    _offset = span.size() - static_cast<size_t>(count);
                    _index = 0; // stay at the first segment
                } else {
                    // Last chunk came from the segment at index (_index - 1).
                    --_index;
                    const auto &ref = _cord->_views[_index];
                    turbo::span<char> span = turbo::span<char>(ref.buffer->data() + ref.range.offset, ref.range.length);
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
                    turbo::span<char> span = turbo::span<char>(ref.buffer->data() + ref.range.offset, ref.range.length);
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
                    turbo::span<char> span = turbo::span<char>(ref.buffer->data() + ref.range.offset, ref.range.length);
                    if (!span.empty()) break;
                    ++_index;
                }
                _offset = 0;
            }

            const CordBufferBase *_cord;
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
        CordBufferBase() = default;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        CordBufferBase(CordBufferBase &&rhs) noexcept {
            _total_size = rhs._total_size;
            _views = std::move(rhs._views);
            _update_write_buffer();
            rhs._total_size = 0;
            rhs._write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }

        ~CordBufferBase() = default;

        const_buffer_iterator buffer_begin() const {
            return _views.begin();
        }

        const BufferRef<Alignment> *buffer_at(size_t idx) const {
            if (idx < _views.size()) {
                return &_views.at(idx);
            }
            return nullptr;
        }

        BufferRef<Alignment> *buffer_at(size_t idx) {
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

        turbo::Status append_reference(const BufferRef<Alignment> &ref);

        turbo::Status append_reference(BufferRef<Alignment> &&ref);

        turbo::Status append_reference(const std::vector<BufferRef<Alignment> > &refs);

        turbo::Status append_reference(std::vector<BufferRef<Alignment> > &&refs);

        turbo::Status prepend_reference(const BufferRef<Alignment> &ref);

        turbo::Status prepend_reference(BufferRef<Alignment> &&ref);

        turbo::Status prepend_reference(const std::vector<BufferRef<Alignment> > &refs);

        turbo::Status prepend_reference(std::vector<BufferRef<Alignment> > &&refs);

        turbo::Status append_writeable(BufferRef<Alignment> &&ref);

        turbo::Status append_writeable(std::vector<BufferRef<Alignment> > &&refs);

        turbo::Status append_buffer(const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset,
                                    size_t length);

        turbo::Status append_buffer(std::shared_ptr<Buffer<value_type, Alignment> > &&buf, size_t offset,
                                    size_t length);

        turbo::Status prepend_buffer(const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset,
                                     size_t length);

        turbo::Status prepend_buffer(std::shared_ptr<Buffer<value_type, Alignment> > &&buf, size_t offset,
                                     size_t length);

        /// Creates empty buffers whose total capacity is at least `bytes_needed`.
        /// New buffers use `BlockSize` as the default reserve granularity (recommended, not required
        /// for externally appended segments, which may use any aligned pool tier capacity).
        static std::vector<BufferRef<Alignment> > create_buffers(size_t bytes_needed);

        /// merge blcoks in one
        static BufferRef<Alignment> create_big_buffer(size_t bytes_needed);

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

        /// Assignment shares segment storage with @p rhs (same as share()), not a deep copy.
        /// Uses copy-and-swap. For an independent copy of bytes, use copy().
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

        const BufferRef<Alignment> &front_buffer() const noexcept;

        void pop_back(size_t n) noexcept;

        void pop_back_buffer() noexcept;

        const BufferRef<Alignment> &back_buffer() const noexcept;

        turbo::Status flatten(Receiver &recv) const;

        template<typename String = std::string, std::enable_if_t<is_contiguous_string_receiver<String>::value, int> = 0>
        String flatten() const;

        /// @name Range views and slicing (byte offsets are logical, spanning all segments).
        /// @{

        /// Returns a new cord sharing [offset, offset + length). Does not modify *this.
        [[nodiscard]] CordBufferBase share_range(size_t offset, size_t length) const;

        /// Returns a new cord sharing [offset, size()). Does not modify *this.
        [[nodiscard]] CordBufferBase share_range(size_t offset) const;

        /// Alias for share_range.
        [[nodiscard]] CordBufferBase slice(size_t offset, size_t length) const {
            return share_range(offset, length);
        }

        /// Alias for share_range(offset).
        [[nodiscard]] CordBufferBase slice(size_t offset) const {
            return share_range(offset);
        }

        /// Appends a shared sub-range of @p src. Does not modify @p src.
        void append_range(const CordBufferBase &src, size_t offset, size_t length);

        /// Appends a shared suffix of @p src starting at @p offset.
        void append_range(const CordBufferBase &src, size_t offset);

        /// Removes [offset, offset + length) and returns it. Taken bytes are exclusive
        /// (truncated segments and shared whole segments are copied).
        [[nodiscard]] CordBufferBase take_range(size_t offset, size_t length);

        /// Removes the first @p n bytes and returns them (exclusive ownership).
        [[nodiscard]] CordBufferBase take_front(size_t n);

        /// Removes the last @p n bytes and returns them (exclusive ownership).
        [[nodiscard]] CordBufferBase take_back(size_t n);

        /// Deep-copies [offset, offset + length) into a new cord. Does not modify *this.
        [[nodiscard]] CordBufferBase copy_range(size_t offset, size_t length) const;

        /// Deep-copies the suffix [offset, size()).
        [[nodiscard]] CordBufferBase copy_range(size_t offset) const;

        /// Appends a deep copy of a sub-range of @p src. Does not modify @p src.
        void append_copy(const CordBufferBase &src, size_t offset, size_t length);

        /// Appends a deep copy of the suffix of @p src starting at @p offset.
        void append_copy(const CordBufferBase &src, size_t offset);

        /// Writes [offset, offset + length) into @p recv (deep copy).
        turbo::Status flatten_range(size_t offset, size_t length, Receiver &recv) const;

        /// Returns [offset, offset + length) as a contiguous string (deep copy).
        template<typename String = std::string, std::enable_if_t<is_contiguous_string_receiver<String>::value, int> = 0>
        String flatten_range(size_t offset, size_t length) const;

        /// Splits into [0, pos) and [pos, size()) as shared cords. Does not modify *this.
        [[nodiscard]] SplitView split(size_t pos) const;

        /// Keeps [0, pos) in *this and returns [pos, size()) (take semantics on the suffix).
        [[nodiscard]] CordBufferBase split_off(size_t pos);

        /// Keeps only [offset, offset + length), discarding the rest.
        void retain_range(size_t offset, size_t length) noexcept;

        /// @}

    protected:
        BufferRef<Alignment> *get_write_able_buffer();

        void _update_write_buffer() noexcept;

        [[nodiscard]] size_t _clamp_range_length(size_t offset, size_t length) const noexcept;

        [[nodiscard]] std::vector<BufferRef<Alignment> >
        _refs_for_range_share(size_t offset, size_t length) const;

        [[nodiscard]] static BufferRef<Alignment> _copy_bytes(const char *data, size_t len);

        void _take_front_into(CordBufferBase &result, size_t n) noexcept;

        void _take_back_into(CordBufferBase &result, size_t n) noexcept;

    protected:
        static const BufferRef<Alignment> kZeroBuffer;
        mutable BufferRef<Alignment> *_write_buffer{const_cast<BufferRef<Alignment> *>(&kZeroBuffer)};
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
    const BufferRef<Alignment> CordBufferBase<Alignment,
                BlockSize>::kZeroBuffer = []() {
                BufferRef<Alignment> r;
                r.buffer = std::make_shared<Buffer<char, Alignment> >();
                return r;
            }();


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
    inline size_t CordBufferBase<Alignment, BlockSize>::buffer_count() const {
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
    BufferRef<Alignment> *CordBufferBase<Alignment, BlockSize>::get_write_able_buffer() {
        if (_write_buffer->write_able()) {
            return _write_buffer;
        }
        auto ref = BufferRef<Alignment>::create_write_able(BlockSize);
        _views.push_back(std::move(ref));
        _write_buffer = &_views.back();
        return _write_buffer;
    }


    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::swap(CordBufferBase &other) noexcept {
        using std::swap;
        swap(_views, other._views);
        swap(_total_size, other._total_size);
        _update_write_buffer();
        other._update_write_buffer();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::flatten(Receiver &recv) const {
        TURBO_RETURN_NOT_OK(recv.reserve(_total_size));
        for (auto &ref: _views) {
            TURBO_RETURN_NOT_OK(recv.append(ref.data(), ref.size()));
        }
        return turbo::OkStatus();
    }


    template<size_t Alignment, size_t BlockSize>
    template<typename String, std::enable_if_t<is_contiguous_string_receiver<String>::value, int> >
    String CordBufferBase<Alignment, BlockSize>::flatten() const {
        String result;
        result.reserve(_total_size);
        for (auto &ref: _views) {
            KLOG(INFO)<<std::string_view{ref.data(), ref.size()};
            result.append(ref.data(), ref.size());
        }
        return std::move(result);
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_reference(const BufferRef<Alignment> &ref) {
        _views.push_back(ref);
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        _total_size += ref.size();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_reference(BufferRef<Alignment> &&ref) {
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        _total_size += ref.size();
        _views.push_back(std::move(ref));
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment,
        BlockSize>::append_reference(const std::vector<BufferRef<Alignment> > &refs) {
        for (auto &ref: refs) {
            _views.push_back(ref);
            _total_size += ref.size();
        }
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_reference(std::vector<BufferRef<Alignment> > &&refs) {
        for (auto &ref: refs) {
            _total_size += ref.size();
            _views.push_back(std::move(ref));
        }
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend_reference(const BufferRef<Alignment> &ref) {
        _views.push_front(ref);
        _total_size += ref.size();
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend_reference(BufferRef<Alignment> &&ref) {
        _total_size += ref.size();
        _views.push_front(std::move(ref));
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend_reference(
        const std::vector<BufferRef<Alignment> > &refs) {
        for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
            _views.push_front(*it);
            _total_size += (*it).size();
        }
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend_reference(std::vector<BufferRef<Alignment> > &&refs) {
        for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
            _total_size += (*it).size();
            _views.push_front(std::move(*it));
        }
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_writeable(BufferRef<Alignment> &&ref) {
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
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_writeable(std::vector<BufferRef<Alignment> > &&refs) {
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
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_buffer(
        const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset, size_t length) {
        if (!buf || buf->empty()) return turbo::OkStatus();
        if (offset + length > buf->size()) {
            return turbo::invalid_argument_error("overflow buffer size");
        }
        _total_size += length;
        BufferRef<Alignment> ref = BufferRef<Alignment>::reference(buf, offset, length);
        _views.push_back(std::move(ref));
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        return turbo::OkStatus();
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append_buffer(
        std::shared_ptr<Buffer<value_type, Alignment> > &&buf, size_t offset, size_t length) {
        if (!buf || buf->empty()) return turbo::OkStatus();
        if (offset + length > buf->size()) {
            return turbo::invalid_argument_error("overflow buffer size");
        }
        _total_size += length;
        BufferRef<Alignment> ref = BufferRef<Alignment>::assign(std::move(buf), offset, length);
        _views.push_back(std::move(ref));
        // Always set _write_buffer to the newly appended buffer (the tail).
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        return turbo::OkStatus();
    }


    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append(const CordBufferBase &rhs) {
        for (const auto &ref: rhs._views) {
            BufferRef<Alignment> new_ref;
            new_ref.buffer = ref.buffer;
            new_ref.range = ref.range;
            _views.push_back(std::move(new_ref));
        }
        _total_size += rhs._total_size;
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append(CordBufferBase &&rhs) {
        if (rhs._views.empty()) return;
        for (auto &it: rhs._views) {
            _views.push_back(std::move(it));
        }
        _total_size += rhs._total_size;
        rhs._views.clear();
        rhs._total_size = 0;
        rhs._write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        const auto &last = _views.back();
        if (_views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::prepend(const CordBufferBase &rhs) {
        if (rhs._views.empty()) return;
        // Insert copies of rhs segments at the front in reverse order to preserve original order.
        for (auto it = rhs._views.rbegin(); it != rhs._views.rend(); ++it) {
            const auto &ref = *it;
            BufferRef<Alignment> new_ref;
            new_ref.buffer = ref.buffer;
            new_ref.range = ref.range;
            _views.push_front(std::move(new_ref));
        }
        _total_size += rhs._total_size;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::prepend(CordBufferBase &&rhs) {
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
            rhs._write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> CordBufferBase<Alignment, BlockSize>::share() const {
        CordBufferBase result;
        for (const auto &ref: _views) {
            BufferRef<Alignment> new_ref;
            // Share the underlying buffer (increase reference count)
            new_ref.buffer = ref.buffer;
            new_ref.range = ref.range;
            result._views.push_back(std::move(new_ref));
        }
        result._total_size = _total_size;
        // Shared copy must not write into the original buffers; reset the writable pointer.
        result._write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        this->_write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> CordBufferBase<Alignment, BlockSize>::copy() const {
        CordBufferBase result;
        for (const auto &ref: _views) {
            const char *data = ref.buffer->data() + ref.range.offset;
            result.append(data, ref.range.length).ignore_error();
        }
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(const CordBufferBase &rhs) {
        if (this != &rhs) {
            CordBufferBase temp = rhs.share();
            swap(temp);
        }
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(CordBufferBase &&rhs) noexcept {
        if (this != &rhs) {
            _views = std::move(rhs._views);
            _total_size = rhs._total_size;
            _update_write_buffer();
            rhs._total_size = 0;
            rhs._write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
        return *this;
    }

    /// Assigns the cord to a copy of the data from a string range.
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
    /// The buffer's range (if any) is ignored; only the actual data is copied.
    template<size_t Alignment, size_t BlockSize>
    template<size_t BA>
    CordBufferBase<Alignment, BlockSize> &
    CordBufferBase<Alignment, BlockSize>::operator=(const Buffer<char, BA> &data) {
        clear();
        append(data.data(), data.size()).ignore_error();
        return *this;
    }

    /// Appends a string range to the cord.
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
        _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend_buffer(
        const std::shared_ptr<Buffer<value_type, Alignment> > &buf, size_t offset, size_t length) {
        if (!buf || buf->empty()) {
            return turbo::OkStatus();
        }

        if (offset + length > buf->size()) {
            return turbo::invalid_argument_error("overflow buffer");
        }

        BufferRef<Alignment> ref = BufferRef<Alignment>::reference(buf, offset, length);
        _views.push_front(std::move(ref));
        _total_size += _views.front().size();

        if (_views.back().is_unique()) {
            // The new buffer is the only segment, so it is also the tail.
            _write_buffer = &_views.back();
        }
        // Otherwise the existing tail remains the writable buffer; do nothing.

        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::prepend_buffer(
        std::shared_ptr<Buffer<value_type, Alignment> > &&buf, size_t offset, size_t length) {
        if (!buf || buf->empty()) {
            return turbo::OkStatus();
        }
        BufferRef<Alignment> ref = BufferRef<Alignment>::assign(std::move(buf), offset, length);
        _views.push_front(std::move(ref));
        _total_size += _views.front().size();
        return turbo::OkStatus();
    }


    template<size_t Alignment, size_t BlockSize>
    std::vector<BufferRef<Alignment> >
    CordBufferBase<Alignment, BlockSize>::create_buffers(size_t bytes_needed) {
        if (bytes_needed >= kMaxSingleCordSize) {
            throw std::out_of_range("CordBufferBase::create_buffers(size_t bytes_needed)");
        }
        std::vector<BufferRef<Alignment> > result;
        if (bytes_needed == 0) {
            return result;
        }
        size_t remaining = bytes_needed;
        while (remaining > 0) {
            auto buf = BufferRef<Alignment>::create_write_able(BlockSize);
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
    BufferRef<Alignment>
    CordBufferBase<Alignment, BlockSize>::create_big_buffer(size_t bytes_needed) {
        if (bytes_needed >= kMaxSingleCordSize) {
            throw std::out_of_range("CordBufferBase::create_buffers(size_t bytes_needed)");
        }
        BufferRef<Alignment> result;
        if (bytes_needed == 0) {
            return result;
        }
        auto n = (bytes_needed + BlockSize - 1) / BlockSize;
        result = BufferRef<Alignment>::create_write_able(BlockSize * n);
        return std::move(result);
    }


    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(const void * TURBO_RESTRICT data, size_t size) {
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
    turbo::span<char> CordBufferBase<Alignment, BlockSize>::output_next() {
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
    bool CordBufferBase<Alignment, BlockSize>::output_next(void **out, int *size) {
        auto *buf = get_write_able_buffer();
        auto r = buf->borrow(out, size);
        if (!r) return false;
        _total_size += *size;
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
            BufferRef<Alignment> &ref = _views.back();
            remaining -= ref.backup(remaining);
            if (ref.size() == 0) {
                _views.pop_back();
            }
        }
        _total_size -= to_remove;
        if (!_views.empty()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
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
            BufferRef<Alignment> &front = _views.front();
            // Determine the effective size of the front segment (range or full buffer)
            remaining -= front.pop_front(remaining);
            if (front.size() == 0) {
                _views.pop_front();
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
        _total_size -= front.range.length;
        _views.pop_front();
        if (_views.empty()) {
            _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    const BufferRef<Alignment> &
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
            BufferRef<Alignment> &back = _views.back();
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
            _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::pop_back_buffer() noexcept {
        if (_views.empty()) {
            return;
        }
        const auto &back = _views.back();
        _total_size -= back.range.length;
        _views.pop_back();
        if (!_views.empty() && _views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    const BufferRef<Alignment> &
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
        std::vector<BufferRef<Alignment> > buffers;
        if (collect < max_limited) {
            auto bsize = (max_limited - collect) / BlockSize + 1;
            size_t n;
            if (!restart_block) {
                n = std::min(bsize, kMaxReadVSpans - 1);
            } else {
                n = std::min(bsize, kMaxReadVSpans);
            }

            for (size_t i = 0; i < n - 1; i++) {
                auto ref = BufferRef<Alignment>::create_write_able(BlockSize);
                vecs.emplace_back(ref.buffer->data(), ref.capacity());
                collect += ref.capacity();
                buffers.push_back(std::move(ref));
            }
            auto ref = BufferRef<Alignment>::create_write_able(BlockSize);
            auto last = std::min(max_limited - collect, ref.capacity());
            collect += last;
            vecs.emplace_back(ref.buffer->data(), last);
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
            if (got < span.size()) {
                output_backup(span.size() - got);
                break;
            }
        }
        return total;
    }

    template<size_t Alignment, size_t BlockSize>
    size_t CordBufferBase<Alignment, BlockSize>::_clamp_range_length(size_t offset, size_t length) const noexcept {
        if (offset >= _total_size) {
            return 0;
        }
        return std::min(length, _total_size - offset);
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::_update_write_buffer() noexcept {
        if (!_views.empty() && _views.back().is_unique() && _views.back().write_able()) {
            _write_buffer = &_views.back();
        } else {
            _write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        }
    }

    template<size_t Alignment, size_t BlockSize>
    BufferRef<Alignment> CordBufferBase<Alignment, BlockSize>::_copy_bytes(const char *data, size_t len) {
        if (len == 0) {
            return {};
        }
        KCHECK(len <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()));
        auto ref = BufferRef<Alignment>::create_write_able(len);
        std::memcpy(ref.buffer->data(), data, len);
        ref.range.length = static_cast<uint32_t>(len);
        return ref;
    }

    template<size_t Alignment, size_t BlockSize>
    std::vector<BufferRef<Alignment> >
    CordBufferBase<Alignment, BlockSize>::_refs_for_range_share(size_t offset, size_t length) const {
        length = _clamp_range_length(offset, length);
        std::vector<BufferRef<Alignment> > result;
        if (length == 0) {
            return result;
        }

        size_t pos = 0;
        const size_t range_end = offset + length;
        for (const auto &ref: _views) {
            const size_t seg_size = ref.size();
            const size_t seg_start = pos;
            const size_t seg_end = pos + seg_size;
            pos = seg_end;

            if (seg_end <= offset || seg_start >= range_end) {
                continue;
            }

            const size_t intersect_start = std::max(seg_start, offset);
            const size_t intersect_end = std::min(seg_end, range_end);
            const size_t skip = intersect_start - seg_start;
            const size_t take = intersect_end - intersect_start;
            KCHECK(ref.range.offset + skip <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()));
            KCHECK(take <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()));
            result.push_back(BufferRef<Alignment>::reference(
                ref.buffer, static_cast<uint32_t>(ref.range.offset + skip), static_cast<uint32_t>(take)));
        }
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::_take_front_into(CordBufferBase &result, size_t n) noexcept {
        n = std::min(n, _total_size);
        size_t remaining = n;
        while (remaining > 0 && !_views.empty()) {
            BufferRef<Alignment> &front = _views.front();
            const size_t seg_size = front.size();
            const size_t take = std::min(remaining, seg_size);

            if (take < seg_size) {
                result._views.push_back(_copy_bytes(front.data(), take));
                result._total_size += take;
                front.pop_front(take);
            } else {
                if (front.buffer.use_count() > 1) {
                    result._views.push_back(_copy_bytes(front.data(), seg_size));
                    result._total_size += seg_size;
                } else {
                    result._total_size += seg_size;
                    result._views.push_back(std::move(front));
                }
                _views.pop_front();
            }
            remaining -= take;
            _total_size -= take;
        }
        result._update_write_buffer();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::_take_back_into(CordBufferBase &result, size_t n) noexcept {
        n = std::min(n, _total_size);
        size_t remaining = n;
        while (remaining > 0 && !_views.empty()) {
            BufferRef<Alignment> &back = _views.back();
            const size_t seg_size = back.size();
            const size_t take = std::min(remaining, seg_size);

            if (take < seg_size) {
                const char *start = back.data() + (seg_size - take);
                result.prepend_reference(_copy_bytes(start, take));
                back.pop_back(take);
            } else {
                if (back.buffer.use_count() > 1) {
                    result.prepend_reference(_copy_bytes(back.data(), seg_size));
                } else {
                    BufferRef<Alignment> moved = std::move(back);
                    result.prepend_reference(std::move(moved));
                }
                _views.pop_back();
            }
            remaining -= take;
            _total_size -= take;
        }
        result._update_write_buffer();
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::share_range(size_t offset, size_t length) const {
        CordBufferBase result;
        const auto refs = _refs_for_range_share(offset, length);
        for (const auto &ref: refs) {
            result._views.push_back(ref);
            result._total_size += ref.size();
        }
        result._write_buffer = const_cast<BufferRef<Alignment> *>(&kZeroBuffer);
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::share_range(size_t offset) const {
        return share_range(offset, _total_size - offset);
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append_range(const CordBufferBase &src, size_t offset,
                                                            size_t length) {
        append_reference(src._refs_for_range_share(offset, length)).ignore_error();
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append_range(const CordBufferBase &src, size_t offset) {
        append_range(src, offset, src._total_size - offset);
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::take_front(size_t n) {
        CordBufferBase result;
        _take_front_into(result, n);
        _update_write_buffer();
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::take_back(size_t n) {
        CordBufferBase result;
        _take_back_into(result, n);
        _update_write_buffer();
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::take_range(size_t offset, size_t length) {
        length = _clamp_range_length(offset, length);
        if (length == 0) {
            return {};
        }

        CordBufferBase prefix;
        if (offset > 0) {
            prefix = take_front(offset);
        }
        CordBufferBase taken = take_front(length);
        CordBufferBase suffix = std::move(*this);
        clear();
        append(std::move(prefix));
        append(std::move(suffix));
        return taken;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::copy_range(size_t offset, size_t length) const {
        CordBufferBase result;
        const auto refs = _refs_for_range_share(offset, length);
        for (const auto &ref: refs) {
            result._views.push_back(_copy_bytes(ref.data(), ref.size()));
            result._total_size += ref.size();
        }
        result._update_write_buffer();
        return result;
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::copy_range(size_t offset) const {
        return copy_range(offset, _total_size - offset);
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append_copy(const CordBufferBase &src, size_t offset,
                                                           size_t length) {
        append(std::move(src.copy_range(offset, length)));
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::append_copy(const CordBufferBase &src, size_t offset) {
        append_copy(src, offset, src._total_size - offset);
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::flatten_range(size_t offset, size_t length,
                                                                      Receiver &recv) const {
        length = _clamp_range_length(offset, length);
        TURBO_RETURN_NOT_OK(recv.reserve(length));
        const auto refs = _refs_for_range_share(offset, length);
        for (const auto &ref: refs) {
            TURBO_RETURN_NOT_OK(recv.append(ref.data(), ref.size()));
        }
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    template<typename String, std::enable_if_t<is_contiguous_string_receiver<String>::value, int> >
    String CordBufferBase<Alignment, BlockSize>::flatten_range(size_t offset, size_t length) const {
        length = _clamp_range_length(offset, length);
        String result;
        result.reserve(length);
        const auto refs = _refs_for_range_share(offset, length);
        for (const auto &ref: refs) {
            result.append(ref.data(), ref.size());
        }
        return std::move(result);
    }

    template<size_t Alignment, size_t BlockSize>
    typename CordBufferBase<Alignment, BlockSize>::SplitView
    CordBufferBase<Alignment, BlockSize>::split(size_t pos) const {
        return {share_range(0, pos), share_range(pos)};
    }

    template<size_t Alignment, size_t BlockSize>
    CordBufferBase<Alignment, BlockSize>
    CordBufferBase<Alignment, BlockSize>::split_off(size_t pos) {
        if (pos >= _total_size) {
            return {};
        }
        return take_back(_total_size - pos);
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::retain_range(size_t offset, size_t length) noexcept {
        length = _clamp_range_length(offset, length);
        if (length == 0) {
            clear();
            return;
        }
        pop_front(offset);
        pop_back(_total_size - length);
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

    template<size_t Alignment, size_t BlockSize>
    struct is_cord_buffer<fermat::CordBufferBase<Alignment, BlockSize> > : std::true_type {
        static constexpr size_t alignment = Alignment;
        static constexpr size_t block_size = BlockSize;
    };

} // namespace fermat
