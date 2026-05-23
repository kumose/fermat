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
#include <fermat/container/cord_output_stream.h>
#include <fermat/container/vector_set.h>
#include <fermat/container/receiver.h>
#include <istream>
#include <streambuf>
#include <cstring>

namespace fermat {
    /// @brief Stream buffer that reads data from a CordBufferBase using its InputStream.
    template<size_t Alignment, size_t BlockSize>
    class CordBufferInputStreambuf : public std::streambuf {
    public:
        /// Constructs a streambuf to read from the given cord.
        /// @param cord The cord to read from (must outlive the streambuf).
        explicit CordBufferInputStreambuf(const CordBufferBase<Alignment, BlockSize> *cord)
            : _input(cord), _cur_block_data(nullptr), _cur_block_size(0) {
            // Initialize empty buffer; underflow will fetch the first chunk.
        }

        ~CordBufferInputStreambuf() override = default;

    protected:
        /// Called when the get area is empty and more characters are needed.
        int underflow() override {
            if (gptr() && gptr() < egptr()) {
                // Already have data in buffer.
                return traits_type::to_int_type(*gptr());
            }

            // Fetch the next chunk from the InputStream.
            const void *data = nullptr;
            int size = 0;
            if (!_input.next(&data, &size) || size == 0) {
                // End of stream.
                return traits_type::eof();
            }

            // Set the buffer pointers to point to the chunk's data.
            _cur_block_data = static_cast<const char *>(data);
            _cur_block_size = static_cast<size_t>(size);
            setg(const_cast<char *>(_cur_block_data),
                 const_cast<char *>(_cur_block_data),
                 const_cast<char *>(_cur_block_data + _cur_block_size));
            return traits_type::to_int_type(*gptr());
        }

        /// Called when a character is read and the buffer is advanced.
        int uflow() override {
            if (underflow() == traits_type::eof()) {
                return traits_type::eof();
            }
            // Advance the get pointer and return the character.
            return traits_type::to_int_type(*gptr());
            // Note: gptr() will be incremented by the caller? Actually underflow sets get area,
            // and uflow should consume one character. We manually advance.
        }

        /// Supports putback (unget) of a single character.
        int pbackfail(int ch) override {
            if (gptr() > eback()) {
                // There is room to putback into the buffer.
                gbump(-1);
                if (ch != traits_type::eof()) {
                    *gptr() = traits_type::to_char_type(ch);
                }
                return ch;
            }
            // Need to putback to a previous block using InputStream::back_up.
            // Note: gptr() is at the beginning of the current block; we need to go back one byte.
            if (_cur_block_data && gptr() == eback()) {
                // Try to back up one byte in the underlying stream.
                // The last chunk consumed was of size _cur_block_size.
                // We must back up the entire current block plus one byte? Actually we only need to back up 1 byte.
                // But InputStream::back_up expects a count that does not exceed the last returned chunk size.
                // The last chunk returned by next() was the current block. We can back up 1 byte.
                _input.back_up(1);
                // After backing up, the next next() will return the same block again (or previous).
                // However, we also need to reposition the buffer to reflect that new position.
                // Simpler approach: clear the buffer and let underflow re-fetch.
                setg(nullptr, nullptr, nullptr);
                _cur_block_data = nullptr;
                _cur_block_size = 0;
                if (underflow() == traits_type::eof()) {
                    return traits_type::eof();
                }
                // Move to the last character of the buffer (the one we backed into).
                gbump(egptr() - gptr() - 1);
                if (ch != traits_type::eof()) {
                    *gptr() = traits_type::to_char_type(ch);
                }
                return ch;
            }
            return traits_type::eof();
        }

    private:
        mutable typename CordBufferBase<Alignment, BlockSize>::InputStream _input;
        const char *_cur_block_data;
        size_t _cur_block_size;
    };


    /// @brief An input stream that reads formatted data from a CordBufferBase.
    ///
    /// This class inherits std::istream and uses CordBufferInputStreambuf to read
    /// data from the underlying cord. It supports all standard istream operations
    /// such as operator>>, getline, etc.
    ///
    /// Example:
    ///   CordBufferBase<64, 4096> cord;
    ///   // ... fill cord with data ...
    ///   CordInputStringStream<64, 4096> iss(&cord);
    ///   std::string s;
    ///   iss >> s;
    template<size_t Alignment, size_t BlockSize>
    class CordInputStringStream : public std::istream {
    public:
        using CordType = CordBufferBase<Alignment, BlockSize>;

