#include <benchmark/benchmark.h>
#include "Aurora/CodeGen/RegisterAllocator.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Air/Builder.h"

using namespace aurora;

static void BM_RegAlloc_LiveIntervalComputation(benchmark::State& state) {
    auto mod = std::make_unique<Module>("bench");
    SmallVector<Type*, 8> params;
    for (int i = 0; i < 4; ++i) params.push_back(Type::getInt64Ty());
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = mod->createFunction(fnTy, "bench_fn");
    auto* entry = fn->getEntryBlock();

    AIRBuilder b(entry);
    unsigned v0 = b.createAdd(Type::getInt64Ty(), 0, 1);
    unsigned v1 = b.createMul(Type::getInt64Ty(), v0, 2);
    for (int i = 0; i < 50; ++i) {
        v0 = b.createAdd(Type::getInt64Ty(), v0, v1);
        v1 = b.createMul(Type::getInt64Ty(), v1, v0);
    }
    b.createRet(v0);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);

    // Run AIR->MIr pass first
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    for (auto _ : state) {
        LinearScanRegAlloc ra(mf);
        ra.allocateRegisters();
    }
}
BENCHMARK(BM_RegAlloc_LiveIntervalComputation);
