// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
// ... (license header) ...

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>

#include <type_traits>
#include <utility>
#include <fermat/container/construct.h>
#include <fermat/container/utility.h>
#include <fermat/container/traits.h>
#include <fermat/memory/allocator.h>

namespace fermat {
    /// @brief Fixed‑capacity vector with no dynamic expansion.
    /// @tparam T element type
    /// @tparam N fixed capacity (compile‑time constant)
    /// @tparam Alignment memory alignment (default 0)
    /// @tparam Policy growth policy (must be FixedPolicy<N>)
    /// @tparam Allocator allocator type (must provide fixed‑size allocation)
    template<typename T, size_t N, size_t Alignment = 0,
        typename Policy = FixedPolicy<N>,
        typename Allocator = AlignedAllocator<T, Alignment, Policy, FixedAllocator<T, Alignment, N> > >
    class BasicFixedVector {
    public:
        // --- type definitions ---
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using iterator = T *;
        using const_iterator = const T *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type = uint32_t;
        using difference_type = ptrdiff_t;
        using allocator_type = Allocator;

        static constexpr size_type npos =  std::numeric_limits<uint32_t>::max();
        static constexpr size_type kMaxSize = N;

        static_assert(N < std::numeric_limits<uint32_t>::max(), "");
        static_assert(!std::is_const_v<value_type>, "BasicFixedVector<T>::value_type must be non-const.");
        static_assert(!std::is_volatile_v<value_type>, "BasicFixedVector<T>::value_type must be non-volatile.");

        // --- constructors / destructor ---
        BasicFixedVector();

        explicit BasicFixedVector(const allocator_type &alloc = allocator_type{});

        explicit BasicFixedVector(value_type v, const allocator_type &alloc = allocator_type{});

        BasicFixedVector(const BasicFixedVector &other);

        BasicFixedVector(BasicFixedVector &&other) noexcept;

        ~BasicFixedVector();

        // --- assignment ---
        BasicFixedVector &operator=(const BasicFixedVector &other);

        BasicFixedVector &operator=(BasicFixedVector &&other) noexcept;

        // --- iterators ---
        iterator begin() noexcept;

        const_iterator begin() const noexcept;

        const_iterator cbegin() const noexcept;

        iterator end() noexcept;

        const_iterator end() const noexcept;

        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;

        const_reverse_iterator rbegin() const noexcept;

        const_reverse_iterator crbegin() const noexcept;

        reverse_iterator rend() noexcept;

        const_reverse_iterator rend() const noexcept;

        const_reverse_iterator crend() const noexcept;

        // --- capacity ---
        [[nodiscard]] bool empty() const noexcept;

        size_type size() const noexcept;

        size_type capacity() const noexcept;

        /// Allocates memory for N elements if not already allocated.
        BasicFixedVector &reserve(size_type n = N);

        // --- element access (no bounds checking) ---
        reference operator[](size_type n);

        const_reference operator[](size_type n) const;

        /// Returns a reference to the first element (undefined behavior if empty).
        reference front() noexcept;

        const_reference front() const noexcept;

        reference back() noexcept;

        const_reference back() const noexcept;

        pointer data() noexcept;

        const_pointer data() const noexcept;

        // --- modifiers ---
        /// Returns the number of elements actually added (0 or 1).
        size_type push_back(const value_type &value);

        /// Returns the number of elements actually added.
        size_type push_back(const value_type *value, size_type n);

        /// move, returns the number of elements actually added.
        size_type push_back(value_type *value, size_type n);

        /// Returns the number of elements actually added (0 or 1).
        size_type push_back(value_type &&value);

        template<typename... Args>
        size_type emplace_back(Args &&... args);

        /// Removes the last n elements. Returns the number of elements actually removed.
        size_type pop_back(size_type n);

        /// Resizes the container to n (capped at capacity). Returns new size.
        size_type resize(size_type n);

        size_type resize(size_type n, const value_type &val);

