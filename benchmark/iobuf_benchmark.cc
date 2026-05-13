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

#include <benchmark/benchmark.h>
#include <fermat/io/iobuf.h>
#include <fermat/io/cord_buffer.h>
#include <fermat/container/string.h>
#include <fermat/container/buffer.h>
#include <string>
#include <random>
#include <vector>

namespace {
    // Generate random data of given size (bytes)
    std::vector<char> GenerateRandomData(size_t size) {
        std::vector<char> data(size);
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<char>(dist(rng));
        }
        return data;
    }

    // Benchmark: IOBuf append (default alignment=64, blockSize=4096)
    static void BM_IOBuf_Append(benchmark::State &state) {
        const size_t total_bytes = state.range(0);
        const auto data = GenerateRandomData(total_bytes);
        for (auto _: state) {
            fermat::IOBuf<64, 131072> buf;
            buf.append(data.data(), data.size());
            benchmark::DoNotOptimize(buf);
        }
        state.SetBytesProcessed(state.iterations() * total_bytes);
    }

    // Benchmark: IOBuf append (default alignment=64, blockSize=4096)
    static void BM_CordBuffer_Append(benchmark::State &state) {
        const size_t total_bytes = state.range(0);
        const auto data = GenerateRandomData(total_bytes);
        for (auto _: state) {
            fermat::CordBuffer<64, 65536> buf;
            buf.append(data.data(), data.size());
            benchmark::DoNotOptimize(buf);
        }
        state.SetBytesProcessed(state.iterations() * total_bytes);
    }

    // Benchmark: std::string append
    static void BM_String_Append(benchmark::State &state) {
        const size_t total_bytes = state.range(0);
        const auto data = GenerateRandomData(total_bytes);
        for (auto _: state) {
            std::string str;
            str.append(data.data(), data.size());
            benchmark::DoNotOptimize(str);
        }
        state.SetBytesProcessed(state.iterations() * total_bytes);
    }

    // Benchmark: std::string append
    static void BM_FermatString_Append(benchmark::State &state) {
        const size_t total_bytes = state.range(0);
        const auto data = GenerateRandomData(total_bytes);
        for (auto _: state) {
            fermat::BasicString<char, 64> str;
            str.append(data.data(), data.size());
            benchmark::DoNotOptimize(str);
        }
        state.SetBytesProcessed(state.iterations() * total_bytes);
    }

    // Benchmark: std::string append
    static void BM_Buffer_Append(benchmark::State &state) {
        const size_t total_bytes = state.range(0);
        const auto data = GenerateRandomData(total_bytes);
        for (auto _: state) {
            fermat::Buffer<char, 64> str;
            str.append(data.data(), data.size());
            benchmark::DoNotOptimize(str);
        }
        state.SetBytesProcessed(state.iterations() * total_bytes);
    }

    // Benchmark: IOBuf append in chunks (simulating incremental writes)
    static void BM_IOBuf_AppendChunked(benchmark::State &state) {
        const size_t total_bytes = state.range(0);
        const size_t chunk_size = state.range(1);
        const auto data = GenerateRandomData(total_bytes);
        for (auto _: state) {
            fermat::IOBuf<> buf;
            size_t written = 0;
            while (written < total_bytes) {
                size_t n = std::min(chunk_size, total_bytes - written);
                buf.append(data.data() + written, n);
                written += n;
            }
            benchmark::DoNotOptimize(buf);
        }
        state.SetBytesProcessed(state.iterations() * total_bytes);
    }

    // Register benchmarks
#define BENCH_ARGS ->Args({1 << 10, 0})   // 1KB
#define BENCH_ARGS_10K ->Args({10 << 10, 0})
#define BENCH_ARGS_100K ->Args({100 << 10, 0})
#define BENCH_ARGS_1M ->Args({1 << 20, 0})
#define BENCH_ARGS_10M ->Args({10 << 20, 0})
#define BENCH_ARGS_10M ->Args({20 << 20, 0})
#define BENCH_ARGS_50M ->Args({50 << 20, 0})

    BENCHMARK(BM_IOBuf_Append) BENCH_ARGS;
    BENCHMARK(BM_IOBuf_Append) BENCH_ARGS_10K;
    BENCHMARK(BM_IOBuf_Append) BENCH_ARGS_100K;
    BENCHMARK(BM_IOBuf_Append) BENCH_ARGS_1M;
    BENCHMARK(BM_IOBuf_Append) BENCH_ARGS_10M;
    BENCHMARK(BM_IOBuf_Append) BENCH_ARGS_50M;

    BENCHMARK(BM_CordBuffer_Append) BENCH_ARGS;
    BENCHMARK(BM_CordBuffer_Append) BENCH_ARGS_10K;
    BENCHMARK(BM_CordBuffer_Append) BENCH_ARGS_100K;
    BENCHMARK(BM_CordBuffer_Append) BENCH_ARGS_1M;
    BENCHMARK(BM_CordBuffer_Append) BENCH_ARGS_10M;
    BENCHMARK(BM_CordBuffer_Append) BENCH_ARGS_50M;


    BENCHMARK(BM_String_Append) BENCH_ARGS;
    BENCHMARK(BM_String_Append) BENCH_ARGS_10K;
    BENCHMARK(BM_String_Append) BENCH_ARGS_100K;
    BENCHMARK(BM_String_Append) BENCH_ARGS_1M;
    BENCHMARK(BM_String_Append) BENCH_ARGS_10M;
    BENCHMARK(BM_String_Append) BENCH_ARGS_50M;


    BENCHMARK(BM_FermatString_Append) BENCH_ARGS;
    BENCHMARK(BM_FermatString_Append) BENCH_ARGS_10K;
    BENCHMARK(BM_FermatString_Append) BENCH_ARGS_100K;
    BENCHMARK(BM_FermatString_Append) BENCH_ARGS_1M;
    BENCHMARK(BM_FermatString_Append) BENCH_ARGS_10M;
    BENCHMARK(BM_FermatString_Append) BENCH_ARGS_50M;

    BENCHMARK(BM_Buffer_Append) BENCH_ARGS;
    BENCHMARK(BM_Buffer_Append) BENCH_ARGS_10K;
    BENCHMARK(BM_Buffer_Append) BENCH_ARGS_100K;
    BENCHMARK(BM_Buffer_Append) BENCH_ARGS_1M;
    BENCHMARK(BM_Buffer_Append) BENCH_ARGS_10M;
    BENCHMARK(BM_Buffer_Append) BENCH_ARGS_50M;

    // Chunked append with 4KB chunks (typical block size)
    BENCHMARK(BM_IOBuf_AppendChunked)->Args({1 << 20, 4096})->Name("BM_IOBuf_AppendChunked/1M/4K");
    BENCHMARK(BM_IOBuf_AppendChunked)->Args({10 << 20, 4096})->Name("BM_IOBuf_AppendChunked/10M/4K");
    BENCHMARK(BM_IOBuf_AppendChunked)->Args({20 << 20, 4096})->Name("BM_IOBuf_AppendChunked/20M/4K");
    BENCHMARK(BM_IOBuf_AppendChunked)->Args({50 << 20, 4096})->Name("BM_IOBuf_AppendChunked/50M/4K");
} // namespace

BENCHMARK_MAIN();
