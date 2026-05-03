#include <benchmark/benchmark.h>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/ADT/BitVector.h"
#include "Aurora/ADT/Allocator.h"

using namespace aurora;

// ---- SmallVector benchmarks ----

static void BM_SmallVector_PushBack(benchmark::State& state) {
    for (auto _ : state) {
        SmallVector<int, 16> v;
        for (int i = 0; i < state.range(0); ++i)
            v.push_back(i);
        benchmark::DoNotOptimize(v.data());
    }
}
BENCHMARK(BM_SmallVector_PushBack)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_SmallVector_Access(benchmark::State& state) {
    SmallVector<int, 1024> v;
    for (int i = 0; i < state.range(0); ++i)
        v.push_back(i);
    for (auto _ : state) {
        int sum = 0;
        for (size_t i = 0; i < v.size(); ++i)
            sum += v[i];
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_SmallVector_Access)->Arg(100)->Arg(1000);

// ---- BitVector benchmarks ----

static void BM_BitVector_Set(benchmark::State& state) {
    BitVector bv(10000);
    for (auto _ : state) {
        for (unsigned i = 0; i < 1000; ++i)
            bv.set(i, i % 2 == 0);
        benchmark::DoNotOptimize(bv);
    }
}
BENCHMARK(BM_BitVector_Set);

static void BM_BitVector_Test(benchmark::State& state) {
    BitVector bv(10000);
    for (unsigned i = 0; i < 10000; ++i) bv.set(i, i % 2 == 0);

    int count = 0;
    for (auto _ : state) {
        for (unsigned i = 0; i < 10000; ++i)
            if (bv.test(i)) ++count;
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(BM_BitVector_Test);

// ---- BumpPtrAllocator benchmarks ----

static void BM_Allocator_SmallAllocs(benchmark::State& state) {
    BumpPtrAllocator alloc;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i)
            alloc.allocate(32, 8);
        alloc.reset();
    }
}
BENCHMARK(BM_Allocator_SmallAllocs);

static void BM_Allocator_CreateObject(benchmark::State& state) {
    BumpPtrAllocator alloc;
    for (auto _ : state) {
        for (int i = 0; i < 500; ++i) {
            int* p = alloc.create<int>(i);
            benchmark::DoNotOptimize(p);
        }
        alloc.reset();
    }
}
BENCHMARK(BM_Allocator_CreateObject);
