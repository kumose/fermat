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

#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <turbo/log/logging.h>

namespace fermat {
    /// ReferenceObject
    ///
    /// Lightweight intrusive reference handle (one machine word).
    ///
    ///   1. REF_UNIQUE (ref_count == 1, not touched in construction)
    ///      - Construction window, single-threaded.
    ///      - Copy / share() / copy assignment: DKCHECK.
    ///
    ///   2. REF_SHARED (same ref_count, atomic ops)
    ///      - Entered by publish(). ref_count unchanged.
    ///
    template<typename T>
    class ReferenceObject {
    public:
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;

        enum class RefFlag : uint32_t {
            REF_NONE = 0,
            REF_UNIQUE = 1,
            REF_SHARED = 2,
        };

        struct alignas(8) ReferenceCount {
            RefFlag ref_flag{RefFlag::REF_NONE};
            std::atomic<uint32_t> ref_count{1};
            T object;
        };

        struct adopt_unique_tag_t {
            explicit adopt_unique_tag_t() = default;
        };

        static inline constexpr adopt_unique_tag_t kAdoptUnique{};

    public:
        ReferenceObject() noexcept = default;

        ReferenceObject(const ReferenceObject &other);

        ReferenceObject(ReferenceObject &&other) noexcept;

        explicit ReferenceObject(adopt_unique_tag_t, ReferenceCount *block) noexcept;

        ~ReferenceObject();

        ReferenceObject &operator=(const ReferenceObject &other);

        ReferenceObject &operator=(ReferenceObject &&other) noexcept;

        template<typename... Args>
        static ReferenceObject make_unique(Args &&... args);

        static ReferenceObject make_unique(T &&object);

        static ReferenceObject make_unique(const T &object) = delete;

        static ReferenceObject make_unique(std::unique_ptr<T> &&object);

        static ReferenceObject adopt_unique(ReferenceObject &&other) noexcept;

        void publish() &;

        [[nodiscard]] ReferenceObject share() const;

        [[nodiscard]] ReferenceObject copy() const;

        void reset() noexcept;

        void swap(ReferenceObject &other) noexcept;

        [[nodiscard]] pointer get() noexcept;

        [[nodiscard]] const_pointer get() const noexcept;

        [[nodiscard]] reference operator*() noexcept;

        [[nodiscard]] const_reference operator*() const noexcept;

        [[nodiscard]] pointer operator->() noexcept;

        [[nodiscard]] const_pointer operator->() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool unique() const noexcept;

        [[nodiscard]] bool shared() const noexcept;

        [[nodiscard]] uint32_t use_count() const noexcept;

        [[nodiscard]] RefFlag ref_flag() const noexcept;

    protected:
        static void destroy_block(ReferenceCount *block) noexcept;

        void add_ref();

        void release_ref() noexcept;

    protected:
        ReferenceCount *_ptr{nullptr};
    };

    template<typename T>
    inline void ReferenceObject<T>::destroy_block(ReferenceCount *block) noexcept {
        if (!block) {
            return;
        }
        delete block;
    }

    template<typename T>
    inline ReferenceObject<T>::ReferenceObject(const ReferenceObject &other) {
        if (!other._ptr) {
            _ptr = nullptr;
            return;
        }
        DKCHECK(other._ptr->ref_flag == RefFlag::REF_SHARED);
        _ptr = other._ptr;
        add_ref();
    }

    template<typename T>
    inline ReferenceObject<T>::ReferenceObject(ReferenceObject &&other) noexcept
        : _ptr(other._ptr) {
        other._ptr = nullptr;
    }

    template<typename T>
    inline ReferenceObject<T>::ReferenceObject(adopt_unique_tag_t, ReferenceCount *block) noexcept
        : _ptr(block) {
        DKCHECK(!_ptr || (_ptr->ref_flag == RefFlag::REF_UNIQUE && _ptr->ref_count == 1));
    }

    template<typename T>
    inline ReferenceObject<T>::~ReferenceObject() {
        release_ref();
    }

    template<typename T>
    inline ReferenceObject<T> &ReferenceObject<T>::operator=(const ReferenceObject &other) {
        if (this == &other) {
            return *this;
        }
        ReferenceObject(other).swap(*this);
        return *this;
    }

    template<typename T>
    inline ReferenceObject<T> &ReferenceObject<T>::operator=(ReferenceObject &&other) noexcept {
        if (this != &other) {
            release_ref();
            _ptr = other._ptr;
            other._ptr = nullptr;
        }
        return *this;
    }

    template<typename T>
    template<typename... Args>
    inline ReferenceObject<T> ReferenceObject<T>::make_unique(Args &&... args) {
        auto *block = new ReferenceCount;
        block->ref_flag = RefFlag::REF_UNIQUE;
        ::new(static_cast<void *>(&block->object)) T(std::forward<Args>(args)...);
        return ReferenceObject(kAdoptUnique, block);
    }

    template<typename T>
    inline ReferenceObject<T> ReferenceObject<T>::make_unique(T &&object) {
        auto *block = new ReferenceCount;
        block->ref_flag = RefFlag::REF_UNIQUE;
        ::new(static_cast<void *>(&block->object)) T(std::move(object));
        return ReferenceObject(kAdoptUnique, block);
    }

    template<typename T>
    inline ReferenceObject<T> ReferenceObject<T>::make_unique(std::unique_ptr<T> &&object) {
        if (!object) {
            return ReferenceObject{};
        }
        return make_unique(std::move(*object));
    }

