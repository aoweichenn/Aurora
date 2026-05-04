# 用户使用手册

## 1. 快速开始

### 1.1 环境要求

- **操作系统**: Linux x86-64 (或 WSL2)
- **编译器**: GCC 15+ / Clang 18+ (C++17)
- **构建工具**: CMake 3.16+
- **可选依赖**: GoogleTest / Google Benchmark (自动下载)

### 1.2 构建

```bash
# 基本构建 (库 + 驱动)
cmake -B build -S .
cmake --build build -j8

# 构建并启用测试
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build

# 构建并启用基准测试
cmake -B build -S . -DAURORA_BUILD_TESTS=ON -DAURORA_BUILD_BENCHMARKS=ON
cmake --build build -j8

# 构建 Release 版本
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

### 1.3 运行示例

```bash
# 运行内置示例
./build/tools/aurorac/aurorac
```

---

## 2. 快速集成

### 2.1 CMake 集成

```cmake
# 在你的 CMakeLists.txt 中
add_subdirectory(path/to/Aurora)
target_link_libraries(your_compiler PRIVATE AuroraMC)
```

### 2.2 链接静态库

```cmake
target_link_libraries(your_compiler PRIVATE
    AuroraADT AuroraAir AuroraTarget AuroraX86
    AuroraCodeGen AuroraMC
)
```

### 2.3 头文件

```cpp
// 所有头文件在 include/Aurora/ 下
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86/X86AsmPrinter.h"
```

---

## 3. 使用场景

### 3.1 场景 A：构建简单算术函数

```cpp
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"

// 构建 add(i32, i32) -> i32
auto module = std::make_unique<Module>("math");
SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
auto* fn = module->createFunction(fnTy, "add");

AIRBuilder builder(fn->getEntryBlock());
unsigned sum = builder.createAdd(Type::getInt32Ty(), 0, 1);
builder.createRet(sum);
```

### 3.2 场景 B：构建带控制流的函数

```cpp
// 构建 abs(i32) -> i32
// if x < 0: return -x else return x
auto* fn = module->createFunction(..., "abs");
auto* entry = fn->getEntryBlock();
auto* neg_bb = fn->createBasicBlock("neg");
auto* exit_bb = fn->createBasicBlock("exit");

AIRBuilder b(entry);
unsigned cmp = b.createICmp(ICmpCond::SLT, 0, 0);
b.createCondBr(cmp, neg_bb, exit_bb);

b.setInsertPoint(neg_bb);
unsigned neg = b.createSub(Type::getInt32Ty(), 0, 0); // 0 - x
b.createBr(exit_bb);

b.setInsertPoint(exit_bb);
SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings = {
    {entry, 0}, {neg_bb, neg}
};
unsigned phi = b.createPhi(Type::getInt32Ty(), incomings);
b.createRet(phi);
```

### 3.3 场景 C：编译 AIR → 汇编

```cpp
// 1. 创建目标机器
auto tm = TargetMachine::createX86_64();

// 2. 运行代码生成
CodeGenContext cgc(*tm, *module);
cgc.run();

// 3. 输出汇编
AsmTextStreamer streamer(std::cout);
const auto& ri = static_cast<const X86RegisterInfo&>(tm->getRegisterInfo());
X86AsmPrinter printer(streamer, ri);

