# 完整示例

## 示例 1: 编译简单加法函数

### 目标
将 `int add(int a, int b) { return a + b; }` 编译为 x86-64 汇编。

### 完整代码

```cpp
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86AsmPrinter.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include <iostream>
#include <memory>

using namespace aurora;

int main() {
    // ===== Phase 1: Build AIR IR =====
    auto module = std::make_unique<Module>("example");
    
    // int add(int a, int b)
    SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    auto* fn = module->createFunction(fnTy, "add");
    
    AIRBuilder builder(fn->getEntryBlock());
    
    // %2 = add i32 %0, %1
    unsigned sum = builder.createAdd(Type::getInt32Ty(), 0, 1);
    
    // ret i32 %2
    builder.createRet(sum);

    // ===== Phase 2: Print AIR IR =====
    std::cout << "=== AIR IR ===\n";
    for (auto& bb : fn->getBlocks()) {
        std::cout << bb->getName() << ":\n";
        AIRInstruction* inst = bb->getFirst();
        while (inst) {
            std::cout << "  " << inst->toString() << "\n";
            inst = inst->getNext();
        }
    }

    // ===== Phase 3: Code Generation =====
    auto tm = TargetMachine::createX86_64();
    CodeGenContext cgc(*tm, *module);
    cgc.run();

    // ===== Phase 4: Assembly Emission =====
    std::cout << "\n=== x86-64 Assembly ===\n";
    AsmTextStreamer streamer(std::cout);
    const auto& ri = static_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
    X86AsmPrinter printer(streamer, ri);

    for (auto& fn : module->getFunctions()) {
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        printer.emitFunction(mf);
    }

    return 0;
}
```

### 输出

```
=== AIR IR ===
entry:
  %2 = add i32 %0, %1
  ret %2

=== x86-64 Assembly ===
.globl add
add:
	.type add, @function
	.cfi_startproc
.Lentry:
	movl %edi, %eax
	addl %esi, %eax
	ret
	.cfi_endproc
	.size add, .-add
```

---

## 示例 2: 绝对值函数

### 目标
将 `int abs(int x) { return x < 0 ? -x : x; }` 编译为汇编。

### 代码

```cpp
auto module = std::make_unique<Module>("abs_example");

// int abs(int x)
SmallVector<Type*, 8> params = {Type::getInt32Ty()};
auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
auto* fn = module->createFunction(fnTy, "abs");

auto* entry = fn->getEntryBlock();
auto* negBB = fn->createBasicBlock("neg");
auto* exitBB = fn->createBasicBlock("exit");

// Entry block
AIRBuilder builder(entry);
unsigned cmp = builder.createICmp(ICmpCond::SLT, 0, 0);  // x < 0 ?
builder.createCondBr(cmp, negBB, exitBB);

// Neg block: return -x
builder.setInsertPoint(negBB);
unsigned neg = builder.createSub(Type::getInt32Ty(), 0, 0);  // 0 - x
builder.createBr(exitBB);

// Exit block: phi(x, -x)
builder.setInsertPoint(exitBB);
SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings = {
    {entry, 0},
    {negBB, neg}
};
unsigned phi = builder.createPhi(Type::getInt32Ty(), incomings);
builder.createRet(phi);
```

### 生成的 AIR IR

```
entry:
  %3 = icmp slt %0, %0
  condbr %3, &neg, &exit

neg:
  %4 = sub i32 %0, %0
  br &exit

exit:
  %5 = phi [&entry, %0], [&neg, %4]
  ret %5
```

### 期望的汇编

```asm
abs:
	cmpl $0, %edi
	jl .Lneg
	movl %edi, %eax
	ret
.Lneg:
	movl %edi, %eax
	negl %eax
	ret
```

---

## 示例 3: 循环累加

### 目标
`int sum_to(int n) { int s = 0; for (int i = 1; i <= n; i++) s += i; return s; }`

### 代码

```cpp
auto* fn = module->createFunction(fnTy, "sum_to");
auto* entry = fn->getEntryBlock();
auto* loop = fn->createBasicBlock("loop");
auto* exitBB = fn->createBasicBlock("exit");

// Entry block: s = 0, i = 1
AIRBuilder b(entry);
unsigned sAlloca = b.createAlloca(Type::getInt32Ty());
unsigned iAlloca = b.createAlloca(Type::getInt32Ty());
b.createStore(0, sAlloca);  // s = 0
b.createStore(1, iAlloca);  // i = 1
b.createBr(loop);

// Loop block
b.setInsertPoint(loop);
unsigned i = b.createLoad(Type::getInt32Ty(), iAlloca);
unsigned s = b.createLoad(Type::getInt32Ty(), sAlloca);
unsigned cmp = b.createICmp(ICmpCond::SLE, i, 0);  // i <= n (n = arg0)
b.createCondBr(cmp, loop, exitBB);  // continue or exit?

// Update s += i, i++
unsigned newS = b.createAdd(Type::getInt32Ty(), s, i);
b.createStore(newS, sAlloca);
unsigned newI = b.createAdd(Type::getInt32Ty(), i, 1);
b.createStore(newI, iAlloca);
b.createBr(loop);

// Exit block
b.setInsertPoint(exitBB);
unsigned finalS = b.createLoad(Type::getInt32Ty(), sAlloca);
b.createRet(finalS);
```

