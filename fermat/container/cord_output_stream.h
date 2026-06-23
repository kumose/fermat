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
#include <turbo/base/endian.h>
#include <ostream>
#include <streambuf>
#include <cstring>

namespace fermat {
    template<size_t Alignment, size_t BlockSize>
    class CordOutputStringStream;

    /// @brief Stream buffer adapter that writes data directly into a CordBufferBase.
    ///
    /// This streambuf uses the CordBufferBase's output_next() / output_backup() mechanism
    /// to obtain writable memory blocks. It automatically manages block boundaries and
    /// commits data to the cord as blocks are filled.
    template<size_t Alignment, size_t BlockSize>
    class CordBufferStreambuf : public std::streambuf {
    public:
        /// Constructs a streambuf that writes into the given cord.
        /// @param cord The target cord (must outlive the streambuf).
        explicit CordBufferStreambuf(CordBufferBase<Alignment, BlockSize> *cord) noexcept
            : _cord(cord) {
            // No initial buffer; the first write will allocate via overflow().
        }
        CordBufferStreambuf(CordBufferStreambuf &&) noexcept= default;
        CordBufferStreambuf &operator=(CordBufferStreambuf &&) noexcept= default;

        ~CordBufferStreambuf() override {
            sync(); // Commit any pending writes.
        }

    protected:
        /// Flushes the current buffer and releases it.
        int sync() override {
            return _commit_buffer() ? 0 : -1;
        }

        /// Handles a single character output.
        /// If there is space in the current buffer, writes there; otherwise,
        /// commits the current buffer and acquires a new one.
        int overflow(int ch) override {
            if (ch == EOF) {
                return sync() == 0 ? ch : traits_type::eof();
            }
            // If we have a current buffer and space remains, write directly.
            if (pptr() && pptr() < epptr()) {
                *pptr() = static_cast<char>(ch);
                pbump(1);
                return ch;
            }

            // Commit any existing buffer before acquiring a new one.
            if (!_commit_buffer()) {
                return EOF;
            }

            // Acquire a fresh writable block.
            if (!_acquire_buffer()) {
                return EOF;
            }

            // Write the character into the new buffer.
            *pptr() = static_cast<char>(ch);
            pbump(1);
            return ch;
        }

        /// Writes a sequence of characters.
        std::streamsize xsputn(const char *s, std::streamsize n) override {
            if (n <= 0) return 0;

            std::streamsize total = 0;
            while (total < n) {
                // If no buffer or buffer is full, commit and acquire new.
                if (!pptr() || pptr() == epptr()) {
                    if (!_commit_buffer() || !_acquire_buffer()) {
                        break;
                    }
                }

                std::streamsize avail = epptr() - pptr();
                std::streamsize to_copy = std::min(n - total, avail);
                std::memcpy(pptr(), s + total, static_cast<size_t>(to_copy));
                pbump(static_cast<int>(to_copy));
                total += to_copy;
            }
            return total;
        }

    private:
        friend class CordOutputStringStream<Alignment, BlockSize>;
        /// Commits the current buffer (if any) to the cord by adjusting the cord's
        /// logical size to match the actually written bytes.
        /// @return true on success, false on failure (should not happen here).
        bool _commit_buffer() {
            if (!pptr()) return true; // No active buffer.
            std::streamsize used = pptr() - pbase();
            if (used > 0) {
                // The buffer was obtained via output_next() which already reserved the
                // full block size. If we wrote less, we must roll back the unused portion.
                size_t reserved = epptr() - pbase();
                if (static_cast<size_t>(used) < reserved) {
                    _cord->output_backup(static_cast<int>(reserved - used));
                }
                // The cord's total size is already advanced by output_next();
                // output_backup has already reduced it if necessary.
            }
            // Reset buffer pointers to indicate no active buffer.
            setp(nullptr, nullptr);
            return true;
        }

        /// Acquires a new writable block from the cord via output_next().
        /// @return true if a block was successfully obtained.
        bool _acquire_buffer() {
            auto span = _cord->output_next();
            if (span.empty()) return false;
            // Set the buffer pointers: base, pptr, epptr.
            char *start = span.data();
            setp(start, start + span.size());
            return true;
        }

        CordBufferBase<Alignment, BlockSize> *_cord; ///< Target cord, not owned.
    };

