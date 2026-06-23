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
#include <sstream>
#include <string>
#include <random>
#include <cstring>
#include <fermat/container/cord_buffer.h>
#include <fermat/container/cord_output_stream.h>
#include <fermat/container/cord_input_stream.h>
#include <benchmark/cord_formatter.h>

using namespace fermat;

// Generate a random string of given size using a deterministic RNG.
static std::string MakeRandomString(size_t size, uint64_t seed = 12345) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<char> dist(33, 126); // printable ASCII
    std::string result;
    result.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        result.push_back(dist(rng));
    }
    return result;
}

// Common test data: a vector of (size, random_string)
static std::vector<std::pair<size_t, std::string>> PrepareTestData(
    size_t min_size = 1024, size_t max_size = 10 * 1024 * 1024, int num_samples = 100) {
    std::vector<std::pair<size_t, std::string>> data;
    std::mt19937_64 rng(98765);
    std::uniform_int_distribution<size_t> size_dist(min_size, max_size);
    for (int i = 0; i < num_samples; ++i) {
        size_t sz = size_dist(rng);
        data.emplace_back(sz, MakeRandomString(sz, sz)); // seed = size for reproducibility
    }
    return data;
}

// Benchmark: standard stringstream round-trip (write then read)
static void BM_StdStringStream_RoundTrip(benchmark::State& state) {
    auto test_data = PrepareTestData(1024, 10*1024*1024, 100);
    size_t idx = 0;
    size_t total_bytes = 0;
    for (auto _ : state) {
        const auto& [size, content] = test_data[idx % test_data.size()];
        ++idx;
        total_bytes += size;

        // Write
        std::ostringstream oss;
        oss << content;
        // Read back
        std::istringstream iss(oss.str());
        std::string read_back;
        iss >> read_back; // assuming no whitespace
        benchmark::DoNotOptimize(read_back);
    }
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_StdStringStream_RoundTrip);

// Benchmark: CordOutputStringStream + CordInputStringStream round-trip
static void BM_CordStringStream_RoundTrip(benchmark::State& state) {
    auto test_data = PrepareTestData(1024, 10*1024*1024, 100);
    size_t idx = 0;
    size_t total_bytes = 0;
    for (auto _ : state) {
        const auto& [size, content] = test_data[idx % test_data.size()];
        ++idx;
        total_bytes += size;

        CordBufferBase<64, 4096> cord;
        // Write
        {
            CordOutputStringStream<64, 4096> oss(&cord);
            oss << content;
            // The stream is flushed on destruction
        }
        // Read back
        CordInputStringStream<64, 4096> iss(&cord);
        std::string read_back;
        iss >> read_back;
        benchmark::DoNotOptimize(read_back);
    }
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_CordStringStream_RoundTrip);