        /// Assigns n copies of val (capped at capacity). Returns number of elements assigned.
        size_type assign(size_type n, const value_type &val);

        size_type assign(const value_type *val, size_type n);

        /// Assigns from range [first, last) (capped at capacity). Returns number of elements assigned.
        template<typename InputIterator>
        InputIterator assign(InputIterator first, InputIterator last);

        /// Erases element at position. Returns 1 if erased, 0 if position invalid.
        size_type erase(const_iterator position);

        /// Erases range [first, last). Returns number of elements erased.
        size_type erase(const_iterator first, const_iterator last);

        size_type erase(size_type pos, size_type n);

        /// Places a single element at position. Existing elements from position to end are shifted right.
        /// If total size would exceed capacity, trailing elements beyond capacity are discarded.
        /// Returns number of elements actually placed (0 or 1).
        size_type shift_insert(const_iterator position, const value_type &value);

        size_type shift_insert(const_iterator position, value_type &&value);

        /// Places n copies of value starting at position. At most (capacity() - (position - begin())) elements
        /// are placed; excess copies ignored. Trailing elements beyond capacity are discarded.
        /// Returns number of elements actually placed.
        size_type shift_insert(const_iterator position, size_type n, const value_type &value);

        /// Places elements from the array pointed by data (with length n). Same semantics as above.
        /// Returns number of elements actually placed.
        size_type shift_insert(const_iterator position, const value_type *data, size_type n);

        size_type shift_insert(const_iterator position, value_type *data, size_type n);

        /// Places elements from the range [first, last). Returns an iterator pointing to the first
        /// element in the input range that was NOT placed (i.e., first + actual_placed_count).
        template<typename InputIterator>
        InputIterator shift_insert(const_iterator position, InputIterator first, InputIterator last);

        void clear() noexcept;

        void swap(BasicFixedVector &other) noexcept;

        // --- allocator observer ---
        const allocator_type &get_allocator() const noexcept;

        allocator_type &get_allocator() noexcept;

    protected:
        void do_allocate();

        void do_deallocate();

    protected:
        value_type *_data{nullptr};
        compressed_pair<size_type, allocator_type> _capacity{0,allocator_type{}};
        size_type _start{0};
        size_type _end{0};
    };

