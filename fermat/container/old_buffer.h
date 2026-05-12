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

#include <string>
#include <fermat/memory/allocator.h>

namespace fermat {
    template<typename T, size_t Alignment>
    class OldBuffer {
    public:
        OldBuffer() = default;

        ~OldBuffer() {
            if (data) {
                AlignedMalloc<Alignment>::good_free(data);
            }
        }

        void grow(size_t gsize) {
            if (gsize == 0) {
                return;
            }
            auto n = (gsize + size) * sizeof(T);

            auto ptr = AlignedMalloc<Alignment>::good_alloc(&n);
            if (!ptr) throw std::bad_alloc();
            if (data && size > 0) {
                memcpy(ptr, data, size * sizeof(T));
            }
            data = reinterpret_cast<T *>(ptr);
            size = n / sizeof(T);
        }

        turbo::span<T> seize() {
            turbo::span<T> span(data, size);
            data = nullptr;
            size = 0;
            return span;
        }

        T *data{nullptr};
        size_t size{0};
    };


    /// @brief A growable buffer for trivially copyable types, designed for data passing.
    /// @tparam T Element type (must be trivially copyable).
    /// @tparam Alignment Required alignment (power of two).
    template<typename T, size_t Alignment = kDefaultAlignedSize>
    class OldABuffer {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

    public:
        // ----- type aliases -----
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using iterator = T *;
        using const_iterator = const T *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        // ----- constructors / destructor -----
        OldABuffer() noexcept = default;

        OldABuffer(size_type n) { resize(n); }
        OldABuffer(size_type n, const T &val) { assign(n, val); }
        OldABuffer(std::initializer_list<T> init) { assign(init.begin(), init.size()); }

        template<class InputIt>
        OldABuffer(InputIt first, InputIt last) { assign(first, last); }

        OldABuffer(const OldABuffer &) = delete; // no copy (ownership semantics)

        virtual ~OldABuffer() noexcept = default;

        OldABuffer &operator=(const OldABuffer &) = delete;


        OldABuffer(OldABuffer &&other) noexcept
            : _buffer(other._buffer), _size(other._size) {
            other._buffer.data = nullptr;
            other._buffer.size = 0;
            other._size = 0;
        }

        OldABuffer &operator=(OldABuffer &&other) noexcept {
            if (this != &other) {
                if (_buffer.data) AlignedMalloc<Alignment>::good_free(_buffer.data);
                _buffer = other._buffer;
                _size = other._size;
                other._buffer.data = nullptr;
                other._buffer.size = 0;
                other._size = 0;
            }
            return *this;
        }


        // ----- iterators -----
        iterator begin() noexcept { return _buffer.data; }
        const_iterator begin() const noexcept { return _buffer.data; }
        const_iterator cbegin() const noexcept { return _buffer.data; }
        iterator end() noexcept { return _buffer.data + _size; }
        const_iterator end() const noexcept { return _buffer.data + _size; }
        const_iterator cend() const noexcept { return _buffer.data + _size; }
        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }


        // ----- element access -----
        reference operator[](size_type pos) { return _buffer.data[pos]; }
        const_reference operator[](size_type pos) const { return _buffer.data[pos]; }

        reference at(size_type pos) {
            if (pos >= _size) throw std::out_of_range("OldABuffer::at");
            return _buffer.data[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= _size) throw std::out_of_range("OldABuffer::at");
            return _buffer.data[pos];
        }

        reference front() { return _buffer.data[0]; }
        const_reference front() const { return _buffer.data[0]; }
        reference back() { return _buffer.data[_size - 1]; }
        const_reference back() const { return _buffer.data[_size - 1]; }
        T *data() noexcept { return _buffer.data; }
        const T *data() const noexcept { return _buffer.data; }

        // ----- capacity -----
        size_type size() const noexcept { return _size; }
        size_type capacity() const noexcept { return _buffer.size; }
        bool empty() const noexcept { return _size == 0; }

        void reserve(size_type n) {
            if (n <= capacity()) return;
            ensure_capacity(n - _size);
        }

        // ----- modifiers (only append/pop, no insert/erase in middle) -----
        void clear() noexcept { _size = 0; }

        void assign(size_type count, const T &value) {
            resize(count);
            for (size_type i = 0; i < count; ++i) _buffer.data[i] = value;
        }

        void assign(const T *first, const T *last) {
            size_type len = last - first;
            resize(len);
            std::memcpy(_buffer.data, first, len * sizeof(T));
        }

        void assign(std::initializer_list<T> ilist) {
            resize(ilist.size());
            std::memcpy(_buffer.data, ilist.begin(), ilist.size() * sizeof(T));
        }

