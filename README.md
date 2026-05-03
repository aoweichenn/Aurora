# Aurora — Production-Grade x86-64 Compiler Backend

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-green.svg)](https://cmake.org)

Aurora 是一个面向 C++ 编译器的**生产级 x86-64 后端代码生成框架**，设计参考 LLVM 架构。

## 快速开始

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
./build/tools/aurorac/aurorac
```

## 5 行代码编译函数

```cpp
auto module = std::make_unique<Module>("example");
auto* fn = module->createFunction(fnTy, "add");
AIRBuilder builder(fn->getEntryBlock());
unsigned sum = builder.createAdd(Type::getInt32Ty(), 0, 1);
builder.createRet(sum);   // → addl %esi, %edi ; ret
```

## 模块

| 模块 | 说明 |
|------|------|
| **ADT** | SmallVector, BitVector, Graph, BumpPtrAllocator |
| **AIR** | SSA 中间表示 (50 条 LLVM 风格指令) |
| **Target** | 目标机器抽象 (RegisterInfo, InstrInfo, ABI) |
| **X86** | x86-64 完整实现 (~100 条指令编码) |
| **CodeGen** | SelectionDAG, ISel, Linear Scan RA, Pass 管线 |
| **MC** | AT&T 汇编打印, x86-64 二进制编码 |

## 文档

| 文档 | 说明 |
|------|------|
| [架构设计](docs/ARCHITECTURE.md) | 整体架构、设计哲学、技术选型 |
| [类图与时序图](docs/DIAGRAMS.md) | Mermaid 类图、时序图、流程图 |
| [需求分析](docs/REQUIREMENTS.md) | 功能需求、非功能需求、风险分析 |
| [模块设计](docs/MODULES.md) | 7 个模块的详细设计文档 |
| [API 参考](docs/API_REFERENCE.md) | 完整 API 参考 |
| [用户手册](docs/USER_GUIDE.md) | 快速集成、使用场景、FAQ |
| [开发者指南](docs/DEVELOPER_GUIDE.md) | 编码规范、添加新功能、调试 |
| [完整示例](docs/EXAMPLES.md) | 8 个从简单到复杂的完整案例 |

## 测试

```
[  PASSED  ] ADT_tests       30+ tests
[  PASSED  ] Air_tests       46  tests
[  PASSED  ] Target_tests    30  tests
[  PASSED  ] MC_tests        12  tests
[  PASSED  ] CodeGen_tests   22  tests
─────────────────────────────────────
  TOTAL:                     140+ tests
```

## 性能

| 操作 | 耗时 |
|------|------|
| ISel 单条匹配 | **11 ns** |
| ISel 全部模式 | **262 ns** |
| BitVector Set | ~3 ns/bit |

## 许可证

MIT