    /// @brief An output stream that writes formatted data into a CordBufferBase.
    ///
    /// This class inherits std::ostream and uses CordBufferStreambuf to seamlessly
    /// append data (including formatted output via operator<<) to the underlying cord.
    /// It is particularly useful for building large strings or binary data incrementally
    /// without multiple allocations.
    ///
    /// Example:
    ///   CordBufferBase<64, 4096> cord;
    ///   CordOutputStringStream<64, 4096> oss(&cord);
    ///   oss << "Hello, " << 42 << " world!";
    ///   // Data is now stored in the cord.
    template<size_t Alignment, size_t BlockSize>
    class CordOutputStringStream : public std::ostream {
    public:
        using CordType = CordBufferBase<Alignment, BlockSize>;

        /// Constructs an output stream that writes into the provided cord.
        /// The cord must remain valid for the lifetime of this stream.
        explicit CordOutputStringStream(CordType *cord)
            : std::ostream(&_streambuf),
              _streambuf(cord) {
        }

        /// Default destructor.
        ~CordOutputStringStream() override = default;

        // Disable copy, allow move.
        CordOutputStringStream(const CordOutputStringStream &) = delete;

        CordOutputStringStream &operator=(const CordOutputStringStream &) = delete;

        CordOutputStringStream(CordOutputStringStream &&) noexcept = default;

        CordOutputStringStream &operator=(CordOutputStringStream &&) noexcept = default;

        /// Returns a reference to the underlying cord (read-only).
        const CordType &cord() const noexcept { return *_streambuf._cord; }

        /// Flushes the stream.
        CordOutputStringStream &flush() {
            std::ostream::flush();
            return *this;
        }

    private:
        CordBufferStreambuf<Alignment, BlockSize> _streambuf; ///< Underlying streambuf.
    };

    struct EndianFormat {
        template<typename T>
        static T encode(T host, bool big) {
            if (big) {
                return turbo::big_endian::from_host(host);
            } else {
                return turbo::little_endian::from_host(host);
            }
        }

        template<typename T>
        static T decode(T nex, bool big) {
            if (big) {
                return turbo::big_endian::to_host(nex);
            } else {
                return turbo::little_endian::to_host(nex);
            }
        }
    };

    struct BigEndian {
    };

    struct LittleEndian {
    };

    // -----------------------------------------------------------------------------
    // Proxy for writing a pointer as raw bytes.
    // Use: stream << RawPointer(ptr);
    // -----------------------------------------------------------------------------
    template<typename T>
    class RawPointer {
    public:
        /// Constructs from a raw pointer.
        explicit RawPointer(const T *ptr) noexcept : ptr_(ptr) {
        }

        /// Constructs from any smart pointer type that provides .get().
        template<typename SmartPtr>
        explicit RawPointer(const SmartPtr &smart) noexcept
            : ptr_(smart.get()) {
        }

        /// Returns the stored raw pointer.
        const T *get() const noexcept { return ptr_; }

    private:
        const T *ptr_;
    };

    template<typename T>
    class WithContainerSize {
    public:
        explicit WithContainerSize(const T &container) noexcept : container_(container) {
        }

        const T &get() const noexcept { return container_; }

    private:
        const T &container_;
    };

    template<size_t A, size_t B>
    class ShareCord {
    public:
        using CordType = CordBufferBase<A, B>;

        /// Constructs from a const reference (shallow copy, assumes cord outlives usage).
        explicit ShareCord(const CordType &cord) noexcept : cord_(cord) {
        }

        /// Returns the underlying cord pointer.
        const CordType &get() const noexcept { return cord_; }

    private:
        const CordType &cord_;
    };

    /// Binary output stream that writes into a CordBufferBase.
    /// Supports fixed-width integers, floating point, raw bytes, and strings.
    /// Endianness can be controlled via manipulators:
    ///   stream << BigEndian{} << value;   // writes value in big-endian
    ///   stream << LittleEndian{} << value; // writes value in little-endian
    /// The manipulator applies only to the immediate next arithmetic value,
    /// then the stream reverts to its default endianness.
    template<size_t Alignment, size_t BlockSize>
    class CordOutputBinaryStream {
    public:
        using CordType = CordBufferBase<Alignment, BlockSize>;

        // -------------------------------------------------------------------------
        //  Construction
        // -------------------------------------------------------------------------