        template<class InputIt>
        void assign(InputIt first, InputIt last) {
            static_assert(std::is_same_v<typename std::iterator_traits<InputIt>::value_type, T>,
                          "Incompatible iterator");
            size_type len = std::distance(first, last);
            resize(len);
            for (size_type i = 0; i < len; ++i) _buffer.data[i] = *first++;
        }

        void push_back(const T &value) { append(value); }
        void push_back(T &&value) { append(std::move(value)); }

        template<class... Args>
        reference emplace_back(Args &&... args) {
            // For trivially copyable, emplace is same as push_back; we construct in place.
            ensure_capacity(1);
            ::new(static_cast<void *>(_buffer.data + _size)) T(std::forward<Args>(args)...);
            ++_size;
            return back();
        }

        void resize(size_type n) {
            if (n > capacity()) reserve(n);
            if (n > _size) {
                std::memset(_buffer.data + _size, 0, (n - _size) * sizeof(T));
            }
            _size = n;
        }

        void swap(OldABuffer &other) noexcept {
            using std::swap;
            swap(_buffer, other._buffer);
            swap(_size, other._size);
        }

        // ----- memory ownership transfer -----
        turbo::span<T> seize() {
            auto span = _buffer.seize();
            _size = 0;
            return span;
        }

        // ----- comparison operators (non‑member) -----
        friend bool operator==(const OldABuffer &a, const OldABuffer &b) {
            if (a.size() != b.size()) return false;
            return std::memcmp(a.data(), b.data(), a.size() * sizeof(T)) == 0;
        }

        friend bool operator!=(const OldABuffer &a, const OldABuffer &b) { return !(a == b); }

        friend bool operator<(const OldABuffer &a, const OldABuffer &b) {
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
        }

        friend bool operator>(const OldABuffer &a, const OldABuffer &b) { return b < a; }
        friend bool operator<=(const OldABuffer &a, const OldABuffer &b) { return !(a > b); }
        friend bool operator>=(const OldABuffer &a, const OldABuffer &b) { return !(a < b); }


        /// @brief Append a single element (copy).
        void append(const T &t) {
            ensure_capacity(1);
            _buffer.data[_size] = t;
            ++_size;
        }

        /// @brief Append a range of elements (copy).
        void append(const T *array, size_t n) {
            if (n == 0) return;
            ensure_capacity(n);
            std::memcpy(_buffer.data + _size, array, n * sizeof(T));
            _size += n;
        }

        /// @brief Append a single element (move, same as copy for trivial types).
        void append(T &&t) {
            ensure_capacity(1);
            _buffer.data[_size] = std::move(t);
            ++_size;
        }

        /// @brief Append a span of elements.
        void append(turbo::span<T> span) { append(span.data(), span.size()); }

        /// @brief Remove the last `n` elements (no destructor needed).
        void pop_back(size_t n = 1) noexcept {
            if (n > _size) {
                _size = 0;
            } else {
                _size -= n;
            }
        }

        std::enable_if_t<std::is_same_v<char, T> || std::is_same_v<char16_t, T> || std::is_same_v<char32_t, T>, const T
            *> stringify() {
            if (_size == 0) return reinterpret_cast<const T *>(&kNuller);
            reserve(_size +1);
            _buffer.data[_size] = T{0};
            return reinterpret_cast<const T *>(&_buffer.data[0]);
        }

        std::enable_if_t<std::is_same_v<char, T> || std::is_same_v<char16_t, T> || std::is_same_v<char32_t, T>, std::basic_string_view<T>> to_string_view() const {
            if (_size == 0) return  {};
            return {reinterpret_cast<const T *>(&_buffer.data[0]), _size};
        }

        std::enable_if_t<std::is_same_v<char, T> || std::is_same_v<char16_t, T> || std::is_same_v<char32_t, T>, std::basic_string<T>> to_string() const {
            if (_size == 0) return  {};
            return {reinterpret_cast<const T *>(&_buffer.data[0]), _size};
        }

        turbo::span<const T> to_span() const {
            if (_size == 0) return {};
            return {reinterpret_cast<const T *>(&_buffer.data[0]), _size};
        }

        turbo::span<T> mutable_span() {
            if (_size == 0) return {};
            return {reinterpret_cast<T *>(&_buffer.data[0]), _size};
        }

    protected:
        static constexpr uint64_t kNuller = '\0';

    protected:
        virtual void ensure_capacity(size_t extra) {
            size_t needed = _size + extra;
            if (needed <= capacity()) return;
            // Growth policy: at least double, but never less than needed
            size_t new_cap = std::max(capacity() * 2, needed);
            _buffer.grow(new_cap - capacity());
        }

    protected:
        OldBuffer<T, Alignment> _buffer;
        size_t _size{0};
    };
} // namespace fermat