// Optional: ping-pong benchmark (write once, read many times) to stress buffer reuse
static void BM_StdStringStream_PingPong(benchmark::State& state) {
    const size_t data_size = state.range(0);
    const std::string data = MakeRandomString(data_size);
    for (auto _ : state) {
        std::ostringstream oss;
        oss << data;
        std::istringstream iss(oss.str());
        std::string out;
        iss >> out;
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_StdStringStream_PingPong)->Arg(1024)->Arg(4096)->Arg(16384)->Arg(65536)->Arg(1048576)->Arg(4194304);

static void BM_CordStringStream_PingPong(benchmark::State& state) {
    const size_t data_size = state.range(0);
    const std::string data = MakeRandomString(data_size);
    for (auto _ : state) {
        CordBufferBase<64, 4096> cord;
        {
            CordOutputStringStream<64, 4096> oss(&cord);
            oss << data;
        }
        CordInputStringStream<64, 4096> iss(&cord);
        std::string out;
        iss >> out;
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_CordStringStream_PingPong)->Arg(1024)->Arg(4096)->Arg(16384)->Arg(65536)->Arg(1048576)->Arg(4194304);



// Pre‑generate large test data (size in bytes)
static std::string GenerateTestData(size_t size) {
    return MakeRandomString(size, size);
}

// -----------------------------------------------------------------------------
// Benchmark: std::istringstream token read (>> std::string)
// -----------------------------------------------------------------------------
static void BM_StdIStream_TokenRead(benchmark::State& state) {
    const size_t data_size = state.range(0);
    const std::string data = GenerateTestData(data_size);
    // Pre-build the stringstream (not measured)
    std::istringstream iss(data);
    for (auto _ : state) {
        // Reset the stream to beginning for each iteration
        iss.clear();
        iss.seekg(0);
        // Read token by token until EOF
        std::string token;
        while (iss >> token) {
            benchmark::DoNotOptimize(token);
        }
    }
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_StdIStream_TokenRead)->Arg(10 << 20)->Arg(20 << 20)->Arg(50 << 20);

// -----------------------------------------------------------------------------
// Benchmark: CordInputStringStream token read (>> std::string)
// -----------------------------------------------------------------------------
static void BM_CordIStream_TokenRead(benchmark::State& state) {
    const size_t data_size = state.range(0);
    const std::string data = GenerateTestData(data_size);
    // Pre‑build cord data (not measured)
    CordBufferBase<64, 4096> cord;
    {
        CordOutputStringStream<64, 4096> oss(&cord);
        oss << data;
    }
    for (auto _ : state) {
        CordInputStringStream<64, 4096> iss(&cord);
        std::string token;
        while (iss >> token) {
            benchmark::DoNotOptimize(token);
        }
    }
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_CordIStream_TokenRead)->Arg(10 << 20)->Arg(20 << 20)->Arg(50 << 20);

// -----------------------------------------------------------------------------
// Benchmark: std::istringstream bulk read (read into buffer)
// -----------------------------------------------------------------------------
static void BM_StdIStream_BulkRead(benchmark::State& state) {
    const size_t data_size = state.range(0);
    const std::string data = GenerateTestData(data_size);
    std::istringstream iss(data);
    std::vector<char> buffer(data_size);
    for (auto _ : state) {
        iss.clear();
        iss.seekg(0);
        iss.read(buffer.data(), data_size);
        benchmark::DoNotOptimize(buffer);
    }
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_StdIStream_BulkRead)->Arg(10 << 20)->Arg(20 << 20)->Arg(50 << 20);

// -----------------------------------------------------------------------------
// Benchmark: CordInputStringStream bulk read (read into buffer)
// -----------------------------------------------------------------------------
static void BM_CordIStream_BulkRead(benchmark::State& state) {
    const size_t data_size = state.range(0);
    const std::string data = GenerateTestData(data_size);
    CordBufferBase<64, 4096> cord;
    {
        CordOutputStringStream<64, 4096> oss(&cord);
        oss << data;
    }
    std::vector<char> buffer(data_size);
    for (auto _ : state) {
        CordInputStringStream<64, 4096> iss(&cord);
        iss.read(buffer.data(), data_size);
        benchmark::DoNotOptimize(buffer);
    }
    state.SetBytesProcessed(state.iterations() * data_size);
}
BENCHMARK(BM_CordIStream_BulkRead)->Arg(10 << 20)->Arg(20 << 20)->Arg(50 << 20);

// ============================================================================
// Binary serialization: mixed data types (uint64, double, length+string)
// ============================================================================

// Generate a vector of records for binary tests
struct BinaryRecord {
    uint64_t id;
    double value;
    std::string text;
};

static std::vector<BinaryRecord> GenerateBinaryRecords(size_t num_records, size_t avg_text_len) {
    std::vector<BinaryRecord> records;
    records.reserve(num_records);
    std::mt19937_64 rng(54321);
    std::uniform_int_distribution<uint64_t> id_dist;
    std::uniform_real_distribution<double> val_dist(-1000.0, 1000.0);
    std::uniform_int_distribution<size_t> len_dist(avg_text_len / 2, avg_text_len * 3 / 2);
    for (size_t i = 0; i < num_records; ++i) {
        size_t text_len = len_dist(rng);
        records.push_back({
            id_dist(rng),
            val_dist(rng),
            MakeRandomString(text_len, rng())
        });
    }
    return records;
}

// Pre‑generated test data (1000 records, average string length 256)
static const std::vector<BinaryRecord> kBinaryTestData = GenerateBinaryRecords(1000, 256);

// -----------------------------------------------------------------------------
// Benchmark: std::stringstream binary round-trip (write then read)
// -----------------------------------------------------------------------------
static void BM_StdBinaryStream_RoundTrip(benchmark::State& state) {
    size_t total_bytes = 0;
    for (auto _ : state) {
        std::ostringstream oss;
        // Write all records
        for (const auto& rec : kBinaryTestData) {
            oss.write(reinterpret_cast<const char*>(&rec.id), sizeof(rec.id));
            oss.write(reinterpret_cast<const char*>(&rec.value), sizeof(rec.value));
            uint32_t len = static_cast<uint32_t>(rec.text.size());
            oss.write(reinterpret_cast<const char*>(&len), sizeof(len));
            oss.write(rec.text.data(), len);
        }
        total_bytes += oss.tellp();

        // Read back
        std::istringstream iss(oss.str());
        for (const auto& rec_orig : kBinaryTestData) {
            uint64_t id;
            double value;
            uint32_t len;
            iss.read(reinterpret_cast<char*>(&id), sizeof(id));
            iss.read(reinterpret_cast<char*>(&value), sizeof(value));
            iss.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string text(len, '\0');
            iss.read(text.data(), len);
            benchmark::DoNotOptimize(id);
            benchmark::DoNotOptimize(value);
            benchmark::DoNotOptimize(text);
        }
    }
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_StdBinaryStream_RoundTrip);

// -----------------------------------------------------------------------------
// Benchmark: CordInputBinaryStream + CordOutputBinaryStream round-trip
// -----------------------------------------------------------------------------
static void BM_CordBinaryStream_RoundTrip(benchmark::State& state) {
    size_t total_bytes = 0;
    for (auto _ : state) {
        CordBufferBase<64, 4096> cord;
        {
            CordOutputBinaryStream<64, 4096> obs(&cord);
            for (const auto& rec : kBinaryTestData) {
                obs << rec.id << rec.value << static_cast<uint32_t>(rec.text.size());
                obs.write(rec.text.data(), rec.text.size());
            }
        }
        total_bytes += cord.size();

        CordInputBinaryStream<64, 4096> ibs(&cord);
        for (const auto& rec_orig : kBinaryTestData) {
            uint64_t id;
            double value;
            uint32_t len;
            ibs.read(id);
            ibs.read(value);
            ibs.read(len);
            std::string text;
            text.resize(len);
            ibs.read(text.data(), len);
            benchmark::DoNotOptimize(id);
            benchmark::DoNotOptimize(value);
            benchmark::DoNotOptimize(text);
        }
    }
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_CordBinaryStream_RoundTrip);

// -----------------------------------------------------------------------------
// Separate write-only and read-only benchmarks
// -----------------------------------------------------------------------------

static void BM_StdBinaryStream_WriteOnly(benchmark::State& state) {
    size_t total_bytes = 0;
    for (auto _ : state) {
        std::ostringstream oss;
        for (const auto& rec : kBinaryTestData) {
            oss.write(reinterpret_cast<const char*>(&rec.id), sizeof(rec.id));
            oss.write(reinterpret_cast<const char*>(&rec.value), sizeof(rec.value));
            uint32_t len = static_cast<uint32_t>(rec.text.size());
            oss.write(reinterpret_cast<const char*>(&len), sizeof(len));
            oss.write(rec.text.data(), len);
        }
        total_bytes += oss.tellp();
        benchmark::DoNotOptimize(oss);
    }
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_StdBinaryStream_WriteOnly);

static void BM_CordBinaryStream_WriteOnly(benchmark::State& state) {
    size_t total_bytes = 0;
    for (auto _ : state) {
        CordBufferBase<64, 4096> cord;
        {
            CordOutputBinaryStream<64, 4096> obs(&cord);
            for (const auto& rec : kBinaryTestData) {
                obs << rec.id << rec.value << static_cast<uint32_t>(rec.text.size());
                obs.write(rec.text.data(), rec.text.size());
            }
        }
        total_bytes += cord.size();
        benchmark::DoNotOptimize(cord);
    }
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_CordBinaryStream_WriteOnly);

static void BM_StdBinaryStream_ReadOnly(benchmark::State& state) {
    // Prepare binary data once
    std::ostringstream oss;
    for (const auto& rec : kBinaryTestData) {
        oss.write(reinterpret_cast<const char*>(&rec.id), sizeof(rec.id));
        oss.write(reinterpret_cast<const char*>(&rec.value), sizeof(rec.value));
        uint32_t len = static_cast<uint32_t>(rec.text.size());
        oss.write(reinterpret_cast<const char*>(&len), sizeof(len));
        oss.write(rec.text.data(), len);
    }
    std::string data = oss.str();
    for (auto _ : state) {
        std::istringstream iss(data);
        for (const auto& rec_orig : kBinaryTestData) {
            uint64_t id;
            double value;
            uint32_t len;
            iss.read(reinterpret_cast<char*>(&id), sizeof(id));
            iss.read(reinterpret_cast<char*>(&value), sizeof(value));
            iss.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string text(len, '\0');
            iss.read(text.data(), len);
            benchmark::DoNotOptimize(id);
            benchmark::DoNotOptimize(value);
            benchmark::DoNotOptimize(text);
        }
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_StdBinaryStream_ReadOnly);

static void BM_CordBinaryStream_ReadOnly(benchmark::State& state) {
    // Prepare cord data once
    CordBufferBase<64, 4096> cord;
    {
        CordOutputBinaryStream<64, 4096> obs(&cord);
        for (const auto& rec : kBinaryTestData) {
            obs << rec.id << rec.value << static_cast<uint32_t>(rec.text.size());
            obs.write(rec.text.data(), rec.text.size());
        }
    }
    for (auto _ : state) {
        CordInputBinaryStream<64, 4096> ibs(&cord);
        for (const auto& rec_orig : kBinaryTestData) {
            uint64_t id;
            double value;
            uint32_t len;
            ibs.read(id);
            ibs.read(value);
            ibs.read(len);
            std::string text;
            text.resize(len);
            ibs.read(text.data(), len);
            benchmark::DoNotOptimize(id);
            benchmark::DoNotOptimize(value);
            benchmark::DoNotOptimize(text);
        }
    }
    state.SetBytesProcessed(state.iterations() * cord.size());
}
BENCHMARK(BM_CordBinaryStream_ReadOnly);


// -----------------------------------------------------------------------------
// Benchmark: std::ostringstream using operator<< (text formatting)
// -----------------------------------------------------------------------------
static void BM_StdOStreamFormat(benchmark::State& state) {
    const size_t num_iter = state.range(0);
    const std::string test_str = MakeRandomString(256);
    for (auto _ : state) {
        std::ostringstream oss;
        for (size_t i = 0; i < num_iter; ++i) {
            oss << "Hello " << i << " " << test_str << " " << 3.14159 << "\n";
        }
        benchmark::DoNotOptimize(oss.str());
    }
    state.SetBytesProcessed(state.iterations() * num_iter * (256 + 30)); // approximate
}
BENCHMARK(BM_StdOStreamFormat)->Arg(100)->Arg(1000)->Arg(10000);

// -----------------------------------------------------------------------------
// Benchmark: CordFormatter using format() (direct fmt formatting into CordBuffer)
// -----------------------------------------------------------------------------
static void BM_CordFormatterFormat(benchmark::State& state) {
    const size_t num_iter = state.range(0);
    const std::string test_str = MakeRandomString(256);
    for (auto _ : state) {
        CordBufferBase<64, 4096> cord;
        CordFormatter<64, 4096> fmt(&cord);
        for (size_t i = 0; i < num_iter; ++i) {
            fmt.format("Hello {} {} {}\n", i, test_str, 3.14159);
        }
        benchmark::DoNotOptimize(cord.size());
    }
    state.SetBytesProcessed(state.iterations() * num_iter * (256 + 30));
}
BENCHMARK(BM_CordFormatterFormat)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();