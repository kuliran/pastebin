#include "utils/id_gen.hpp"

#include <benchmark/benchmark.h>
#include <userver/engine/run_standalone.hpp>

using namespace paste_service::id_gen;

void GenIdBenchmark(benchmark::State& state) {
    userver::engine::RunStandalone([&] {
        for (auto _ : state) {
            auto result = GenId();
            benchmark::DoNotOptimize(result);
        }
    });
}

BENCHMARK(GenIdBenchmark);