    template<typename T>
    inline ReferenceObject<T> ReferenceObject<T>::adopt_unique(ReferenceObject &&other) noexcept {
        DKCHECK(other._ptr && other._ptr->ref_flag == RefFlag::REF_UNIQUE && other._ptr->ref_count == 1);
        ReferenceObject result;
        result._ptr = other._ptr;
        other._ptr = nullptr;
        return result;
    }

    template<typename T>
    inline void ReferenceObject<T>::publish() & {
        DKCHECK(_ptr && _ptr->ref_flag == RefFlag::REF_UNIQUE && _ptr->ref_count == 1);
        _ptr->ref_flag = RefFlag::REF_SHARED;
    }

    template<typename T>
    inline ReferenceObject<T> ReferenceObject<T>::share() const {
        DKCHECK(_ptr && _ptr->ref_flag == RefFlag::REF_SHARED);
        ReferenceObject result;
        result._ptr = _ptr;
        result.add_ref();
        return result;
    }

    template<typename T>
    inline ReferenceObject<T> ReferenceObject<T>::copy() const {
        DKCHECK(_ptr != nullptr);
        return make_unique(T(_ptr->object));
    }

    template<typename T>
    inline void ReferenceObject<T>::reset() noexcept {
        release_ref();
        _ptr = nullptr;
    }

    template<typename T>
    inline void ReferenceObject<T>::swap(ReferenceObject &other) noexcept {
        std::swap(_ptr, other._ptr);
    }

    template<typename T>
    inline typename ReferenceObject<T>::pointer ReferenceObject<T>::get() noexcept {
        return _ptr ? &_ptr->object : nullptr;
    }

    template<typename T>
    inline typename ReferenceObject<T>::const_pointer ReferenceObject<T>::get() const noexcept {
        return _ptr ? &_ptr->object : nullptr;
    }

    template<typename T>
    inline typename ReferenceObject<T>::reference ReferenceObject<T>::operator*() noexcept {
        DKCHECK(_ptr);
        return _ptr->object;
    }

    template<typename T>
    inline typename ReferenceObject<T>::const_reference ReferenceObject<T>::operator*() const noexcept {
        DKCHECK(_ptr);
        return _ptr->object;
    }

    template<typename T>
    inline typename ReferenceObject<T>::pointer ReferenceObject<T>::operator->() noexcept {
        DKCHECK(_ptr);
        return &_ptr->object;
    }

    template<typename T>
    inline typename ReferenceObject<T>::const_pointer ReferenceObject<T>::operator->() const noexcept {
        DKCHECK(_ptr);
        return &_ptr->object;
    }

    template<typename T>
    inline ReferenceObject<T>::operator bool() const noexcept {
        return _ptr != nullptr;
    }

    template<typename T>
    inline bool ReferenceObject<T>::empty() const noexcept {
        return _ptr == nullptr;
    }

    template<typename T>
    inline bool ReferenceObject<T>::unique() const noexcept {
        if (!_ptr) {
            return false;
        }
        if (_ptr->ref_flag == RefFlag::REF_UNIQUE) {
            return true;
        }
        DKCHECK(_ptr->ref_flag == RefFlag::REF_SHARED);
        return _ptr->ref_count.load(std::memory_order_acquire) == 1;
    }

    template<typename T>
    inline bool ReferenceObject<T>::shared() const noexcept {
        return _ptr && _ptr->ref_flag == RefFlag::REF_SHARED &&
               _ptr->ref_count.load(std::memory_order_acquire) > 1;
    }

    template<typename T>
    inline uint32_t ReferenceObject<T>::use_count() const noexcept {
        if (!_ptr) {
            return 0;
        }
        if (_ptr->ref_flag == RefFlag::REF_UNIQUE) {
            return 1;
        }
        DKCHECK(_ptr->ref_flag == RefFlag::REF_SHARED);
        return _ptr->ref_count.load(std::memory_order_acquire);
    }

    template<typename T>
    inline typename ReferenceObject<T>::RefFlag ReferenceObject<T>::ref_flag() const noexcept {
        return _ptr ? _ptr->ref_flag : RefFlag::REF_NONE;
    }

    template<typename T>
    inline void ReferenceObject<T>::add_ref() {
        DKCHECK(_ptr && _ptr->ref_flag == RefFlag::REF_SHARED);
        _ptr->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    template<typename T>
    inline void ReferenceObject<T>::release_ref() noexcept {
        if (!_ptr) {
            return;
        }
        if (_ptr->ref_flag == RefFlag::REF_SHARED) {
            ReferenceCount *const block = _ptr;
            if (block->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                destroy_block(block);
            }
        } else if (_ptr->ref_flag == RefFlag::REF_UNIQUE) {
            ReferenceCount *const block = _ptr;
            destroy_block(block);
        }
    }

    template<typename T>
    inline void swap(ReferenceObject<T> &a, ReferenceObject<T> &b) noexcept {
        a.swap(b);
    }

    template<typename T>
    inline bool operator==(const ReferenceObject<T> &a, const ReferenceObject<T> &b) noexcept {
        return a.get() == b.get();
    }

    template<typename T>
    inline bool operator!=(const ReferenceObject<T> &a, const ReferenceObject<T> &b) noexcept {
        return !(a == b);
    }
} // namespace fermat
