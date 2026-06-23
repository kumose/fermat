// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.

#include <benchmark/benchmark.h>

#include <fermat/container/reference_ptr.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string MakePayload(std::size_t size) {
    return std::string(size, 'x');
}

// Sole ownership: create + destroy each iteration.
static void BM_SharedPtr_CreateDestroy(benchmark::State &state) {
    const std::string payload = MakePayload(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        auto ptr = std::make_shared<std::string>(payload);
        benchmark::DoNotOptimize(ptr);
        benchmark::DoNotOptimize(*ptr);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_ReferenceObject_CreateDestroy(benchmark::State &state) {
    std::string payload = MakePayload(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        auto obj = fermat::ReferenceObject<std::string>::make_unique(payload);
        benchmark::DoNotOptimize(obj);
        benchmark::DoNotOptimize(*obj);
    }
    state.SetItemsProcessed(state.iterations());
}

// Ref-count bump: copy an already-shared handle each iteration.
static void BM_SharedPtr_CopyShare(benchmark::State &state) {
    const std::string payload = MakePayload(static_cast<std::size_t>(state.range(0)));
    auto origin = std::make_shared<std::string>(payload);
    for (auto _ : state) {
        auto copy = origin;
        benchmark::DoNotOptimize(copy);
        benchmark::DoNotOptimize(*copy);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_ReferenceObject_Share(benchmark::State &state) {
    std::string payload = MakePayload(static_cast<std::size_t>(state.range(0)));
    auto origin = fermat::ReferenceObject<std::string>::make_unique(payload);
    origin.publish();
    for (auto _ : state) {
        auto copy = origin.share();
        benchmark::DoNotOptimize(copy);
        benchmark::DoNotOptimize(*copy);
    }
    state.SetItemsProcessed(state.iterations());
}

// Copy assignment on an existing handle.
static void BM_SharedPtr_CopyAssign(benchmark::State &state) {
    std::string payload = MakePayload(static_cast<std::size_t>(state.range(0)));
    auto origin = std::make_shared<std::string>(payload);
    auto slot = std::make_shared<std::string>("placeholder");
    for (auto _ : state) {
        slot = origin;
        benchmark::DoNotOptimize(slot);
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_ReferenceObject_ShareAssign(benchmark::State &state) {
    std::string payload = MakePayload(static_cast<std::size_t>(state.range(0)));
    auto origin = fermat::ReferenceObject<std::string>::make_unique(payload);
    origin.publish();
    auto slot = fermat::ReferenceObject<std::string>::make_unique("placeholder");
    slot.publish();
    for (auto _ : state) {
        slot = origin.share();
        benchmark::DoNotOptimize(slot);
    }
    state.SetItemsProcessed(state.iterations());
}

// Publish once, then share from multiple threads.
static void BM_SharedPtr_MultiThreadCopy(benchmark::State &state) {
    const int num_threads = static_cast<int>(state.range(0));
    const std::string payload = MakePayload(64);
    auto origin = std::make_shared<std::string>(payload);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(num_threads));
        state.ResumeTiming();

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&origin] {
                auto copy = origin;
                benchmark::DoNotOptimize(copy);
                benchmark::DoNotOptimize(*copy);
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }

        state.PauseTiming();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * num_threads);
}

static void BM_ReferenceObject_MultiThreadShare(benchmark::State &state) {
    const int num_threads = static_cast<int>(state.range(0));
    std::string payload = MakePayload(64);
    auto origin = fermat::ReferenceObject<std::string>::make_unique(payload);
    origin.publish();

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(num_threads));
        state.ResumeTiming();

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&origin] {
                auto copy = origin.share();
                benchmark::DoNotOptimize(copy);
                benchmark::DoNotOptimize(*copy);
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }

        state.PauseTiming();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * num_threads);
}

BENCHMARK(BM_SharedPtr_CreateDestroy)->Arg(0)->Arg(23)->Arg(64)->Arg(1024);
BENCHMARK(BM_ReferenceObject_CreateDestroy)->Arg(0)->Arg(23)->Arg(64)->Arg(1024);

BENCHMARK(BM_SharedPtr_CopyShare)->Arg(0)->Arg(23)->Arg(64)->Arg(1024);
BENCHMARK(BM_ReferenceObject_Share)->Arg(0)->Arg(23)->Arg(64)->Arg(1024);

BENCHMARK(BM_SharedPtr_CopyAssign)->Arg(64);
BENCHMARK(BM_ReferenceObject_ShareAssign)->Arg(64);

BENCHMARK(BM_SharedPtr_MultiThreadCopy)->Arg(1)->Arg(2)->Arg(4)->Arg(8);
BENCHMARK(BM_ReferenceObject_MultiThreadShare)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

} // namespace
