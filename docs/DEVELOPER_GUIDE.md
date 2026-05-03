# 开发者指南

## 1. 项目结构

```
Aurora/
├── CMakeLists.txt                 # 顶层构建
├── LICENSE                        # MIT
├── README.md                      # 项目 README
├── docs/                          # 文档
│   ├── README.md                  # 文档索引 (本文)
│   ├── ARCHITECTURE.md            # 架构设计
│   ├── DIAGRAMS.md                # 类图与时序图
│   ├── REQUIREMENTS.md            # 需求分析
│   ├── MODULES.md                 # 模块设计
│   ├── API_REFERENCE.md           # API 参考
│   ├── USER_GUIDE.md              # 用户手册
│   ├── DEVELOPER_GUIDE.md         # 开发者指南 (本文)
│   └── EXAMPLES.md                # 完整示例
├── include/Aurora/                # 公共头文件
│   ├── ADT/                       # 数据结构
│   │   ├── SmallVector.h
│   │   ├── BitVector.h
│   │   ├── Graph.h
│   │   ├── Allocator.h
│   │   └── SparseSet.h
│   ├── Air/                       # 后端 IR
│   │   ├── Type.h
│   │   ├── Constant.h
│   │   ├── Instruction.h
│   │   ├── BasicBlock.h
│   │   ├── Function.h
│   │   ├── Module.h
│   │   └── Builder.h
│   ├── Target/                    # 目标抽象
│   │   ├── TargetMachine.h
│   │   ├── TargetRegisterInfo.h
│   │   ├── TargetInstrInfo.h
│   │   ├── TargetLowering.h
│   │   ├── TargetCallingConv.h
│   │   ├── TargetFrameLowering.h
│   │   └── X86/                   # x86-64 特化
│   │       ├── X86RegisterInfo.h
│   │       ├── X86InstrInfo.h
│   │       ├── X86InstrEncode.h
│   │       ├── X86TargetLowering.h
│   │       ├── X86CallingConv.h
│   │       ├── X86FrameLowering.h
│   │       ├── X86TargetMachine.h
│   │       └── X86ISelPatterns.h
│   ├── CodeGen/                   # 代码生成
│   │   ├── MachineInstr.h
│   │   ├── MachineBasicBlock.h
│   │   ├── MachineFunction.h
│   │   ├── SelectionDAG.h
│   │   ├── InstructionSelector.h
│   │   ├── LiveInterval.h
│   │   ├── RegisterAllocator.h
│   │   ├── PrologueEpilogueInserter.h
│   │   └── PassManager.h
│   └── MC/                       # 机器码发射
│       ├── MCStreamer.h
│       ├── AsmPrinter.h
│       └── X86AsmPrinter.h
├── lib/                           # 实现文件
│   ├── ADT/                       #  5 .cpp
│   ├── Air/                       #  7 .cpp
│   ├── Target/                    #  6 .cpp
│   │   └── X86/                   #  8 .cpp
│   ├── CodeGen/                   #  9 .cpp
│   └── MC/                        #  4 .cpp
├── tools/aurorac/                 # 驱动
│   ├── CMakeLists.txt
│   └── main.cpp
└── tests/                         # 测试 & 基准
    ├── CMakeLists.txt
    ├── ADT/                       # 4 test files
    ├── Air/                       # 5 test files
    ├── Target/                    # 4 test files
    ├── CodeGen/                   # 4 test files
    ├── MC/                        # 2 test files
    └── Bench/                     # 3 benchmark files
```

---

## 2. 编码规范

### 2.1 命名约定

| 元素 | 风格 | 示例 |
|------|------|------|
| 类名 | CamelCase | `X86TargetMachine`, `AIRInstruction` |
| 函数名 | camelCase | `createAdd()`, `getOpcode()` |
| 成员变量 | trailing_ | `opcode_`, `parent_` |
| 枚举 | CamelCase | `AIROpcode`, `ICmpCond` |
| 枚举值 | CamelCase 或 UPPER | `Add`, `EQ`, `RAX` |
| 命名空间 | lower_case | `aurora` |
| 文件名 | CamelCase.h/.cpp | `X86RegisterInfo.cpp` |
| 宏 | UPPER_SNAKE | `AURORA_AIR_TYPE_H` |
| 模板参数 | CamelCase | `NodeTy`, `T` |

### 2.2 头文件规范

```cpp
// 1. include guard
#ifndef AURORA_MODULE_FILENAME_H
#define AURORA_MODULE_FILENAME_H

// 2. 系统头文件
#include <cstdint>
#include <string>

// 3. 项目头文件
#include "Aurora/ADT/SmallVector.h"

namespace aurora {

// 4. 仅暴露公共接口
class MyClass {
public:
    // 核心 API
    void doSomething();
private:
    // 隐藏实现细节
    class Impl;
    Impl* impl_;
};

} // namespace aurora

#endif
```

### 2.3 禁止事项