    /// Allocates memory for exactly N elements using the allocator.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline void BasicFixedVector<T, N, Alignment, Policy, Allocator>::do_allocate() {
        if (_data == nullptr) {
            _data = _allocator.allocate(N);
        }
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline void BasicFixedVector<T, N, Alignment, Policy, Allocator>::do_deallocate() {
        if (!_data) {
            return;
        }
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            for (size_type i = 0; i < _size; ++i) {
                destroy_at(_data + i);
            }
        }
        _size = 0;
        _allocator.deallocate(_data, N);
        _data = nullptr;
    }

    /// Default constructor. Does not allocate memory.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator>::BasicFixedVector()
        : _data(nullptr), _size(0), _allocator() {
    }

    /// Constructor with allocator. Does not allocate memory.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator>::BasicFixedVector(const allocator_type &alloc)
        : _data(nullptr), _size(0), _allocator(alloc) {
    }

    /// Constructor with a single element. Allocates memory and stores the element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator>::BasicFixedVector(
        value_type v, const allocator_type &alloc)
        : _allocator(alloc) {
        reserve(); // allocate fixed capacity
        _size = N;
        if constexpr (sizeof(T) == 1 && std::is_trivially_copyable_v<value_type>) {
            memset(_data, v, _size);
        } else {
            for (size_t i = 0; i < _size; ++i) {
                _data[i] = v;
            }
        }
    }

    /// Copy constructor. Deep copies the contents of other.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator>::BasicFixedVector(const BasicFixedVector &other)
        : _allocator(other._allocator) {
        if (other._data) {
            reserve(N);
            clear();
            _size = other._size;
            if constexpr (std::is_trivially_copyable_v<value_type>) {
                // Use memcpy for trivial types
                std::memcpy(_data, other._data, _size * sizeof(value_type));
            } else {
                std::uninitialized_copy(other._data, other._data + _size, _data);
            }
        } else {
            clear();
        }
    }

    /// Move constructor. Transfers ownership of resources from other.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator>::BasicFixedVector(BasicFixedVector &&other) noexcept
        : _allocator(std::move(other._allocator)) {
        clear();
        // Transfer ownership from other
        std::swap(_data, other._data);
        std::swap(_size, other._size);
    }

    /// Destructor. Destroys all elements and deallocates memory.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator>::~BasicFixedVector() {
        if (_data) {
            do_deallocate();
        }
    }

    /// Copy assignment. Replaces contents with a copy of other.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator> &
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::operator=(const BasicFixedVector &other) {
        if (this == &other) return *this;
        if (other._data) {
            do_allocate();
            clear();
            _size = other._size;
            if constexpr (std::is_trivially_copyable_v<value_type>) {
                std::memcpy(_data, other._data, _size * sizeof(value_type));
            } else {
                std::uninitialized_copy(other._data, other._data + _size, _data);
            }
        } else {
            clear();
        }
        return *this;
    }

    /// Move assignment. Transfers ownership from other.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator> &
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::operator=(BasicFixedVector &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        clear();

        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_allocator, other._allocator);
        return *this;
    }

    /// Returns an iterator to the first element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::begin() noexcept {
        return _data;
    }

    /// Returns a const iterator to the first element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::begin() const noexcept {
        return _data;
    }

    /// Returns a const iterator to the first element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::cbegin() const noexcept {
        return _data;
    }

    /// Returns an iterator to the element past the last element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::end() noexcept {
        return _data + _size;
    }

    /// Returns a const iterator to the element past the last element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::end() const noexcept {
        return _data + _size;
    }

    /// Returns a const iterator to the element past the last element.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::cend() const noexcept {
        return _data + _size;
    }

    /// Returns a reverse iterator to the first element of the reversed container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::reverse_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::rbegin() noexcept {
        return reverse_iterator(end());
    }

    /// Returns a const reverse iterator to the first element of the reversed container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reverse_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    /// Returns a const reverse iterator to the first element of the reversed container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reverse_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::crbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    /// Returns a reverse iterator to the element past the last element of the reversed container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::reverse_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::rend() noexcept {
        return reverse_iterator(begin());
    }

    /// Returns a const reverse iterator to the element past the last element of the reversed container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reverse_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    /// Returns a const reverse iterator to the element past the last element of the reversed container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reverse_iterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::crend() const noexcept {
        return const_reverse_iterator(begin());
    }

    /// Checks whether the container is empty.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline bool BasicFixedVector<T, N, Alignment, Policy, Allocator>::empty() const noexcept {
        return _size == 0;
    }

    /// Returns the number of elements in the container.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::size() const noexcept {
        return _size;
    }

    /// Returns the maximum number of elements the container can hold.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::capacity() const noexcept {
        return N;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline BasicFixedVector<T, N, Alignment, Policy, Allocator> &BasicFixedVector<T, N, Alignment, Policy,
        Allocator>::reserve(size_type /*n*/) {
        do_allocate();
        return *this;
    }

    /// Returns a reference to the element at specified index (no bounds checking).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::reference
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::operator[](size_type n) {
        return _data[n];
    }

    /// Returns a const reference to the element at specified index (no bounds checking).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reference
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::operator[](size_type n) const {
        return _data[n];
    }

    /// Returns a reference to the first element (undefined behavior if empty).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::reference
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::front() noexcept {
        return _data[0];
    }

    /// Returns a const reference to the first element (undefined behavior if empty).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reference
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::front() const noexcept {
        return _data[0];
    }

    /// Returns a reference to the last element (undefined behavior if empty).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::reference
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::back() noexcept {
        return _data[_size - 1];
    }

    /// Returns a const reference to the last element (undefined behavior if empty).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_reference
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::back() const noexcept {
        return _data[_size - 1];
    }

    /// Returns a pointer to the underlying array.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::pointer
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::data() noexcept {
        return _data;
    }

    /// Returns a const pointer to the underlying array.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::const_pointer
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::data() const noexcept {
        return _data;
    }

    /// Appends an element. Returns the number of elements actually added (0 or 1).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::push_back(const value_type &value) {
        if (_size >= N) return 0;
        if constexpr (std::is_trivially_constructible_v<T>) {
            *(_data + _size) = value;
        } else {
            construct_at(_data + _size, value);
        }
        ++_size;
        return 1;
    }

    /// Appends n elements from the array pointed to by value.
    /// Returns the number of elements actually added (0 .. n).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::push_back(const value_type *value, size_type n) {
        size_type available = N - _size;
        size_type count = n < available ? n : available;
        if (count == 0) return 0;
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memcpy(_data + _size, value, count * sizeof(value_type));
        } else {
            for (size_type i = 0; i < count; ++i) {
                construct_at(_data + _size + i, value[i]);
            }
        }
        _size += count;
        return count;
    }

    /// Appends n elements by moving from the array pointed to by value.
    /// Returns the number of elements actually added (0 .. n).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::push_back(value_type *value, size_type n) {
        size_type available = N - _size;
        size_type count = n < available ? n : available;
        if (count == 0) return 0;
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memcpy(_data + _size, value, count * sizeof(value_type));
        } else {
            for (size_type i = 0; i < count; ++i) {
                construct_at(_data + _size + i, std::move(value[i]));
            }
        }
        _size += count;
        return count;
    }

    /// Appends an element by move. Returns the number of elements actually added (0 or 1).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::push_back(value_type &&value) {
        if (_size >= N) return 0;
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            _data[_size] = std::move(value); // for trivial types, move is same as copy
        } else {
            construct_at(_data + _size, std::move(value));
        }
        ++_size;
        return 1;
    }

    /// Constructs an element in place at the end. Returns the number of elements actually added (0 or 1).
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    template<typename... Args>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::emplace_back(Args &&... args) {
        if (_size >= N) return 0;
        construct_at(_data + _size, std::forward<Args>(args)...);
        ++_size;
        return 1;
    }

    /// Removes up to n elements from the end. Returns the number of elements actually removed.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::pop_back(size_type n) {
        size_type removed = _size < n ? _size : n;
        if (removed == 0) return 0;
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            for (size_type i = 0; i < removed; ++i) {
                destroy_at(_data + _size - 1 - i);
            }
        }
        _size -= removed;
        return removed;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::resize(size_type n) {
        if (n > N) n = N;
        if (n > _size) {
            // Default-construct the extra elements
            if constexpr (std::is_trivially_default_constructible_v<value_type>) {
                // For trivial types, no need to construct (memory already allocated)
                // But we still need to "mark" them as constructed.
                // Usually zero-initialization is not required, so just update _size.
            } else {
                for (size_type i = _size; i < n; ++i) {
                    construct_at(_data + i);
                }
            }
            _size = n;
        } else if (n < _size) {
            // Destroy the excess elements
            if constexpr (!std::is_trivially_destructible_v<value_type>) {
                for (size_type i = n; i < _size; ++i) {
                    destroy_at(_data + i);
                }
            }
            _size = n;
        }
        return _size;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::resize(size_type n, const value_type &val) {
        if (n > N) n = N;
        if (n > _size) {
            if constexpr (std::is_trivially_copyable_v<value_type>) {
                for (size_type i = _size; i < n; ++i) {
                    _data[i] = val;
                }
            } else {
                for (size_type i = _size; i < n; ++i) {
                    construct_at(_data + i, val);
                }
            }
            _size = n;
        } else if (n < _size) {
            if constexpr (!std::is_trivially_destructible_v<value_type>) {
                for (size_type i = n; i < _size; ++i) {
                    destroy_at(_data + i);
                }
            }
            _size = n;
        }
        return _size;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::assign(size_type n, const value_type &val) {
        if (n > N) n = N;
        clear();
        if (n == 0) return 0;
        do_allocate();
        // Assume _data is already allocated (caller must have called reserve() or used a constructor that allocates)
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            for (size_type i = 0; i < n; ++i) {
                _data[i] = val;
            }
        } else {
            for (size_type i = 0; i < n; ++i) {
                construct_at(_data + i, val);
            }
        }
        _size = n;
        return n;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::assign(const value_type *val, size_type n) {
        if (n > N) n = N;
        clear();
        if (n == 0) return 0;
        do_allocate();
        // Assume _data is already allocated (user must have called reserve() or used a constructor that allocates)
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memcpy(_data, val, n * sizeof(value_type));
        } else {
            for (size_type i = 0; i < n; ++i) {
                construct_at(_data + i, val[i]);
            }
        }
        _size = n;
        return n;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    template<typename InputIterator>
    inline InputIterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::assign(InputIterator first, InputIterator last) {
        if (first == last) return first;
        do_allocate();
        clear();
        size_type n = 0;
        while (n < N && first != last) {
            if constexpr (std::is_trivially_copyable_v<value_type>) {
                _data[n] = *first;
            } else {
                construct_at(_data + n, *first);
            }
            ++first;
            ++n;
        }
        _size = n;
        return first;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::erase(const_iterator position) {
        if (position >= end()) return 0;
        size_type index = static_cast<size_type>(position - begin());
        size_type last_index = _size - 1;
        if (index != last_index) {
            if constexpr (std::is_trivially_copyable_v<value_type>) {
                std::memmove(_data + index, _data + index + 1, (last_index - index) * sizeof(value_type));
            } else {
                std::move(_data + index + 1, _data + last_index + 1, _data + index);
            }
        }
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            destroy_at(_data + last_index);
        }
        --_size;
        return 1;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::erase(const_iterator first, const_iterator last) {
        size_type n = static_cast<size_type>(last - first);
        if (n == 0) return 0;
        size_type start = static_cast<size_type>(first - begin());
        size_type end = static_cast<size_type>(last - begin());
        size_type tail = _size - end;
        if (tail > 0) {
            if constexpr (std::is_trivially_copyable_v<value_type>) {
                std::memmove(_data + start, _data + end, tail * sizeof(value_type));
            } else {
                std::move(_data + end, _data + _size, _data + start);
            }
        }
        _size -= n;
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            for (size_type i = 0; i < n; ++i) {
                destroy_at(_data + _size + i);
            }
        }
        return n;
    }


    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::erase(size_type pos, size_type n) {
        if (pos >= _size) return 0;
        size_type count = (n > _size - pos) ? (_size - pos) : n;
        erase(begin() + pos, begin() + pos + count);
        return count;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy,
        Allocator>::shift_insert(const_iterator position, const value_type &value) {
        if (_size >= N) return 0;
        size_type index = static_cast<size_type>(position - begin());
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memmove(_data + index + 1, _data + index, (_size - index) * sizeof(value_type));
            _data[index] = value;
        } else {
            // Shift elements to the right using move (safe for non-trivial types)
            std::move_backward(_data + index, _data + _size, _data + _size + 1);
            construct_at(_data + index, value);
        }
        ++_size;
        return 1;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::shift_insert(const_iterator position, value_type &&value) {
        if (_size >= N) return 0;
        size_type index = static_cast<size_type>(position - begin());
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memmove(_data + index + 1, _data + index, (_size - index) * sizeof(value_type));
            _data[index] = std::move(value);
        } else {
            std::move_backward(_data + index, _data + _size, _data + _size + 1);
            construct_at(_data + index, std::move(value));
        }
        ++_size;
        return 1;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::shift_insert(const_iterator position, size_type n,
                                                                       const value_type &value) {
        size_type index = static_cast<size_type>(position - begin());
        size_type available = N - _size;
        size_type count = n < available ? n : available;
        if (count == 0) return 0;
        size_type tail = _size - index;
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memmove(_data + index + count, _data + index, tail * sizeof(value_type));
            for (size_type i = 0; i < count; ++i) _data[index + i] = value;
        } else {
            std::move_backward(_data + index, _data + _size, _data + _size + count);
            for (size_type i = 0; i < count; ++i) construct_at(_data + index + i, value);
        }
        _size += count;
        return count;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::shift_insert(const_iterator position, const value_type *data,
                                                                       size_type n) {
        size_type index = static_cast<size_type>(position - begin());
        size_type available = N - _size;
        size_type count = n < available ? n : available;
        if (count == 0) return 0;
        size_type tail = _size - index;
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memmove(_data + index + count, _data + index, tail * sizeof(value_type));
            std::memcpy(_data + index, data, count * sizeof(value_type));
        } else {
            std::move_backward(_data + index, _data + _size, _data + _size + count);
            for (size_type i = 0; i < count; ++i) construct_at(_data + index + i, data[i]);
        }
        _size += count;
        return count;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::size_type
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::shift_insert(const_iterator position, value_type *data,
                                                                       size_type n) {
        size_type index = static_cast<size_type>(position - begin());
        size_type available = N - _size;
        size_type count = n < available ? n : available;
        if (count == 0) return 0;
        size_type tail = _size - index;
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            std::memmove(_data + index + count, _data + index, tail * sizeof(value_type));
            std::memcpy(_data + index, data, count * sizeof(value_type));
        } else {
            std::move_backward(_data + index, _data + _size, _data + _size + count);
            for (size_type i = 0; i < count; ++i) construct_at(_data + index + i, std::move(data[i]));
        }
        _size += count;
        return count;
    }

    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    template<typename InputIterator>
    inline InputIterator
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::shift_insert(const_iterator position, InputIterator first,
                                                                       InputIterator last) {
        size_type index = static_cast<size_type>(position - begin());
        size_type available = N - _size;
        size_type count = 0;
        // Determine how many we can insert
        InputIterator it = first;
        for (; count < available && it != last; ++it, ++count) {
        }
        if (count == 0) return first;
        size_type tail = _size - index;
        // For non-trivial types, we need to destroy the gap elements before moving.
        // We'll handle separately for trivial and non-trivial.
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            // Shift existing elements to the right
            std::memmove(_data + index + count, _data + index, tail * sizeof(value_type));
            // Copy new elements into the gap
            it = first;
            for (size_type i = 0; i < count; ++i, ++it) {
                _data[index + i] = *it;
            }
        } else {
            std::move_backward(_data + index, _data + _size, _data + _size + count);
            it = first;
            for (size_type i = 0; i < count; ++i, ++it) {
                construct_at(_data + index + i, *it);
            }
        }
        _size += count;
        // Return iterator to first uninserted element
        return std::next(first, count);
    }

    /// Destroys all elements but does not free memory.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline void BasicFixedVector<T, N, Alignment, Policy, Allocator>::clear() noexcept {
        if (!_data) {
            _size = 0;
            return;
        }
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            for (size_type i = 0; i < _size; ++i) {
                destroy_at(_data + i);
            }
        }
        _size = 0;
    }

    /// Swaps the contents and allocators with another instance.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline void BasicFixedVector<T, N, Alignment, Policy, Allocator>::swap(BasicFixedVector &other) noexcept {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_allocator, other._allocator);
    }

    /// Returns a const reference to the allocator.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline const typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::allocator_type &
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::get_allocator() const noexcept {
        return _allocator;
    }

    /// Returns a reference to the allocator.
    template<typename T, size_t N, size_t Alignment, typename Policy, typename Allocator>
    inline typename BasicFixedVector<T, N, Alignment, Policy, Allocator>::allocator_type &
    BasicFixedVector<T, N, Alignment, Policy, Allocator>::get_allocator() noexcept {
        return _allocator;
    }
} // namespace fermat