        /// Constructs an input stream that reads from the provided cord.
        /// The cord must remain valid for the lifetime of this stream.
        explicit CordInputStringStream(const CordType *cord)
            : std::istream(&_streambuf),
              _streambuf(cord) {
        }

        ~CordInputStringStream() override = default;

        // Disable copy, allow move.
        CordInputStringStream(const CordInputStringStream &) = delete;

        CordInputStringStream &operator=(const CordInputStringStream &) = delete;

        CordInputStringStream(CordInputStringStream &&) noexcept = default;

        CordInputStringStream &operator=(CordInputStringStream &&) noexcept = default;

    private:
        CordBufferInputStreambuf<Alignment, BlockSize> _streambuf;
    };

    /// Binary input stream that reads from a CordBufferBase.
/// Supports fixed‑width integers, floating point, raw bytes, and strings.
/// Endianness can be controlled via manipulators:
///   stream >> BigEndian{};   // subsequent reads use big-endian
///   stream >> LittleEndian{}; // subsequent reads use little-endian
/// The manipulator affects all following reads until changed.
/// Arithmetic read operations return bool to indicate success (true) or failure (false).
    template<size_t Alignment, size_t BlockSize>
    class CordInputBinaryStream {
    public:
        using CordType = CordBufferBase<Alignment, BlockSize>;

        // -------------------------------------------------------------------------
        //  Construction
        // -------------------------------------------------------------------------

        /// Constructor.
        /// @param cord The cord to read from (must outlive the stream).
        /// @param big_endian Default endianness (true = big, false = little).
        explicit CordInputBinaryStream(const CordType *cord, bool big_endian = false) noexcept;

        ~CordInputBinaryStream() = default;

        // Disable copy, allow move.
        CordInputBinaryStream(const CordInputBinaryStream &) = delete;

        CordInputBinaryStream &operator=(const CordInputBinaryStream &) = delete;

        CordInputBinaryStream(CordInputBinaryStream &&) = default;

        CordInputBinaryStream &operator=(CordInputBinaryStream &&) = default;

        // -------------------------------------------------------------------------
        //  Raw byte reading (return true if exactly 'size' bytes read)
        // -------------------------------------------------------------------------

        /// Reads a sequence of raw bytes.
        size_t read(void *buffer, size_t size);

        /// Reads a single byte.
        bool read(uint8_t *out);

        /// Reads a sequence of raw bytes (char overload).
        size_t read(char *buffer, size_t size) {
            return read(static_cast<void *>(buffer), size);
        }

        size_t read(Receiver &buffer, size_t size);

        // -------------------------------------------------------------------------
        //  Endianness manipulators (affect all subsequent reads, return stream&)
        // -------------------------------------------------------------------------

        /// Temporarily enables big‑endian for the next arithmetic value.
        CordInputBinaryStream &operator<<(BigEndian);

        /// Temporarily enables little‑endian for the next arithmetic value.
        CordInputBinaryStream &operator<<(LittleEndian);

        // -------------------------------------------------------------------------
        //  Arithmetic reads (return bool: true if successfully read and converted)
        // -------------------------------------------------------------------------

        bool read(uint8_t &v);

        bool read(int8_t &v);

        bool read(uint16_t &v);

        bool read(int16_t &v);

        bool read(uint32_t &v);

        bool read(int32_t &v);

        bool read(uint64_t &v);

        bool read(int64_t &v);

        bool read(float &v);

        bool read(double &v);

        bool read(turbo::uint128 &v);

        bool read(turbo::int128 &v);

        // -------------------------------------------------------------------------
        //  String and binary data (return bool: true if exactly span.size() bytes read)
        // -------------------------------------------------------------------------

        template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_same_v<T, char>>* = nullptr>
        size_t read(turbo::span<T> span);

        size_t read(turbo::span<char> span);

        // -------------------------------------------------------------------------
        //  Pointer (raw address bytes) – use RawPointer proxy (return bool)
        // -------------------------------------------------------------------------