---

## 示例 4: 函数调用链

### 目标
```c
int square(int x) { return x * x; }
int sum_sq(int a, int b) { return square(a) + square(b); }
```

### 代码

```cpp
auto* squareFn = module->createFunction(squareTy, "square");
AIRBuilder b(squareFn->getEntryBlock());
unsigned sq = b.createMul(Type::getInt32Ty(), 0, 0);  // x * x
b.createRet(sq);

auto* sumSqFn = module->createFunction(sumSqTy, "sum_sq");
b.setInsertPoint(sumSqFn->getEntryBlock());
SmallVector<unsigned, 8> args0 = {0};
SmallVector<unsigned, 8> args1 = {1};
unsigned sqA = b.createCall(squareFn, args0);  // square(a)
unsigned sqB = b.createCall(squareFn, args1);  // square(b)
unsigned sum = b.createAdd(Type::getInt32Ty(), sqA, sqB);
b.createRet(sum);
```

### 生成的 AIR IR

```
define i32 @square(i32 %x) {
entry:
  %2 = mul i32 %0, %0
  ret i32 %2
}

define i32 @sum_sq(i32 %a, i32 %b) {
entry:
  %2 = call i32 @square(i32 %0)
  %3 = call i32 @square(i32 %1)
  %4 = add i32 %2, %3
  ret i32 %4
}
```

---

## 示例 5: 内存操作

### 目标
```c
void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}
```

### 代码

```cpp
SmallVector<Type*, 8> params = {Type::getPointerTy(Type::getInt32Ty()),
                                 Type::getPointerTy(Type::getInt32Ty())};
auto* fnTy = new FunctionType(Type::getVoidTy(), params);
auto* fn = module->createFunction(fnTy, "swap");

AIRBuilder b(fn->getEntryBlock());

// t = *a
unsigned t = b.createLoad(Type::getInt32Ty(), 0);

// *a = *b
unsigned bVal = b.createLoad(Type::getInt32Ty(), 1);
b.createStore(bVal, 0);

// *b = t
b.createStore(t, 1);

b.createRetVoid();
```

### 生成的 AIR IR

```
define void @swap(i32* %a, i32* %b) {
entry:
  %3 = load i32, i32* %0     ; t = *a
  %4 = load i32, i32* %1     ; b_val = *b
  store i32 %4, i32* %0      ; *a = *b
  store i32 %3, i32* %1      ; *b = t
  ret void
}
```

---

## 示例 6: 使用 GEP 访问数组

### 目标
```c
int arr[10];
int get(int i) { return arr[i]; }
```

### 代码

```cpp
auto* arrTy = Type::getArrayTy(Type::getInt32Ty(), 10);
auto* gv = module->createGlobal(arrTy, "arr");

SmallVector<Type*, 8> params = {Type::getInt32Ty()};
auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
auto* fn = module->createFunction(fnTy, "get");

AIRBuilder b(fn->getEntryBlock());

// GEP: arr + i
SmallVector<unsigned, 4> indices = {0, 0}; // [0] for array base, %0 for index
unsigned gep = b.createGEP(Type::getPointerTy(Type::getInt32Ty()), 0, indices);

// Load arr[i]
unsigned val = b.createLoad(Type::getInt32Ty(), gep);

b.createRet(val);
```

---

## 示例 7: 基准测试集成

### 目标
测量不同规模的 AIR→汇编编译链性能。

```cpp
#include <benchmark/benchmark.h>
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/Target/TargetMachine.h"

static void BM_CompileChain(benchmark::State& state) {
    // Build AIR IR
    auto module = std::make_unique<Module>("bench");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = module->createFunction(fnTy, "bench_fn");

    AIRBuilder b(fn->getEntryBlock());
    unsigned v0 = b.createAdd(Type::getInt64Ty(), 0, 1);
    unsigned v1 = b.createMul(Type::getInt64Ty(), v0, 2);
    for (int i = 0; i < state.range(0); ++i) {
        v0 = b.createAdd(Type::getInt64Ty(), v0, v1);
        v1 = b.createMul(Type::getInt64Ty(), v1, v0);
    }
    b.createRet(v0);

    auto tm = TargetMachine::createX86_64();

    for (auto _ : state) {
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        benchmark::DoNotOptimize(mf);
    }
}
BENCHMARK(BM_CompileChain)->Arg(10)->Arg(100)->Arg(1000);
```

---

## 示例 8: 完整的工具链脚本

```bash
#!/bin/bash
# build_and_test.sh — 完整的构建+测试+基准流程

set -euo pipefail

echo "=== Configuring ==="
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DAURORA_BUILD_TESTS=ON \
    -DAURORA_BUILD_BENCHMARKS=ON

echo "=== Building ==="
cmake --build build -j$(nproc)

echo "=== Running Unit Tests ==="
ctest --test-dir build --output-on-failure

echo "=== Running Benchmarks ==="
./build/tests/AuroraBench --benchmark_min_time=0.1s

echo "=== Building Complete ==="
echo "Libraries available in build/lib/"
echo "Driver available at build/tools/aurorac/aurorac"
```
