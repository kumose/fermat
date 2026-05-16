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

       [[nodiscard]] virtual bool is_dynamic() const noexcept = 0;

       [[nodiscard]] virtual size_t size() const noexcept = 0;
    };

    template<typename Container, typename Enabler = void>
    class ContainerAppender;

    template<typename Container>
    class ContainerAppender<Container, std::enable_if_t<is_contiguous_string_receiver<
                Container>::value> > : public Receiver {
    public:
        explicit ContainerAppender(Container &c) : _c(c) {
        }

        turbo::Status reserve(size_t n) override {
            _c.reserve(n);
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) return turbo::invalid_argument_error("nullptr");
            if (len == 0) return turbo::OkStatus();
            _c.append(data, len);
            return turbo::OkStatus();
        }

       [[nodiscard]] bool is_dynamic() const noexcept override { return true; }
      [[nodiscard]]  size_t size() const noexcept override { return _c.size(); }

    private:
        Container &_c;
    };

    // ContainerAppender for vector receivers
    template<typename Container>
    class ContainerAppender<Container, std::enable_if_t<is_contiguous_vector_receiver<
                Container>::value> > : public Receiver {
    public:
        explicit ContainerAppender(Container &c) : _c(c) {
        }

        turbo::Status reserve(size_t n) override {
            _c.reserve(n);
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) return turbo::invalid_argument_error("nullptr");
            if (len == 0) return turbo::OkStatus();
            _c.insert(_c.end(), data, data + len);
            return turbo::OkStatus();
        }

       [[nodiscard]] bool is_dynamic() const noexcept override { return true; }
       [[nodiscard]] size_t size() const noexcept override { return _c.size(); }

    private:
        Container &_c;
    };

    template<typename Container, typename Enabler = void>
    class ContainerReceiver;


    // ContainerReceiver (owns the container) for string receivers
    template<typename Container>
    class ContainerReceiver<Container, std::enable_if_t<is_contiguous_string_receiver<
                Container>::value> > : public Receiver {
    public:
        ContainerReceiver() = default;

        explicit ContainerReceiver(Container &&c) : _c(std::move(c)) {
        }

        turbo::Status reserve(size_t n) override {
            _c.reserve(n);
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) return turbo::invalid_argument_error("nullptr");
            if (len == 0) return turbo::OkStatus();
            _c.append(data, len);
            return turbo::OkStatus();
        }

       [[nodiscard]] bool is_dynamic() const noexcept override { return true; }
       [[nodiscard]] size_t size() const noexcept override { return _c.size(); }
        Container release() { return std::move(_c); }
        const Container &container() const noexcept { return _c; }

    private:
        Container _c;
    };

    // ContainerReceiver for vector receivers
    template<typename Container>
    class ContainerReceiver<Container, std::enable_if_t<is_contiguous_vector_receiver<
                Container>::value> > : public Receiver {
    public:
        ContainerReceiver() = default;

        explicit ContainerReceiver(Container &&c) : _c(std::move(c)) {
        }

        turbo::Status reserve(size_t n) override {
            _c.reserve(n);
            return turbo::OkStatus();
        }

        turbo::Status append(const char *data, size_t len) override {
            if (!data) return turbo::invalid_argument_error("nullptr");
            if (len == 0) return turbo::OkStatus();
            _c.insert(_c.end(), data, data + len);
            return turbo::OkStatus();
        }

       [[nodiscard]] bool is_dynamic() const noexcept override { return true; }
      [[nodiscard]]  size_t size() const noexcept override { return _c.size(); }
        Container release() { return std::move(_c); }
        const Container &container() const noexcept { return _c; }

    private:
        Container _c;
    };
} // namespace fermat