        template<typename T>
        bool read(RawPointer<T> ptr);

        // -------------------------------------------------------------------------
        //  Container with size prefix (symmetric to WithContainerSize)
        // -------------------------------------------------------------------------

        template<typename T>
        bool read(WithContainerSize<T> &wrapper);

        // -------------------------------------------------------------------------
        //  User‑defined types (requires member function deserialize returning bool)
        // -------------------------------------------------------------------------

        template<typename T>
        bool read(T &value) {
            return value.deserialize(*this);
        }

        ////////////////////////////////////////////////////////////////////////////////
        ///
        ///
        size_t read_util(turbo::span<char> data, char c, bool &reach);

        size_t read_util(turbo::span<char> data, std::string_view chars, bool &reach);

        size_t read_util(turbo::span<char> data, const VectorSet<char> &chars);

        // -------------------------------------------------------------------------
        //  Utilities
        // -------------------------------------------------------------------------

        /// Returns the total number of bytes remaining in the stream.
        [[nodiscard]] size_t bytes_remaining() const noexcept;

        /// Returns a read‑only reference to the underlying cord.
        const CordType &cord() const noexcept;

        /// Checks if the stream is in a good state (no read error).
        [[nodiscard]] bool good() const noexcept;

    private:
        const CordType *_cord;
        bool _default_big_endian;
        /// Current read position.
        typename CordType::InputStream _input;
    };

    template<size_t Alignment, size_t BlockSize>
    CordInputBinaryStream<Alignment, BlockSize>::CordInputBinaryStream(
        const CordType *cord, bool big_endian) noexcept
        : _cord(cord),
          _default_big_endian(big_endian),
          _input(cord) {
    }

    /// Reads up to `size` bytes from the stream into `buffer`.
    /// Returns the number of bytes actually read (0 if no data available, not necessarily end).
    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read(void *buffer, size_t size) {
        if (size == 0) {
            return 0;
        }
        char *dst = static_cast<char *>(buffer);
        size_t remaining = size;
        while (remaining > 0) {
            const void *src = nullptr;
            int chunk_size = 0;
            if (!_input.next(&src, &chunk_size) || chunk_size == 0) {
                // No more data available at this moment (may be appended later).
                break;
            }
            size_t to_copy = std::min(remaining, static_cast<size_t>(chunk_size));
            std::memcpy(dst, src, to_copy);
            dst += to_copy;
            remaining -= to_copy;
            if (to_copy < static_cast<size_t>(chunk_size)) {
                // Consumed only part of the chunk; back up the remainder.
                _input.back_up(static_cast<int>(chunk_size - to_copy));
            }
        }
        return size - remaining;
    }