for (auto& fn : module->getFunctions()) {
    MachineFunction mf(*fn, *tm);
    // 重新运行 pass (生产代码中应从 CGC 获取已处理的 MF)
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    printer.emitFunction(mf);
}
```

### 3.4 场景 D：从你的现有 IR 桥接

```cpp
// 你的现有 SSA IR → Aurora AIR
class IRAuroraBridge {
public:
    std::unique_ptr<Module> translate(YourModule& src) {
        auto dst = std::make_unique<Module>(src.getName());

        for (auto& fn : src.functions()) {
            // 1. 映射函数类型
            SmallVector<Type*, 8> params = mapTypes(fn.paramTypes());
            auto* fnTy = new FunctionType(mapType(fn.returnType()), params);

            // 2. 创建 AIR 函数
            auto* airFn = dst->createFunction(fnTy, fn.getName());

            // 3. 映射基本块
            std::unordered_map<YourBlock*, BasicBlock*> blockMap;
            for (auto& bb : fn.blocks()) {
                auto* airBB = airFn->createBasicBlock(bb.name());
                blockMap[&bb] = airBB;
            }

            // 4. 映射指令
            for (auto& bb : fn.blocks()) {
                AIRBuilder builder(blockMap[&bb]);
                for (auto& inst : bb.instructions()) {
                    translateInstruction(inst, builder, blockMap);
                }
            }
        }
        return dst;
    }

private:
    Type* mapType(YourType* ty) {
        if (ty->isInt32()) return Type::getInt32Ty();
        if (ty->isInt64()) return Type::getInt64Ty();
        if (ty->isVoid())  return Type::getVoidTy();
        // ... 更多映射
        return Type::getInt32Ty();
    }

    unsigned translateInstruction(YourInst& inst, AIRBuilder& b,
                                  std::unordered_map<YourBlock*, BasicBlock*>& blockMap) {
        switch (inst.getOpcode()) {
        case YourOpcode::Add:
            return b.createAdd(mapType(inst.getType()),
                               inst.getOperand(0), inst.getOperand(1));
        case YourOpcode::ICmp:
            return b.createICmp(mapICmpCond(inst.getCond()),
                                inst.getOperand(0), inst.getOperand(1));
        case YourOpcode::Ret:
            b.createRet(inst.getOperand(0));
            return 0;
        case YourOpcode::Br:
            b.createBr(blockMap[inst.getTarget()]);
            return 0;
        // ... 更多指令
        }
    }
};
```

---

## 4. 自定义 Pass

### 4.1 创建自定义 Pass

```cpp
class MyCustomPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        for (auto& mbb : mf.getBlocks()) {
            MachineInstr* mi = mbb->getFirst();
            while (mi) {
                // 检查并优化每条指令
                if (mi->getOpcode() == X86::MOV64rr) {
                    auto& src = mi->getOperand(0);
                    auto& dst = mi->getOperand(1);
                    // mov %rax, %rax → 删除
                    if (src.isReg() && dst.isReg() && src.getReg() == dst.getReg()) {
                        // 删除这条指令
                        MachineInstr* next = mi->getNext();
                        delete mi;
                        mi = next;
                        continue;
                    }
                }
                mi = mi->getNext();
            }
        }
    }

    const char* getName() const override { return "MyCustomPass"; }
};

// 使用
PassManager pm;
CodeGenContext::addStandardPasses(pm, *tm);
pm.addPass(std::make_unique<MyCustomPass>()); // 插入自定义 pass
pm.run(mf);
```

---

## 5. 编译选项

| CMake 选项 | 默认值 | 说明 |
|-----------|--------|------|
| `AURORA_BUILD_SHARED_LIBS` | ON | 构建动态库 |
| `AURORA_BUILD_STATIC_LIBS` | ON | 构建静态库 |
| `AURORA_BUILD_TESTS` | ON | 构建测试 |
| `AURORA_BUILD_BENCHMARKS` | ON | 构建基准测试 |
| `CMAKE_BUILD_TYPE` | (空) | Debug/Release |

---

## 6. 常见问题

### Q: 如何在 Release 模式下构建？
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

### Q: 如何只构建特定库？
```bash
cmake --build build --target AuroraADT
cmake --build build --target AuroraAir
```

### Q: 如何只运行特定测试？
```bash
./build/tests/ADT_tests --gtest_filter=SmallVectorTest.*
./build/tests/MC_tests --gtest_filter=IntegrationTest.*
```

### Q: 如何禁用动态库构建？
```bash
cmake -B build -S . -DAURORA_BUILD_SHARED_LIBS=OFF
```