        /// Constructor.
        /// @param cord Target cord (must outlive the stream).
        /// @param big_endian Default endianness (true = big, false = little).
        explicit CordOutputBinaryStream(CordType *cord, bool big_endian = false) noexcept;

        ~CordOutputBinaryStream() = default;

        CordOutputBinaryStream &reset(CordType *cord = nullptr) noexcept;

        // Disable copy, allow move.
        CordOutputBinaryStream(const CordOutputBinaryStream &) = default;

        CordOutputBinaryStream &operator=(const CordOutputBinaryStream &) = default;

        CordOutputBinaryStream(CordOutputBinaryStream &&) = default;

        CordOutputBinaryStream &operator=(CordOutputBinaryStream &&) = default;

        // -------------------------------------------------------------------------
        //  Raw byte writing
        // -------------------------------------------------------------------------

        /// Writes a sequence of raw bytes (no endian conversion).
        /// @param data Pointer to the data buffer.
        /// @param size Number of bytes to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &write(const void *data, size_t size);

        /// Writes a single byte.
        /// @param byte The byte value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &write(uint8_t byte);

        // -------------------------------------------------------------------------
        //  Endianness manipulators (affect only next arithmetic write)
        // -------------------------------------------------------------------------

        /// Temporarily enables big-endian for the next arithmetic value.
        CordOutputBinaryStream &operator<<(BigEndian);

        /// Temporarily enables little-endian for the next arithmetic value.
        CordOutputBinaryStream &operator<<(LittleEndian);

        // -------------------------------------------------------------------------
        //  Arithmetic types (endian-converted according to current flag)
        // -------------------------------------------------------------------------

        /// Writes a uint8_t value (no endian conversion needed).
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(uint8_t v);


        /// Writes an int8_t value (no endian conversion needed).
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(int8_t v);

        /// Writes a uint16_t value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(uint16_t v);

        /// Writes an int16_t value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(int16_t v);

        /// Writes a uint32_t value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(uint32_t v);


        /// Writes an int32_t value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(int32_t v);


        /// Writes a uint64_t value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(uint64_t v);

        /// Writes an int64_t value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(int64_t v);

        /// Writes a turbo::uint128 value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(turbo::uint128 v);

        /// Writes a turbo::int128 value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(turbo::int128 v);

        /// Writes a float value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(float v);

        /// Writes a double value with endian conversion using default endianness.
        /// @param v The value to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(double v);

        // -------------------------------------------------------------------------
        //  String and binary data (no implicit length prefix, no endian conversion)
        // -------------------------------------------------------------------------

        /// Writes raw bytes from a string view (no length prefix, no endian conversion).
        /// @param sv The string view to write.
        /// @return Reference to this stream.
        CordOutputBinaryStream &operator<<(std::string_view sv);

        /// Writes raw bytes from a span of std::byte.
        template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T> > >
        CordOutputBinaryStream &operator<<(turbo::span<const T> bytes);

        // -------------------------------------------------------------------------
        //  Pointer (raw address bytes)
        // -------------------------------------------------------------------------

        /// Writes the pointer value as raw bytes (size depends on platform).
        template<typename T>
        CordOutputBinaryStream &operator<<(RawPointer<T> ptr) {
            return write(&ptr.get(), sizeof(T *));
        }

        template<typename T>
        CordOutputBinaryStream &operator<<(const WithContainerSize<T> &wrapper);


        template<size_t OA, size_t OB>
        CordOutputBinaryStream &operator<<(const CordBufferBase<OA, OB> &wrapper);

        template<size_t OA, size_t OB>
        CordOutputBinaryStream &operator<<(const ShareCord<OA, OB> &wrapper);

        template<size_t OA, size_t OB>
        CordOutputBinaryStream &operator<<(CordBufferBase<OA, OB> &&wrapper);

        template<size_t OA>
        CordOutputBinaryStream &operator<<(const Buffer<char, OA> &wrapper);

        template<size_t OA>
        CordOutputBinaryStream &operator<<(Buffer<char, OA> &&wrapper);

        template<size_t OA>
        CordOutputBinaryStream &operator<<(BasicString<char, OA> &&wrapper);

        template<size_t OA>
        CordOutputBinaryStream &operator<<(Vector<char, OA> &&wrapper);


        // -------------------------------------------------------------------------
        //  User-defined types (ADL hook)
        // -------------------------------------------------------------------------