    /// Reads a single byte.
    /// @param out Output variable.
    /// @return true if a byte was successfully read, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(uint8_t *out) {
        return read(static_cast<void *>(out), 1) == 1;
    }

    template<size_t Alignment, size_t BlockSize>
    CordInputBinaryStream<Alignment, BlockSize> &
    CordInputBinaryStream<Alignment, BlockSize>::operator<<(BigEndian) {
        _default_big_endian = true;
        return *this;
    }

    template<size_t Alignment, size_t BlockSize>
    CordInputBinaryStream<Alignment, BlockSize> &
    CordInputBinaryStream<Alignment, BlockSize>::operator<<(LittleEndian) {
        _default_big_endian = false;
        return *this;
    }

    /// Reads up to `size` bytes from the stream and appends them to the receiver.
    /// Returns the number of bytes actually read (may be less than `size`).
    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read(Receiver &buffer, size_t size) {
        if (size == 0) return 0;
        size_t total = 0;
        const void *data = nullptr;
        int chunk_size = 0;
        while (total < size && _input.next(&data, &chunk_size) && chunk_size > 0) {
            size_t to_take = std::min<size_t>(size - total, static_cast<size_t>(chunk_size));
            auto status = buffer.append(static_cast<const char *>(data), to_take);
            if (!status.ok()) {
                // Append failed; back up the remainder of the current chunk.
                if (to_take < static_cast<size_t>(chunk_size)) {
                    _input.back_up(static_cast<int>(chunk_size - to_take));
                }
                break;
            }
            total += to_take;
            if (to_take < static_cast<size_t>(chunk_size)) {
                // Did not consume the whole chunk; back up the remainder.
                _input.back_up(static_cast<int>(chunk_size - to_take));
                break;
            }
        }
        return total;
    }

    /// Reads a uint8_t value from the stream.
    /// Returns true if successful, false if no data available.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(uint8_t &v) {
        return read(&v, 1) == 1;
    }

    /// Reads an int8_t value from the stream.
    /// Returns true if successful, false if no data available.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(int8_t &v) {
        return read(reinterpret_cast<uint8_t *>(&v), 1) == 1;
    }

    /// Reads a uint16_t value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(uint16_t &v) {
        uint16_t tmp;
        if (read(&tmp, sizeof(tmp)) != sizeof(tmp)) {
            return false;
        }
        v = EndianFormat::decode(tmp, _default_big_endian);
        return true;
    }

    /// Reads an int16_t value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(int16_t &v) {
        return read(reinterpret_cast<uint16_t &>(v));
    }

    /// Reads a uint32_t value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(uint32_t &v) {
        uint32_t tmp;
        if (read(&tmp, sizeof(tmp)) != sizeof(tmp)) {
            return false;
        }
        v = EndianFormat::decode(tmp, _default_big_endian);
        return true;
    }

    /// Reads an int32_t value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(int32_t &v) {
        return read(reinterpret_cast<uint32_t &>(v));
    }

    /// Reads a uint64_t value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(uint64_t &v) {
        uint64_t tmp;
        if (read(&tmp, sizeof(tmp)) != sizeof(tmp)) {
            return false;
        }
        v = EndianFormat::decode(tmp, _default_big_endian);
        return true;
    }

    /// Reads an int64_t value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(int64_t &v) {
        return read(reinterpret_cast<uint64_t &>(v));
    }

    /// Reads a float value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(float &v) {
        uint32_t tmp;
        if (!read(tmp)) {
            return false;
        }
        std::memcpy(&v, &tmp, sizeof(v));
        return true;
    }

    /// Reads a double value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(double &v) {
        uint64_t tmp;
        if (!read(tmp)) {
            return false;
        }
        std::memcpy(&v, &tmp, sizeof(v));
        return true;
    }

    /// Reads a turbo::uint128 value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(turbo::uint128 &v) {
        uint64_t lo, hi;
        if (_default_big_endian) {
            if (!read(hi) || !read(lo)) {
                return false;
            }
        } else {
            if (!read(lo) || !read(hi)) {
                return false;
            }
        }
        v = turbo::make_uint128(hi, lo);
        return true;
    }

    /// Reads a turbo::int128 value from the stream with endian conversion using current default endianness.
    /// Returns true if successful, false otherwise.
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(turbo::int128 &v) {
        turbo::uint128 uv{};
        if (!read(uv)) {
            return false;
        }
        v = static_cast<turbo::int128>(uv);
        return true;
    }

    template<size_t Alignment, size_t BlockSize>
    template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_same_v<T, char>> *>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read(turbo::span<T> span) {
        return read(span.data(), span.size() * sizeof(T));
    }

    /// Reads raw bytes into a span of char.
    /// Returns the number of characters actually read.
    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read(turbo::span<char> span) {
        return read(span.data(), span.size());
    }

    template<size_t Alignment, size_t BlockSize>
    template<typename T>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(RawPointer<T> ptr) {
        void *p = const_cast<void *>(static_cast<const void *>(ptr.get()));
        return read(p, sizeof(T *)) == sizeof(T *);
    }

    /// Returns the total number of bytes remaining in the stream.
    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::bytes_remaining() const noexcept {
        return _input.remain();
    }

    /// Returns a read‑only reference to the underlying cord.
    template<size_t Alignment, size_t BlockSize>
    const typename CordInputBinaryStream<Alignment, BlockSize>::CordType &
    CordInputBinaryStream<Alignment, BlockSize>::cord() const noexcept {
        return *_cord;
    }

    /// Checks if the stream is in a good state (no read error).
    template<size_t Alignment, size_t BlockSize>
    bool CordInputBinaryStream<Alignment, BlockSize>::good() const noexcept {
        return true; // No error flag present; always true.
    }

    template<size_t Alignment, size_t BlockSize>
    template<typename T>
    bool CordInputBinaryStream<Alignment, BlockSize>::read(WithContainerSize<T> &wrapper) {
        auto &container = wrapper.get();
        container.clear();

        uint64_t size = 0;
        if (!read(size)) {
            return false;
        }

        for (uint64_t i = 0; i < size; ++i) {
            typename T::value_type elem;
            if (!read(elem)) {
                return false;
            }
            container.push_back(std::move(elem));
        }
        return true;
    }

    /// Reads data into the given span until delimiter char is found or span is filled.
