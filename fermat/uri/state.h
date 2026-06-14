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

#include <fermat/uri/common_defs.h>
#include <fermat/container/string.h>

namespace fermat::uri {
    /// @see https://url.spec.whatwg.org/#url-parsing
    enum class state {
        /// @see https://url.spec.whatwg.org/#authority-state
        AUTHORITY,

        /// @see https://url.spec.whatwg.org/#scheme-start-state
        SCHEME_START,

        /// @see https://url.spec.whatwg.org/#scheme-state
        SCHEME,

        /// @see https://url.spec.whatwg.org/#host-state
        HOST,

        /// @see https://url.spec.whatwg.org/#no-scheme-state
        NO_SCHEME,

        /// @see https://url.spec.whatwg.org/#fragment-state
        FRAGMENT,

        /// @see https://url.spec.whatwg.org/#relative-state
        RELATIVE_SCHEME,

        /// @see https://url.spec.whatwg.org/#relative-slash-state
        RELATIVE_SLASH,

        /// @see https://url.spec.whatwg.org/#file-state
        FILE,

        /// @see https://url.spec.whatwg.org/#file-host-state
        FILE_HOST,

        /// @see https://url.spec.whatwg.org/#file-slash-state
        FILE_SLASH,

        /// @see https://url.spec.whatwg.org/#path-or-authority-state
        PATH_OR_AUTHORITY,

        /// @see https://url.spec.whatwg.org/#special-authority-ignore-slashes-state
        SPECIAL_AUTHORITY_IGNORE_SLASHES,

        /// @see https://url.spec.whatwg.org/#special-authority-slashes-state
        SPECIAL_AUTHORITY_SLASHES,

        /// @see https://url.spec.whatwg.org/#special-relative-or-authority-state
        SPECIAL_RELATIVE_OR_AUTHORITY,

        /// @see https://url.spec.whatwg.org/#query-state
        QUERY,

        /// @see https://url.spec.whatwg.org/#path-state
        PATH,

        /// @see https://url.spec.whatwg.org/#path-start-state
        PATH_START,

        /// @see https://url.spec.whatwg.org/#cannot-be-a-base-url-path-state
        OPAQUE_PATH,

        /// @see https://url.spec.whatwg.org/#port-state
        PORT,
    };

    /// Stringify a URL state machine state.
    [[nodiscard]] fermat::KString to_string(fermat::uri::state s);
} // namespace fermat::uri