        /// Serializes a user-defined type.
        /// Prefers a member function `serialize(CordOutputBinaryStream&) const` returning turbo::Status.
        /// Falls back to `to_string() const` returning std::string (or convertible to string_view)
        /// and writes the resulting string as raw bytes (no implicit length prefix).
        template<typename T>
        auto operator<<(const T &value) -> std::enable_if_t<
            std::is_invocable_v<decltype(&T::serialize), const T &, CordOutputBinaryStream &>,
            turbo::Status> {
            return value.serialize(*this);
        }

        template<typename T>
        auto operator<<(const T &value) -> std::enable_if_t<
            !std::is_invocable_v<decltype(&T::serialize), const T &, CordOutputBinaryStream &> &&
            std::is_invocable_v<decltype(&T::to_string), const T &>,
            turbo::Status> {
            return *this << value.to_string();
        }


        // -------------------------------------------------------------------------
        //  Utilities
        // -------------------------------------------------------------------------

        /// Flushes the stream (no-op; data is written immediately).
        void flush() noexcept;

        /// Returns the total number of bytes written so far.
        [[nodiscard]] size_t bytes_written() const noexcept;

        /// Returns a read-only reference to the underlying cord.
        const CordType &cord() const noexcept;

    private:
        CordType *_cord;
        bool _default_big_endian;
    };

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize>::CordOutputBinaryStream(CordType *cord, bool big_endian) noexcept
        : _cord(cord),
          _default_big_endian(big_endian) {
        // The temporary endian flag is initialized to the default value,
        // and its active state is not tracked; manipulator behavior may be limited.
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::reset(CordType *cord) noexcept {
        if (cord) {
            _cord = cord;
        }
        _cord->clear();
        return *this;
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::write(const void *data, size_t size) {
        if (_cord) {
            _cord->append(data, size).ignore_error();
        }
        return *this;
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::write(uint8_t byte) {
        return write(&byte, 1);
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(BigEndian) {
        _default_big_endian = true;
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(LittleEndian) {
        _default_big_endian = false;
        return *this;
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(uint8_t v) {
        return write(v);
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(int8_t v) {
        return write(static_cast<uint8_t>(v));
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(uint16_t v) {
        auto encoded = EndianFormat::encode(v, _default_big_endian);
        return write(&encoded, sizeof(v));
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(int16_t v) {
        auto encoded = EndianFormat::encode(v, _default_big_endian);
        return write(&encoded, sizeof(v));
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(uint32_t v) {
        auto encoded = EndianFormat::encode(v, _default_big_endian);
        return write(&encoded, sizeof(v));
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(int32_t v) {
        auto encoded = EndianFormat::encode(v, _default_big_endian);
        return write(&encoded, sizeof(v));
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(uint64_t v) {
        auto encoded = EndianFormat::encode(v, _default_big_endian);
        return write(&encoded, sizeof(v));
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(int64_t v) {
        auto encoded = EndianFormat::encode(v, _default_big_endian);
        return write(&encoded, sizeof(v));
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(turbo::uint128 v) {
        std::array<uint64_t, 2> v128{
            EndianFormat::encode(turbo::uint128_low64(v), _default_big_endian),
            EndianFormat::encode(turbo::uint128_high64(v), _default_big_endian)
        };
        write(v128.data(), sizeof(v128));
        return *this;
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(turbo::int128 v) {
        // Treat as unsigned for binary representation.
        return *this << static_cast<turbo::uint128>(v);
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(float v) {
        uint32_t tmp;
        std::memcpy(&tmp, &v, sizeof(v));
        auto encoded = EndianFormat::encode(tmp, _default_big_endian);
        return write(&encoded, sizeof(encoded));
    }


    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(double v) {
        uint64_t tmp;
        std::memcpy(&tmp, &v, sizeof(v));
        auto encoded = EndianFormat::encode(tmp, _default_big_endian);
        return write(&encoded, sizeof(encoded));
    }

    template<size_t Alignment, size_t BlockSize>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(std::string_view sv) {
        return write(sv.data(), sv.size());
    }

    /// Writes raw bytes from a span of trivially copyable type T (no length prefix, no endian conversion).
    /// @tparam T The element type, must be trivially copyable.
    /// @param bytes The span to write.
    /// @return Reference to this stream.
    template<size_t Alignment, size_t BlockSize>
    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T> > >
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(turbo::span<const T> bytes) {
        return write(bytes.data(), bytes.size() * sizeof(T));
    }

    /// Flushes the stream (no-op; data is written immediately).
    template<size_t Alignment, size_t BlockSize>
    void CordOutputBinaryStream<Alignment, BlockSize>::flush() noexcept {
        // Nothing to flush: data is appended to cord immediately.
    }

    /// Returns the total number of bytes written so far.
    template<size_t Alignment, size_t BlockSize>
    size_t CordOutputBinaryStream<Alignment, BlockSize>::bytes_written() const noexcept {
        return _cord ? _cord->size() : 0;
    }

    /// Returns a read-only reference to the underlying cord.
    /// The cord must be non-null.
    template<size_t Alignment, size_t BlockSize>
    const typename CordOutputBinaryStream<Alignment, BlockSize>::CordType &
    CordOutputBinaryStream<Alignment, BlockSize>::cord() const noexcept {
        return *_cord;
    }

    template<size_t Alignment, size_t BlockSize>
    template<typename T>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(const WithContainerSize<T> &wrapper) {
        const auto &container = wrapper.get();
        auto size = static_cast<uint64_t>(container.size());
        *this << size;
        for (const auto &elem: container) {
            *this << elem;
        }
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(const CordBufferBase<OA, OB> &wrapper) {
        auto is = wrapper.input_stream();
        const void *data;
        int size;
        while (is.next(&data, &size)) {
            write(data, static_cast<size_t>(size));
        }
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t A, size_t B>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(const ShareCord<A, B> &wrapper) {
        if constexpr (A == Alignment && B == BlockSize) {
            _cord->append(wrapper.get());
        } else {
            auto is = wrapper.input_stream();
            const void *data;
            int size;
            while (is.next(&data, &size)) {
                write(data, static_cast<size_t>(size));
            }
        }

        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA, size_t OB>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(CordBufferBase<OA, OB> &&wrapper) {
        if constexpr (OA == Alignment && OB == BlockSize) {
            // Same parameters: move the entire cord (zero-copy)
            _cord->append(std::move(wrapper));
        } else {
            // Different parameters: copy data chunk by chunk
            auto is = wrapper.input_stream();
            const void *data;
            int size;
            while (is.next(&data, &size)) {
                write(data, static_cast<size_t>(size));
            }
        }
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(const Buffer<char, OA> &wrapper) {
        return write(wrapper.data(), wrapper.size());
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(Buffer<char, OA> &&wrapper) {
        // If the buffer's alignment matches, we can directly take ownership.
        if constexpr (OA == Alignment) {
            if (wrapper.capacity() == BlockSize) {
                auto buf = std::make_shared<Buffer<char, OA> >(std::move(wrapper));
                _cord->append(std::move(buf));
                return *this;
            }
        } // Different alignment: cannot take ownership, must copy.
        write(wrapper.data(), wrapper.size());
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(BasicString<char, OA> &&wrapper) {
        // If the buffer's alignment matches, we can directly take ownership.
        if (wrapper.empty()) {
            return *this;
        }
        if constexpr (OA == Alignment) {
            if (wrapper.capacity() == BlockSize) {
                auto buf = std::make_shared<Buffer<char, OA> >();
                size_t size;
                size_t capacity;
                auto b = wrapper.seize(&size, &capacity);
                buf->bestow(b, size, capacity + 1);
                _cord->append(std::move(buf));
                return *this;
            }
        } // Different alignment: cannot take ownership, must copy.
        write(wrapper.data(), wrapper.size());
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    template<size_t OA>
    CordOutputBinaryStream<Alignment, BlockSize> &
    CordOutputBinaryStream<Alignment, BlockSize>::operator<<(Vector<char, OA> &&wrapper) {
        if (wrapper.empty()) {
            return *this;
        }
        if constexpr (OA == Alignment) {
            if (wrapper.capacity() == BlockSize) {
                auto buf = std::make_shared<Buffer<char, OA> >();
                size_t size;
                size_t capacity;
                void *data = wrapper.seize(&size, &capacity);
                buf->bestow(static_cast<char *>(data), size, capacity);
                _cord->append(std::move(buf));
                return *this;
            }
        }
        write(wrapper.data(), wrapper.size());
        return *this;
    }
} // namespace fermat
