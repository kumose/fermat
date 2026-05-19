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

#include <fermat/container/stl.h>
#include <fermat/container/string.h>
#include <fermat/container/vector.h>
#include <fermat/container/buffer.h>
#include <string>

namespace fermat {
    /// Abstract interface for receiving a stream of bytes.
    /// Used for deserialization, IO reading, and dynamic buffer writing.
    class Receiver {
    public:
        virtual ~Receiver() = default;

        /// Clear all stored data and reset the receiver state.
        virtual void clear() noexcept = 0;

        /// Resize the underlying container to hold exactly @p n elements.
        /// @param n The new size in bytes
        /// @return OkStatus() on success; error status on failure
        virtual turbo::Status resize(size_t n) = 0;

        /// Reserve space for at least @p n elements to avoid reallocations.
        /// @param n The number of bytes to reserve
        /// @return OkStatus() on success; error status on failure
        virtual turbo::Status reserve(size_t n) = 0;

        /// Append a block of data to the receiver.
        /// @param data Pointer to the input bytes
        /// @param len Number of bytes to append
        /// @return OkStatus() on success; error status on invalid input or failure
        virtual turbo::Status append(const char *data, size_t len) = 0;

        /// Check if the receiver supports dynamic memory growth.
        /// @return true if the container can grow dynamically
        [[nodiscard]] virtual bool is_dynamic() const noexcept = 0;

        /// Get the current data size in bytes.
        /// @return Number of bytes stored
        [[nodiscard]] virtual size_t size() const noexcept = 0;

        /// Get the current allocated capacity in bytes.
        /// @return Allocated buffer capacity
        [[nodiscard]] virtual size_t capacity() const noexcept = 0;
    };


    /// Type trait to determine if a type is a contiguous, string-like receiver container.
    /// Provides an alignment hint for optimized memory operations.
    template<typename T>
    struct is_contiguous_string_receiver : std::false_type {
        static constexpr size_t kAlignment = 0;
    };

    /// Specialization for std::string
    template<>
    struct is_contiguous_string_receiver<std::string> : std::true_type {
        static constexpr size_t kAlignment = 0;
    };

    /// Specialization for fermat::BasicString<char, Alignment>
    template<size_t Alignment>
    struct is_contiguous_string_receiver<BasicString<char, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for fermat::AlignedString<Alignment>
    template<size_t Alignment>
    struct is_contiguous_string_receiver<AlignedString<Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    template<typename Container, typename Enabler = void>
    class ContainerAppender;

    template<typename Container, typename Enabler = void>
    class ContainerReceiver;


    /// Specialization for contiguous string-like containers (std::string, BasicString, AlignedString)
    template<typename Container>
    class ContainerAppender<
                Container,
                std::enable_if_t<is_contiguous_string_receiver<Container>::value>
            > : public Receiver {
    public:
        /// Construct a ContainerAppender from a reference to the target container.
        /// @param c Container to receive data
        explicit ContainerAppender(Container &c)
            : _c(c) {
        }

        /// @copydoc Receiver::clear
        void clear() noexcept override {
            _c.clear();
        }

        /// @copydoc Receiver::resize
        turbo::Status resize(size_t n) override {
            try {
                _c.resize(n);
                return turbo::OkStatus();
            } catch (...) {
                return turbo::resource_exhausted_error("Container resize failed");
            }
        }

        /// @copydoc Receiver::reserve
        turbo::Status reserve(size_t n) override {
            try {
                _c.reserve(n);
                return turbo::OkStatus();
            } catch (...) {
                return turbo::resource_exhausted_error("Container reserve failed");
            }
        }

        /// @copydoc Receiver::append
        turbo::Status append(const char *data, size_t len) override {
            if (!data) {
                return turbo::invalid_argument_error("Data pointer is null");
            }
            if (len == 0) {
                return turbo::OkStatus();
            }

            try {
                _c.append(data, len);
                return turbo::OkStatus();
            } catch (...) {
                return turbo::resource_exhausted_error("Container append failed");
            }
        }

        /// @copydoc Receiver::is_dynamic
        [[nodiscard]] bool is_dynamic() const noexcept override {
            return true;
        }

        /// @copydoc Receiver::size
        [[nodiscard]] size_t size() const noexcept override {
            return _c.size();
        }

        /// @copydoc Receiver::capacity
        [[nodiscard]] size_t capacity() const noexcept override {
            return _c.capacity();
        }

    private:
        Container &_c; ///< Reference to the underlying container
    };

