/// @file radix_sort_benchmark.cc
/// @brief Benchmark comparing fermat::radix_sort with std::sort.

#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdint>
#include <fermat/container/sort.h>

namespace fermat {
    namespace {
        // ----------------------------------------------------------------------------
        // SimpleKey structure with mKey for radix_sort extractor.
        // ----------------------------------------------------------------------------
        struct SimpleKey {
            using radix_type = uint32_t;
            radix_type mKey;
            int dummy;

            bool operator==(const SimpleKey &other) const {
                return mKey == other.mKey && dummy == other.dummy;
            }
        };

        /// Extractor for SimpleKey (used by radix_sort)
        struct SimpleKeyExtractor {
            using radix_type = uint32_t;
            radix_type operator()(const SimpleKey &sk) const { return sk.mKey; }
        };

        // ----------------------------------------------------------------------------
        // Generate random data.
        // ----------------------------------------------------------------------------
        static std::vector<SimpleKey> GenerateRandomKeys(size_t n) {
            std::vector<SimpleKey> data(n);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dist(0, 1000000);
            for (size_t i = 0; i < n; ++i) {
                data[i] = {dist(gen), static_cast<int>(i)};
            }
            return data;
        }

        static std::vector<SimpleKey> GenerateSortedKeys(size_t n) {
            std::vector<SimpleKey> data(n);
            for (size_t i = 0; i < n; ++i) {
                data[i] = {static_cast<uint32_t>(i), static_cast<int>(i)};
            }
            return data;
        }

        static std::vector<SimpleKey> GenerateReverseSortedKeys(size_t n) {
            std::vector<SimpleKey> data(n);
            for (size_t i = 0; i < n; ++i) {
                data[i] = {static_cast<uint32_t>(n - i), static_cast<int>(i)};
            }
            return data;
        }

        // -------------------------------------------------------a---------------------
        // Benchmarks for radix_sort.
        // ----------------------------------------------------------------------------
        static void BM_RadixSort_Random(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateRandomKeys(n);
            std::vector<SimpleKey> buffer(n); // Scratch buffer for radix_sort.

            for (auto _: state) {
                auto vec = data; // Copy to avoid sorting the same array across iterations.
                fermat::radix_sort<decltype(vec.begin()), SimpleKeyExtractor>(
                    vec.begin(), vec.end(), buffer.begin());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_Random)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_RadixSort_Sorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateSortedKeys(n);
            std::vector<SimpleKey> buffer(n);

            for (auto _: state) {
                auto vec = data;
                fermat::radix_sort<decltype(vec.begin()), SimpleKeyExtractor>(
                    vec.begin(), vec.end(), buffer.begin());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_Sorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_RadixSort_ReverseSorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateReverseSortedKeys(n);
            std::vector<SimpleKey> buffer(n);

            for (auto _: state) {
                auto vec = data;
                fermat::radix_sort<decltype(vec.begin()), SimpleKeyExtractor>(
                    vec.begin(), vec.end(), buffer.begin());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_ReverseSorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        // ----------------------------------------------------------------------------
        // Benchmarks for std::sort.
        // ----------------------------------------------------------------------------
        static void BM_StdSort_Random(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateRandomKeys(n);

            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end(),
                          [](const SimpleKey &a, const SimpleKey &b) { return a.mKey < b.mKey; });
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_Random)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_StdSort_Sorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateSortedKeys(n);

            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end(),
                          [](const SimpleKey &a, const SimpleKey &b) { return a.mKey < b.mKey; });
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_Sorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_StdSort_ReverseSorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateReverseSortedKeys(n);

            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end(),
                          [](const SimpleKey &a, const SimpleKey &b) { return a.mKey < b.mKey; });
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_ReverseSorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        // ============================================================================
        // Plain uint32_t benchmarks
        // ============================================================================

        static std::vector<uint32_t> GenerateRandomU32(size_t n) {
            std::vector<uint32_t> data(n);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dist(0, 1000000);
            for (size_t i = 0; i < n; ++i) data[i] = dist(gen);
            return data;
        }

        static std::vector<uint32_t> GenerateSortedU32(size_t n) {
            std::vector<uint32_t> data(n);
            for (size_t i = 0; i < n; ++i) data[i] = static_cast<uint32_t>(i);
            return data;
        }

        static std::vector<uint32_t> GenerateReverseSortedU32(size_t n) {
            std::vector<uint32_t> data(n);
            for (size_t i = 0; i < n; ++i) data[i] = static_cast<uint32_t>(n - i);
            return data;
        }

        static void BM_RadixSort_U32_Random(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateRandomU32(n);
            for (auto _: state) {
                auto vec = data;
                fermat::radix_sort(vec.begin(), vec.end());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_U32_Random)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_RadixSort_U32_Sorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateSortedU32(n);
            for (auto _: state) {
                auto vec = data;
                fermat::radix_sort(vec.begin(), vec.end());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_U32_Sorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_RadixSort_U32_ReverseSorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateReverseSortedU32(n);
            for (auto _: state) {
                auto vec = data;
                fermat::radix_sort(vec.begin(), vec.end());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_U32_ReverseSorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_StdSort_U32_Random(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateRandomU32(n);
            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_U32_Random)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_StdSort_U32_Sorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateSortedU32(n);
            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_U32_Sorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_StdSort_U32_ReverseSorted(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateReverseSortedU32(n);
            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end());
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_U32_ReverseSorted)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        // ============================================================================
        // Person struct benchmarks
        // ============================================================================

        struct Person {
            uint32_t id;
            std::string name;
            bool operator==(const Person &other) const { return id == other.id && name == other.name; }
        };

        static std::vector<Person> GenerateRandomPersons(size_t n) {
            std::vector<Person> data(n);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dist(0, 1000000);
            for (size_t i = 0; i < n; ++i) {
                data[i] = {dist(gen), "person_" + std::to_string(i)};
            }
            return data;
        }

        static void BM_RadixSort_Person_Random(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateRandomPersons(n);
            for (auto _: state) {
                auto vec = data;
                fermat::radix_sort(vec.begin(), vec.end(), [](const Person &p) { return p.id; });
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_RadixSort_Person_Random)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);

        static void BM_StdSort_Person_Random(benchmark::State &state) {
            const size_t n = state.range(0);
            auto data = GenerateRandomPersons(n);
            for (auto _: state) {
                auto vec = data;
                std::sort(vec.begin(), vec.end(),
                          [](const Person &a, const Person &b) { return a.id < b.id; });
                benchmark::DoNotOptimize(vec);
            }
            state.SetItemsProcessed(state.iterations() * n);
        }

        BENCHMARK(BM_StdSort_Person_Random)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000);
    } // namespace
} // namespace fermat

BENCHMARK_MAIN();
