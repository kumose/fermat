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

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace fermat {

    // Platform abstraction for handles
#ifdef _WIN32
    using PlatformHandle = HANDLE;
    constexpr PlatformHandle kInvalidHandle = INVALID_HANDLE_VALUE;
    inline void close_handle(PlatformHandle h) {
        if (h != INVALID_HANDLE_VALUE && h != NULL) {
            ::CloseHandle(h);
        }
    }
#else
    using PlatformHandle = int;
    constexpr PlatformHandle kInvalidHandle = -1;
    inline void close_handle(PlatformHandle h) {
        if (h >= 0) {
            ::close(h);
        }
    }
#endif

    // RAII guard for any platform handle
    class HandleGuard {
    public:
        HandleGuard() : _handle(kInvalidHandle) {}
        explicit HandleGuard(PlatformHandle h) : _handle(h) {}

        ~HandleGuard() {
            reset();
        }

        // Move constructor
        HandleGuard(HandleGuard&& other) noexcept
            : _handle(other.release()) {}

        // Move assignment
        HandleGuard& operator=(HandleGuard&& other) noexcept {
            if (this != &other) {
                reset(other.release());
            }
            return *this;
        }

        // Disable copy
        HandleGuard(const HandleGuard&) = delete;
        HandleGuard& operator=(const HandleGuard&) = delete;

        // Replace the current handle with a new one (closes existing if any)
        void reset(PlatformHandle h = kInvalidHandle) {
            if (_handle != kInvalidHandle) {
                close_handle(_handle);
                _handle = kInvalidHandle;
            }
            _handle = h;
        }

        // Release ownership of the handle, return it
        PlatformHandle release() noexcept {
            PlatformHandle h = _handle;
            _handle = kInvalidHandle;
            return h;
        }

        // Get the underlying handle
       [[nodiscard]] PlatformHandle get() const noexcept { return _handle; }

        // Check if the handle is valid
       [[nodiscard]] bool is_valid() const noexcept { return _handle != kInvalidHandle; }

        // Implicit conversion to the underlying handle for convenience
        operator PlatformHandle() const { return _handle; }

    private:
        PlatformHandle _handle;
    };
} // namespace fermat
