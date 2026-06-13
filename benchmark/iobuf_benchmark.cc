/// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#include <benchmark/benchmark.h>
#include <fermat/container/cord_buffer.h>
#include <fermat/container/string.h>
#include <fermat/container/buffer.h>
#include <butil/iobuf.h>
#include <string>
#include <random>
#include <vector>

namespace {
    constexpr size_t kMaxChunkSize = 4096;

    static const std::vector<char> &GetRandomData() {
        static std::vector<char> data(kMaxChunkSize);
        static std::mt19937 rng(12345);
        static std::uniform_int_distribution<int> dist(0, 255);
        static bool init = [] {
            for (size_t i = 0; i < kMaxChunkSize; ++i)
                data[i] = static_cast<char>(dist(rng));
            return true;
        }();
        (void) init;
        return data;
    }

    inline size_t RandomChunkSize() {
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(1, kMaxChunkSize);
        return dist(rng);
    }

    template<typename Container>
    void AppendRandomChunked(Container &c, size_t total) {
        const auto &data = GetRandomData();
        size_t remain = total;
        while (remain) {
            size_t n = std::min(RandomChunkSize(), remain);
            c.append(data.data(), n);
            remain -= n;
        }
    }

    static void BM_CordBuffer_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        for (auto _: st) {
            fermat::CordBufferBase<32, 16 * 1024> buf;
            AppendRandomChunked(buf, total);
            benchmark::DoNotOptimize(buf);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_QueueBuffer_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        for (auto _: st) {
            fermat::BufferQueueBase<32, 16 * 1024> buf;
            AppendRandomChunked(buf, total);
            benchmark::DoNotOptimize(buf);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_String_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        for (auto _: st) {
            std::string str;
            AppendRandomChunked(str, total);
            benchmark::DoNotOptimize(str);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_FermatString_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        for (auto _: st) {
            fermat::BasicString<char, 64> str;
            AppendRandomChunked(str, total);
            benchmark::DoNotOptimize(str);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_Buffer_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        for (auto _: st) {
            fermat::Buffer<char, 64> buf;
            AppendRandomChunked(buf, total);
            benchmark::DoNotOptimize(buf);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_BrpcIOBuf_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        for (auto _: st) {
            butil::IOBuf buf;
            AppendRandomChunked(buf, total);
            benchmark::DoNotOptimize(buf);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_AbseilCord_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        const auto &data = GetRandomData();
        for (auto _: st) {
            absl::Cord cord;
            size_t remain = total;
            while (remain) {
                size_t n = std::min(RandomChunkSize(), remain);
                cord.Append(std::string_view(data.data(), n));
                remain -= n;
            }
            benchmark::DoNotOptimize(cord);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

    static void BM_TurboCord_RandomChunked(benchmark::State &st) {
        const size_t total = st.range(0);
        const auto &data = GetRandomData();
        for (auto _: st) {
            turbo::Cord cord;
            size_t remain = total;
            while (remain) {
                size_t n = std::min(RandomChunkSize(), remain);
                cord.append(std::string_view(data.data(), n));
                remain -= n;
            }
            benchmark::DoNotOptimize(cord);
        }
        st.SetBytesProcessed(st.iterations() * total);
    }

#define BENCH_ARGS \
    ->Args({1 << 10}) \
    ->Args({10 << 10}) \
    ->Args({100 << 10}) \
    ->Args({1 << 20}) \
    ->Args({10 << 20}) \
    ->Args({20 << 20}) \
    ->Args({50 << 20}) \
    ->Args({100 << 20})


    BENCHMARK(BM_QueueBuffer_RandomChunked) BENCH_ARGS;

    BENCHMARK(BM_CordBuffer_RandomChunked) BENCH_ARGS;
    BENCHMARK(BM_String_RandomChunked) BENCH_ARGS;
    BENCHMARK(BM_Buffer_RandomChunked) BENCH_ARGS;
    BENCHMARK(BM_BrpcIOBuf_RandomChunked) BENCH_ARGS;
    BENCHMARK(BM_AbseilCord_RandomChunked) BENCH_ARGS;
    BENCHMARK(BM_TurboCord_RandomChunked) BENCH_ARGS;

} // namespace

BENCHMARK_MAIN();
