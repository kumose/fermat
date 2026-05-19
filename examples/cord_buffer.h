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
            turbo::span<char> view;
        };

    public:
        using value_type = char;
        using buffer_type = Buffer<value_type, Alignment>;

        using vector_type = fermat::Vector<BufferRef>;
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

        class OutputStream {
        public:
            explicit OutputStream(CordBufferBase *cord) : _cord(cord) {
            }

            ~OutputStream() = default;

            OutputStream(const OutputStream &) = default;

            OutputStream(OutputStream &&) = default;

            OutputStream &operator=(const OutputStream &) = default;

            OutputStream &operator=(OutputStream &&) = default;

            bool next(void **data, int *size) {
                _cord->reserve_writeable(BlockSize + 1);
                auto buffer = _cord->get_write_able_buffer();
                _byte_count += buffer->capacity() - buffer->size();
                *data = buffer->data() + buffer->size();
                *size = buffer->capacity() - buffer->size();
                _cord->_writeable -= *size;
                buffer->resize(buffer->capacity());
                return true;
            }

            // `count' can be as long as ByteCount()
            void back_up(int count) {
                DKCHECK(count < _byte_count);
                size_t backed = 0;
                while (backed < count) {
                    auto &br = _cord->_views.back();
                    auto back = std::min(br.buffer->size(), count - backed);
                    br.buffer->resize(br.buffer->size() - back);
                    if (br.buffer->empty()) {
                        auto tmp = std::move(_cord->_views.back());
                        _cord->_views.pop_back();
                        _cord->_current.push_back(std::move(tmp));
                    }
                    backed += back;
                }
                _cord->_writeable += count;
                _byte_count -= count;
            }

            [[nodiscard]] int64_t byte_count() const {
                return _byte_count;
            }

        protected:
            CordBufferBase *_cord;
            int64_t _byte_count{0};
        };

        /// ZeroCopyInputStream adapter for CordBufferBase.
        /// Provides sequential read access to the underlying concatenated buffers.
        class InputStream {
        public:
            /// Constructs an InputStream reading from the given CordBufferBase.
            /// The buffer must remain valid for the lifetime of this stream.
            explicit InputStream(const CordBufferBase *cord) : _cord(cord) {
                _index = 0;
                _offset = 0;
                _skip_empty();
            }

            /// Obtains the next chunk of data.
            /// @param data  Output pointer to the beginning of the chunk.
            /// @param size  Output size of the chunk in bytes.
            /// @return true if there is more data, false at end of stream.
            bool next(const void **data, int *size) {
                if (_index >= _cord->_views.size()) return false;
                const auto &ref = _cord->_views[_index];
                const char *buf = ref.buffer->data();
                size_t buf_size = ref.buffer->size();
                *data = buf + _offset;
                *size = static_cast<int>(buf_size - _offset);
                ++_index;
                _offset = 0;
                _skip_empty();
                return true;
            }

            /// Backs up a number of bytes, which must not exceed the size of the last
            /// chunk returned by next().
            void back_up(int count) {
                if (count <= 0) return;
                // The last chunk came from the block at (_index - 1)
                if (_index > 0) {
                    --_index;
                    const auto &ref = _cord->_views[_index];
                    _offset = ref.buffer->size() - count;
                } else {
                    // No previous block, adjust current offset (should be rare)
                    _offset -= count;
                }
            }

            /// Returns the total number of bytes read so far.
            [[nodiscard]] int64_t byte_count() const {
                // Bytes read = total size - bytes remaining
                size_t remaining = 0;
                for (size_t i = _index; i < _cord->_views.size(); ++i) {
                    remaining += _cord->_views[i].buffer->size();
                }
                if (_index < _cord->_views.size()) {
                    remaining -= _offset;
                }
                return static_cast<int64_t>(_cord->size() - remaining);
            }

        private:
            /// Advances to the first non-empty block (if any).
            void _skip_empty() {
                while (_index < _cord->_views.size() &&
                       _cord->_views[_index].buffer->size() == 0) {
                    ++_index;
                }
            }

            const CordBufferBase *_cord;
            size_t _index; ///< Current block index in _views
            size_t _offset; ///< Offset within current block
        };

        class FormatSink {
        public:
            explicit FormatSink(CordBufferBase &buf) : buf_(buf) {
            }

            void Append(std::string_view s) {
                buf_.append(s.data(), s.size()).ignore_error();
            }

        private:
            friend void turbo_format_flush(FormatSink* sink, std::string_view v) {
                sink->Append(v);
            }

            CordBufferBase &buf_;
        };

    public:
        CordBufferBase() = default;

        CordBufferBase(const CordBufferBase &) = delete;

        CordBufferBase &operator=(const CordBufferBase &) = delete;

        // Move constructor: transfer ownership of all resources.
        // The source will be left in a valid but empty state (similar to default-constructed).
        // Precondition: source must not be in a state where it has an active lease?
        // Actually we just move the lease; after move, source will have an empty lease.
        CordBufferBase(CordBufferBase &&rhs) noexcept
            : _views(std::move(rhs._views)),
              _total_size(std::exchange(rhs._total_size, 0)) {
            // After moving _views, rhs._views is empty. Its _lease is now moved,
            // so rhs._lease will have _capacity = 0 (since BufferLease's move constructor
            // moves the spans and resets the source to empty). That is desirable.
            // No further action needed.
        }

        // Move assignment: release current resources, then transfer from rhs.
        CordBufferBase &operator=(CordBufferBase &&rhs) noexcept {
            if (this != &rhs) {
                // Free current resources

                // Transfer ownership
                _views = std::move(rhs._views);
                _total_size = std::exchange(rhs._total_size, 0);
            }
            return *this;
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

        OutputStream output_stream() {
            return OutputStream(this);
        }

        InputStream input_stream() const {
            return InputStream(this);
        }

        FormatSink format_sink() {
            return FormatSink(*this);
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

        turbo::Status append(std::string_view data);

        turbo::Status append(const void *data, size_t size);

        turbo::Result<std::pair<size_t, bool> > append_by_readv(IOReader &reader, size_t max_limited);

        turbo::Result<std::pair<size_t, bool> > append_by_read(IOReader &reader, size_t max_limited);

        turbo::Result<std::pair<size_t, bool> > append_by_pread(IOReader &reader, size_t offset, size_t max_limited);

        void reserve_writeable(size_t n);

        void reserve(size_t n) {
            if (n <= _writeable + _total_size) {
                return;
            }
            reserve_writeable(n - _total_size);
        }

        BufferLease *borrow(size_t byte_size);

        void commit(BufferLease *l);

        std::vector<Buffer<char, Alignment> > release();

        const std::vector<Buffer<char, Alignment> > &buffers() const;

    protected:
        Buffer<char, Alignment> *get_write_able_buffer();

    protected:
        std::list<BufferRef> _current;
        ///< All block views (including Umount ones).
        fermat::Vector<BufferRef> _views;
        ///< Total logical bytes stored.
        size_t _total_size{0};
        size_t _writeable{0};
        BufferLease _lease;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// inlines
    ///
    ///////////////////////////////////////////////////////////////////////////
    ///


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
    void CordBufferBase<Alignment, BlockSize>::reserve_writeable(size_t n) {
        while (n > _writeable) {
            BufferRef ref;
            ref.buffer = std::make_shared<Buffer<char, Alignment> >();
            ref.buffer->reserve(BlockSize);
            _current.push_back(std::move(ref));
            _writeable += BlockSize;
        }
    }

    template<size_t Alignment, size_t BlockSize>
    Buffer<char, Alignment> *CordBufferBase<Alignment, BlockSize>::get_write_able_buffer() {
        if (!_views.empty() && _views.back().buffer->capacity() > _views.back().buffer->size()) {
            return _views.back().buffer.get();
        }
        auto v = std::move(_current.front());
        _current.pop_front();
        _views.push_back(std::move(v));
        return _views.back().buffer.get();
    }

    template<size_t Alignment, size_t BlockSize>
    BufferLease *CordBufferBase<Alignment, BlockSize>::borrow(size_t byte_size) {
        DKCHECK(!_lease.borrowed());
        if (byte_size == 0) {
            return nullptr;
        }
        reserve_writeable(byte_size);

        std::vector<turbo::span<char> > spans;
        size_t remain = byte_size;

        /// 1. Check if there is an incomplete block at the end of _views.
        if (!_views.empty()) {
            BufferRef &last = _views.back();
            Buffer<char, Alignment> *buf = last.buffer.get();
            size_t free = buf->capacity() - buf->size();
            if (free > 0) {
                size_t take = std::min(remain, free);
                last.view = {buf->data() + buf->size(), take};
                spans.emplace_back(buf->data() + buf->size(), take);
                remain -= take;
            }
        }

        /// 2. Walk through _current (all fresh unused blocks).
        for (auto it = _current.begin(); it != _current.end() && remain > 0; ++it) {
            BufferRef &ref = *it;
            Buffer<char, Alignment> *buf = ref.buffer.get();
            size_t free = buf->capacity() - buf->size(); // size is 0 initially
            size_t take = std::min(remain, free);
            ref.view = {buf->data() + buf->size(), take};
            spans.emplace_back(buf->data() + buf->size(), take);
            remain -= take;
        }

        DKCHECK(remain == 0);
        _lease.set(std::move(spans));
        return &_lease;
    }

    template<size_t Alignment, size_t BlockSize>
    void CordBufferBase<Alignment, BlockSize>::commit(BufferLease *l) {
        KCHECK(l == &_lease) << "Fatal: Lease does not belong to this IOBuf. "
                     << "Expected internal lease address " << &_lease
                     << ", got " << l << ". Possible double commit or foreign lease.";
        if (!_lease.borrowed()) return;

        auto remain = l->size();

        if (!_views.empty()) {
            BufferRef &last = _views.back();
            Buffer<char, Alignment> *buf = last.buffer.get();
            size_t free = buf->capacity() - buf->size();
            if (free > 0) {
                size_t take = std::min(remain, free);
                last.view = {buf->data() + buf->size(), take};
                buf->resize(take + buf->size());
                remain -= take;
            }
        }
        while (remain > 0) {
            auto v = std::move(_current.front());
            _current.pop_front();
            size_t take = std::min(remain, v.buffer->capacity());
            v.buffer->resize(take);
            remain -= take;
            _views.push_back(std::move(v));
        }
        _total_size += _lease.size();
        _writeable -= _lease.size();
        _lease.clear();
    }

    template<size_t Alignment, size_t BlockSize>
    std::vector<Buffer<char, Alignment> > CordBufferBase<Alignment, BlockSize>::release() {
        return std::move(_views);
    }

    template<size_t Alignment, size_t BlockSize>
    const std::vector<Buffer<char, Alignment> > &CordBufferBase<Alignment, BlockSize>::buffers() const {
        return _views;
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Status CordBufferBase<Alignment, BlockSize>::append(const void *data, size_t size) {
        if (size == 0 || data == nullptr) return turbo::OkStatus();
        reserve_writeable(size);
        const char *src = static_cast<const char *>(data);
        auto remain = size;
        do {
            auto *b = get_write_able_buffer();
            auto app_len = std::min(remain, b->capacity() - b->size());
            b->append(src, app_len);
            remain -= app_len;
            src += app_len;
        } while (remain > 0);
        _total_size += size;
        _writeable -= size;
        return turbo::OkStatus();
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<std::pair<size_t, bool> > CordBufferBase<Alignment, BlockSize>::append_by_readv(
        IOReader &reader, size_t max_limited) {
        bool reach_read_zero = false;
        if (max_limited == 0) {
            return std::pair<size_t, bool>{0ul, reach_read_zero};
        }
        reserve_writeable(max_limited);
        std::vector<turbo::span<char> > vecs;
        vecs.reserve(kMaxReadVSpans);
        size_t remain = max_limited;
        do {
            vecs.clear();
            size_t batch_remain = 0;
            auto *b = get_write_able_buffer();
            auto app_len = std::min(remain, b->capacity() - b->size());
            auto span = turbo::span<char>(b->data() + b->size(), app_len);
            vecs.push_back(span);
            batch_remain += app_len;
            for (auto &it: _current) {
                auto seg_len = std::min(remain - batch_remain, it.buffer->capacity());
                auto s = turbo::span<char>(it.buffer->data(), (seg_len));
                vecs.push_back(s);
                batch_remain += seg_len;
                if (batch_remain == remain || vecs.size() == kMaxReadVSpans) {
                    break;
                }
            }
            TURBO_MOVE_OR_RAISE(auto r, reader.readv(vecs, batch_remain));

            remain -= r;
            size_t processed_bytes = 0;
            if (r > 0) {
                auto *buf = _views.back().buffer.get();
                auto commit_len = std::min(r, buf->capacity() - buf->size());
                buf->resize(commit_len + buf->size());
                processed_bytes += commit_len;

                while (processed_bytes < r) {
                    BufferRef ref = std::move(_current.front());
                    _current.pop_front();
                    commit_len = std::min(r - processed_bytes, ref.buffer->capacity());
                    ref.buffer->resize(commit_len);
                    _views.push_back(std::move(ref));
                    processed_bytes += commit_len;
                }
            }
            if (processed_bytes < batch_remain) {
                reach_read_zero = true;
                break;
            }
        } while (remain > 0);
        _total_size += (max_limited - remain);
        _writeable -= (max_limited - remain);
        return std::pair<size_t, bool>{max_limited - remain, reach_read_zero};
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<std::pair<size_t, bool> > CordBufferBase<Alignment, BlockSize>::append_by_read(
        IOReader &reader, size_t max_limited) {
        bool reach_read_zero = false;
        if (max_limited == 0) {
            return std::pair<size_t, bool>{0ul, reach_read_zero};
        }
        size_t remain = max_limited;
        do {
            auto *b = get_write_able_buffer();
            auto app_len = std::min(remain, b->capacity() - b->size());
            turbo::span<char> span(b->data() + b->size(), app_len);
            TURBO_MOVE_OR_RAISE(auto r, reader.read(span));
            remain -= r;
            b->resize(b->size() + r);
            if (r < app_len) {
                reach_read_zero = true;
                break;
            }
        } while (remain > 0);
        _total_size += (max_limited - remain);
        _writeable -= (max_limited - remain);
        return std::pair<size_t, bool>{max_limited - remain, reach_read_zero};
    }

    template<size_t Alignment, size_t BlockSize>
    turbo::Result<std::pair<size_t, bool> > CordBufferBase<Alignment, BlockSize>::append_by_pread(
        IOReader &reader, size_t offset, size_t max_limited) {
        bool reach_read_zero = false;
        if (max_limited == 0) {
            return std::pair<size_t, bool>{0ul, reach_read_zero};
        }
        size_t remain = max_limited;
        do {
            auto *b = get_write_able_buffer();
            auto app_len = std::min(remain, b->capacity() - b->size());
            turbo::span<char> span(b->data() + b->size(), app_len);
            TURBO_MOVE_OR_RAISE(auto r, reader.pread(span, offset));
            remain -= r;
            b->resize(b->size() + r);
            offset += r;
            if (r < app_len) {
                reach_read_zero = true;
                break;
            }
        } while (remain > 0);
        _total_size += (max_limited - remain);
        _writeable -= (max_limited - remain);
        return std::pair<size_t, bool>{max_limited - remain, reach_read_zero};
    }


    template<size_t Alignment, size_t BlockSize>
    class CordBufferStreambuf : public std::streambuf {
    public:
        explicit CordBufferStreambuf(CordBufferBase<Alignment, BlockSize> *buf) : _cord(buf) {
        }

    protected:
        int overflow(int ch) override {
            if (ch == EOF) return 0;
            char c = static_cast<char>(ch);
            return _cord->append(&c, 1).ok() ? ch : EOF;
        }

        std::streamsize xsputn(const char *s, std::streamsize n) override {
            return _cord->append(s, static_cast<size_t>(n)).ok() ? n : 0;
        }

        int sync() override { return 0; }

    private:
        CordBufferBase<Alignment, BlockSize> *_cord;
    };

    template<size_t Alignment, size_t BlockSize>
    class CordBufferOStream : public std::ostream {
    public:
        explicit CordBufferOStream(CordBufferBase<Alignment, BlockSize> *buf)
            : std::ostream(&_streambuf), _streambuf(buf) {
        }

    private:
        CordBufferStreambuf<Alignment, BlockSize> _streambuf;
    };
} // namespace fermat