- **禁止** 在头文件中包含大段实现代码（模板类除外）
- **禁止** 在头文件中暴露私有成员
- **禁止** 循环依赖
- **禁止** 使用全局可变状态（必要的单例除外，如 Type 系统）
- **禁止** 裸 new/delete 在 hot path 使用（优先 BumpPtrAllocator）
- **推荐** 使用 `std::unique_ptr` 管理所有权
- **推荐** 接口使用 `const&` 传递大对象

---

## 3. 添加新功能

### 3.1 添加新的 AIR 指令

1. 在 `Instruction.h` 的 `AIROpcode` 枚举中添加新值
2. 在 `Instruction.h/cpp` 中添加工厂方法和操作数访问
3. 在 `Builder.h/cpp` 中添加 Builder 包装
4. 在 `X86ISelPatterns.cpp` 中添加匹配模式
5. 在 `X86AsmPrinter.cpp` 中添加汇编输出
6. 在测试中添加对应单元测试

### 3.2 添加新的 x86 指令

1. 在 `X86InstrInfo.h` 的 `X86::Opcode` 枚举中添加新值
2. 在 `X86InstrInfo.cpp` 的 `buildOpcodeTable()` 中添加指令描述
3. 在 `X86InstrEncode.cpp` 的 `X86EncodeTable` 中添加编码信息
4. 在 `X86AsmPrinter.cpp` 的 `emitInstruction()` 中添加汇编格式

### 3.3 添加新的 CodeGen Pass

```cpp
// 1. 创建 inherit CodeGenPass
class MyPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override { /* ... */ }
    const char* getName() const override { return "MyPass"; }
};

// 2. 注册到标准管线
void CodeGenContext::addStandardPasses(PassManager& pm, TargetMachine& tm) {
    // ... 现有 passes
    pm.addPass(std::make_unique<MyPass>());
}
```

### 3.4 添加新的目标架构

参见 [ARCHITECTURE.md](ARCHITECTURE.md) 的扩展计划部分。

---

## 4. 测试指南

### 4.1 运行测试

```bash
# 所有测试
ctest --test-dir build --output-on-failure

# 指定模块
./build/tests/ADT_tests
./build/tests/Air_tests

# 指定测试
./build/tests/ADT_tests --gtest_filter=SmallVectorTest.PushBack

# 运行基准
./build/tests/AuroraBench
```

### 4.2 编写测试

```cpp
#include <gtest/gtest.h>
#include "Aurora/module/Header.h"

TEST(TestSuiteName, TestName) {
    // Arrange
    auto obj = CreateTestObject();

    // Act
    auto result = obj.DoSomething();

    // Assert
    EXPECT_EQ(result, expected_value);
}
```

### 4.3 测试原则

- 每个公开 API 至少一个测试
- 测试边界条件 (空容器、零值、溢出)
- 测试错误路径 (nullptr、无效输入)
- 测试性能敏感路径
- 测试完成后释放所有资源

---

## 5. 调试

### 5.1 AddressSanitizer

```bash
cmake -B build -S . -DCMAKE_CXX_FLAGS="-fsanitize=address"
cmake --build build -j8
./build/tests/CodeGen_tests  # 检测内存错误
```

### 5.2 UndefinedBehaviorSanitizer

```bash
cmake -B build -S . -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
```

### 5.3 常见问题排查

| 症状 | 可能原因 | 检查 |
|------|----------|------|
| Segfault | 空指针解引用 | 检查 Builder 是否设置了 insertPoint |
| Double free | 重复删除 | 检查对象所有权 |
| "free(): invalid size" | 堆破坏 | 检查 SmallVector 使用、activate ASan |
| 汇编输出错误 | 指令选择错误 | 检查 ISelPattern 匹配逻辑 |
| 寄存器分配崩溃 | LiveInterval 越界 | 检查 vreg index 范围 |

---

## 6. 构建系统

### 6.1 模块 CMakeLists.txt 模板

```cmake
add_library(AuroraModule STATIC
    File1.cpp
    File2.cpp
)
target_link_libraries(AuroraModule PUBLIC AuroraDependency)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraModule_shared SHARED
        File1.cpp
        File2.cpp
    )
    target_link_libraries(AuroraModule_shared PUBLIC AuroraDependency)
    set_target_properties(AuroraModule_shared PROPERTIES OUTPUT_NAME AuroraModule)
endif()

install(TARGETS AuroraModule DESTINATION lib)
```

### 6.2 依赖管理

- GoogleTest v1.15.2 — CMake FetchContent 自动下载
- Google Benchmark v1.9.1 — CMake FetchContent 自动下载
- 无其他外部依赖

---

## 7. 发布流程

1. 确保所有测试通过 (`ctest --test-dir build`)
2. 确保基准测试不回归 (`./build/tests/AuroraBench`)
3. 确保 Release 构建零警告
4. 更新 CHANGELOG
5. git tag vX.Y.Z
6. 创建 GitHub Release

---

## 8. 相关资料

- [LLVM Writing a Backend](https://llvm.org/docs/WritingAnLLVMBackend.html)
- [x86-64 ABI (System V)](https://gitlab.com/x86-psABIs/x86-64-ABI)
- [Intel Software Developer Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
