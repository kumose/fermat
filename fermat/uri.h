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

#include <fermat/uri/ada_idna.h>
#include <fermat/uri/character_sets-inl.h>
#include <fermat/uri/checkers-inl.h>
#include <fermat/uri/common_defs.h>
#include <fermat/uri/log.h>
#include <fermat/uri/encoding_type.h>
#include <fermat/uri/helpers.h>
#include <fermat/uri/parser.h>
#include <fermat/uri/scheme-inl.h>
#include <fermat/uri/serializers.h>
#include <fermat/uri/state.h>
#include <fermat/uri/unicode.h>
#include <fermat/uri/url_base.h>
#include <fermat/uri/url_base-inl.h>
#include <fermat/uri/url-inl.h>
#include <fermat/uri/url_components.h>
#include <fermat/uri/url_aggregator.h>
#include <fermat/uri/url_aggregator-inl.h>
#include <fermat/uri/url_search_params.h>
#include <fermat/uri/url_search_params-inl.h>

// Public API
#include <fermat/uri/ada_version.h>
#include <fermat/uri/implementation.h>