    /// ContainerReceiver (owns the container) for string receivers
    template<typename Container>
    class ContainerReceiver<Container, std::enable_if_t<is_contiguous_string_receiver<
                Container>::value> > : public Receiver {
    public:
        /// Default constructor
        ContainerReceiver() = default;

        /// Construct with a moved container
        explicit ContainerReceiver(Container &&c) : storage(std::move(c)) {
        }

        /// @copydoc Receiver::clear
        void clear() noexcept override {
            storage.clear();
        }

        /// @copydoc Receiver::resize
        turbo::Status resize(size_t n) override {
            storage.resize(n);
            return turbo::OkStatus();
        }

        /// @copydoc Receiver::reserve
        turbo::Status reserve(size_t n) override {
            storage.reserve(n);
            return turbo::OkStatus();
        }

        /// @copydoc Receiver::append
        turbo::Status append(const char *data, size_t len) override {
            if (!data) return turbo::invalid_argument_error("nullptr");
            if (len == 0) return turbo::OkStatus();
            storage.append(data, len);
            return turbo::OkStatus();
        }

        /// @copydoc Receiver::is_dynamic
        [[nodiscard]] bool is_dynamic() const noexcept override {
            return true;
        }

        /// @copydoc Receiver::size
        [[nodiscard]] size_t size() const noexcept override {
            return storage.size();
        }

        /// @copydoc Receiver::capacity
        [[nodiscard]] size_t capacity() const noexcept override {
            return storage.capacity();
        }

        /// Release ownership of the underlying container
        Container release() {
            return std::move(storage);
        }

        /// Get const reference to the underlying container
        const Container &container() const noexcept {
            return storage;
        }

    public:
        Container storage; ///< Public container for direct user access
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////
    /// vector like
    /// Type trait to determine if a type is a contiguous byte/int8 vector container
    /// that supports receiver operations.
    template<typename T>
    struct is_contiguous_vector_receiver : std::false_type {
        static constexpr size_t kAlignment = 0;
    };

    /// Specialization for std::vector<char>
    template<>
    struct is_contiguous_vector_receiver<std::vector<char> > : std::true_type {
        static constexpr size_t kAlignment = 0;
    };

    /// Specialization for std::vector<int8_t>
    template<>
    struct is_contiguous_vector_receiver<std::vector<int8_t> > : std::true_type {
        static constexpr size_t kAlignment = 0;
    };

    /// Specialization for std::vector<uint8_t>
    template<>
    struct is_contiguous_vector_receiver<std::vector<uint8_t> > : std::true_type {
        static constexpr size_t kAlignment = 0;
    };

    /// Specialization for AlignedVector<char>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<AlignedVector<char, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for AlignedVector<int8_t>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<AlignedVector<int8_t, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for AlignedVector<uint8_t>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<AlignedVector<uint8_t, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for Buffer<char>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<Buffer<char, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for Buffer<int8_t>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<Buffer<int8_t, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for Buffer<uint8_t>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<Buffer<uint8_t, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for Vector<char>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<Vector<char, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for Vector<int8_t>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<Vector<int8_t, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    /// Specialization for Vector<uint8_t>
    template<size_t Alignment>
    struct is_contiguous_vector_receiver<Vector<uint8_t, Alignment> > : std::true_type {
        static constexpr size_t kAlignment = Alignment;
    };

    // ContainerAppender for vector receivers
    /// ContainerAppender specialization for contiguous vector containers (non-owning)
    template<typename Container>
    class ContainerAppender<Container, std::enable_if_t<is_contiguous_vector_receiver<
                Container>::value> > : public Receiver {
    public:
        /// Construct with a reference to the target container
        explicit ContainerAppender(Container &c) : _c(c) {
        }

        /// Clear all contents
        void clear() noexcept override {
            _c.clear();
        }

        /// Resize the container
        turbo::Status resize(size_t n) override {
            _c.resize(n);
            return turbo::OkStatus();
        }

        /// Reserve capacity
        turbo::Status reserve(size_t n) override {
            _c.reserve(n);
            return turbo::OkStatus();
        }

        /// Append raw bytes with high performance
        turbo::Status append(const char *data, size_t len) override {
            if (!data) {
                return turbo::invalid_argument_error("nullptr");
            }
            if (len == 0) {
                return turbo::OkStatus();
            }

            const size_t old_size = _c.size();
            _c.resize(old_size + len);
            std::memcpy(_c.data() + old_size, data, len);

            return turbo::OkStatus();
        }

        /// Check if dynamic
        [[nodiscard]] bool is_dynamic() const noexcept override {
            return true;
        }

        /// Current size
        [[nodiscard]] size_t size() const noexcept override {
            return _c.size();
        }

        /// Current capacity
        [[nodiscard]] size_t capacity() const noexcept override {
            return _c.capacity();
        }

    private:
        Container &_c;
    };


    /// ContainerReceiver for vector receivers
    template<typename Container>
    class ContainerReceiver<Container, std::enable_if_t<is_contiguous_vector_receiver<
                Container>::value> > : public Receiver {
    public:
        ContainerReceiver() = default;

        explicit ContainerReceiver(Container &&c) : storage(std::move(c)) {
        }

        void clear() noexcept override {
            storage.clear();
        }

        turbo::Status resize(size_t n) override {
            storage.resize(n);
            return turbo::OkStatus();
        }

        turbo::Status reserve(size_t n) override {
            storage.reserve(n);
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) return turbo::invalid_argument_error("nullptr");
            if (len == 0) return turbo::OkStatus();

            // FAST PATH: resize + memcpy (no insert)
            size_t old_size = storage.size();
            storage.resize(old_size + len);
            std::memcpy(storage.data() + old_size, data, len);
            return turbo::OkStatus();
        }

        [[nodiscard]] bool is_dynamic() const noexcept override { return true; }
        [[nodiscard]] size_t size() const noexcept override { return storage.size(); }
        [[nodiscard]] size_t capacity() const noexcept override { return storage.capacity(); }

        Container release() { return std::move(storage); }
        const Container &container() const noexcept { return storage; }

    public:
        Container storage;
    };
} // namespace fermat
