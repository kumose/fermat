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
#include <fermat/container/traits.h>

namespace fermat {
    class Receiver {
    public:
        virtual ~Receiver() = default;

        virtual turbo::Status reserve(size_t n) = 0;

        virtual turbo::Status append(const char *data, size_t len) = 0;

        virtual bool is_dynamic() const noexcept = 0;

        virtual size_t size() const noexcept = 0;
    };

    template<typename Container, typename Enabler = void>
    class ContainerReceiver;

    template<typename Container>
    class ContainerReceiver<Container, std::enable_if_t<std::is_same_v<VectorContainerTag, typename
                                                            ContainerTraits<Container>::container_tag> ||
                                                        std::is_same_v<StringContainerTag, typename
                                                            ContainerTraits<Container>::container_tag>> > : public Receiver {
    public:
        ContainerReceiver(Container &c) : _c(c) {
        }

        virtual ~ContainerReceiver() = default;

        turbo::Status reserve(size_t n) override {
            TURBO_RETURN_NOT_OK(ContainerTraits<Container>::reserve(_c, n));
            return  turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) {
                return  turbo::invalid_argument_error("data pointer is nullptr");
            }
            if (len == 0 ) {
                return  turbo::OkStatus();
            }
            ContainerTraits<Container>::append(_c, data, len);
            return  turbo::OkStatus();
        }

        bool is_dynamic() const noexcept override {
            return  true;
        }

        size_t size() const noexcept override {
            return  _c.size();
        }

    private:
        Container &_c;
    };

    template<typename Container>
    class ContainerReceiver<Container, std::enable_if_t<std::is_same_v<FixedContainerTag, typename
                ContainerTraits<Container>::container_tag> > > : public Receiver {
    public:
        ContainerReceiver(Container &c) : _c(c) {
        }

        virtual ~ContainerReceiver() = default;

        turbo::Status reserve(size_t n) override {
            TURBO_RETURN_NOT_OK(ContainerTraits<Container>::reserve(_c, n));
            return  turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) {
                return  turbo::invalid_argument_error("data pointer is nullptr");
            }
            if (len == 0 ) {
                return  turbo::OkStatus();
            }
            ContainerTraits<Container>::append(_c, data, len);
            return  turbo::OkStatus();
        }

        bool is_dynamic() const noexcept override {
            return  false;
        }

        size_t size() const noexcept override {
            return  _c.size();
        }

    private:
        Container &_c;
    };
} // namespace fermat
