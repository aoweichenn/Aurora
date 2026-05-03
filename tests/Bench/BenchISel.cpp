#include <benchmark/benchmark.h>
#include "Aurora/Target/X86/X86ISelPatterns.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

static void BM_ISel_MatchAdd64(benchmark::State& state) {
    const std::vector<unsigned> vregTypes;
    for (auto _ : state) {
        auto result = X86ISelPatterns::matchPattern(
            AIROpcode::Add, Type::getInt64Ty(), vregTypes, 0, 0);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ISel_MatchAdd64);

static void BM_ISel_MatchAllPatterns(benchmark::State& state) {
    const std::vector<unsigned> vregTypes;
    const auto& patterns = X86ISelPatterns::getAllPatterns();
    for (auto _ : state) {
        for (const auto& pat : patterns) {
            auto result = X86ISelPatterns::matchPattern(
                pat.airOp, Type::getInt64Ty(), vregTypes, 0, 0);
            benchmark::DoNotOptimize(result);
        }
    }
}
BENCHMARK(BM_ISel_MatchAllPatterns);