/// The delimiter is consumed but not stored.
/// @param data Destination span.
/// @param c Delimiter character.
/// @param reach Set to true if delimiter was encountered (and consumed).
/// @return Number of bytes written into data.
    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read_util(turbo::span<char> data, char c, bool &reach) {
        reach = false;
        size_t written = 0;
        while (written < data.size()) {
            const void *chunk = nullptr;
            int chunk_size = 0;
            if (!_input.next(&chunk, &chunk_size) || chunk_size == 0) {
                break; // No more data
            }
            const char *ptr = static_cast<const char *>(chunk);
            const char *found = static_cast<const char *>(std::memchr(ptr, c, chunk_size));
            size_t available = found ? static_cast<size_t>(found - ptr) : static_cast<size_t>(chunk_size);
            size_t to_copy = std::min(available, data.size() - written);
            if (to_copy > 0) {
                std::memcpy(data.data() + written, ptr, to_copy);
                written += to_copy;
            }
            // Consume the processed part plus possibly the delimiter
            size_t consumed = to_copy;
            if (found && (to_copy == available)) {
                // Delimiter was exactly at the end of copied part or before? Actually found indicates delimiter location.
                // If we copied up to (found - ptr) bytes, then delimiter is at ptr + to_copy.
                // We need to consume it.
                consumed = to_copy + 1;
                reach = true;
            }
            if (consumed < static_cast<size_t>(chunk_size)) {
                // Not all of the chunk was consumed; back up the remainder.
                _input.back_up(static_cast<int>(chunk_size - consumed));
                // If we haven't yet set reach and we stopped because of space, break.
                if (!reach) break;
                // else if reach is true, we are done anyway.
            }
            if (reach) break;
        }
        return written;
    }

    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read_util(turbo::span<char> data,
                                                                  std::string_view chars,
                                                                  bool &reach) {
        reach = false;
        size_t written = 0;
        while (written < data.size()) {
            const void *chunk = nullptr;
            int chunk_size = 0;
            if (!_input.next(&chunk, &chunk_size) || chunk_size == 0) break;

            const char *ptr = static_cast<const char *>(chunk);
            std::string_view chunk_view(ptr, chunk_size);
            size_t delim_pos = chunk_view.find_first_of(chars);
            size_t copy_limit = (delim_pos == std::string_view::npos) ? chunk_size : delim_pos;
            size_t to_write = std::min(copy_limit, data.size() - written);
            if (to_write > 0) {
                std::memcpy(data.data() + written, ptr, to_write);
                written += to_write;
            }
            size_t consumed = to_write;
            if (delim_pos != std::string_view::npos) {
                consumed = to_write + 1; // include delimiter
                reach = true;
            }
            if (consumed < static_cast<size_t>(chunk_size)) {
                _input.back_up(static_cast<int>(chunk_size - consumed));
            }
            if (reach) break;
        }
        return written;
    }

    /// Reads data until any character from the given VectorSet is encountered.
    /// Converts VectorSet to string_view and calls the above overload.
    /// @param data Destination span (not modified).
    /// @param chars Set of delimiter characters.
    /// @return Number of bytes written.
    template<size_t Alignment, size_t BlockSize>
    size_t CordInputBinaryStream<Alignment, BlockSize>::read_util(turbo::span<char> data,
                                                                  const VectorSet<char> &chars) {
        bool dummy_reach;
        std::string_view chars_view(reinterpret_cast<const char *>(chars.data()), chars.size());
        return read_util(data, chars_view, dummy_reach);
    }
} // namespace fermat
