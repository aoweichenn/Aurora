# Aurora

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../../LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-green.svg)](https://cmake.org)

Aurora 是一个 C++17 x86-64 编译器后端与代码生成练习项目。
当前仓库包含 AIR 中间表示、目标抽象层、x86 专用 lowering 与编码、ELF 可重定位对象写出器，以及用于示例工具和测试的 MiniC 前端。

## 快速开始

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
./build/tools/minic/minic test.mini
```

## 文档目录

- `ARCHITECTURE.md`：当前架构、流水线和模块分层
- `API_REFERENCE.md`：当前公开接口清单
- `USER_GUIDE.md`：构建、运行、集成方式
- `DEVELOPER_GUIDE.md`：扩展 AIR、x86 指令、Pass、MiniC 语法的流程
- `EXAMPLES.md`：AIR、MiniC、汇编输出、对象文件输出示例
- English: `../en/README.md`

## 当前范围

- `ADT`：`SmallVector`、`BitVector`、`Graph`、`SparseSet`、`BumpPtrAllocator`
- `AIR`：类型、常量、函数、基本块、指令、Builder、Module
- `Target`：寄存器、指令、lowering、调用约定、栈帧 lowering 抽象
- `X86`：x86-64 目标层具体实现
- `CodeGen`：SelectionDAG 框架、指令选择、寄存器分配、Pass 管线
- `MC`：汇编打印、对象编码、ELF relocatable writer
- `tools/minic`：Mini 语言的词法、语法和 AIR 生成
- `tools/aurorac`：后端流水线示例驱动
- `tools/aurora-obj`：ELF 对象写出示例驱动

## 说明

- 中文和英文文档保持一一对应。
- 目前仓库为 ADT、AIR、Target、CodeGen、MC 都配套了测试。
- 当前实现重点是可运行、可测试、可扩展，文档只描述已经落地的能力。
