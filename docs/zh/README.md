# Aurora - 生产级 x86-64 编译器后端

Aurora 是一个 C++17 编写的生产级 x86-64 编译器后端框架，仿 LLVM 架构设计。将自定义 SSA IR（AIR）转换为 x86-64 AT&T 汇编和 ELF 目标文件。

## 快速开始

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/tools/minic/minic test.mini
./build/tools/aurora-obj/aurora-obj test.o
```

## 模块架构

```
ADT (数据结构) → Air (SSA IR) → Target (平台抽象)
                                  ↓
                                X86 (x86-64 实现)
                                  ↓
CodeGen (指令选择，寄存器分配) → MC (汇编/ELF 输出)
```

| 模块 | 用途 |
|------|------|
| ADT | SmallVector, BitVector, Graph, BumpAllocator, SparseSet |
| Air | SSA 中间表示：~40 条指令，类型系统，AIRBuilder |
| Target | 目标机器的抽象基类 |
| X86 | x86-64 寄存器信息，指令集，ISel 模式表，调用约定，栈帧管理 |
| CodeGen | SelectionDAG, InstructionSelector, LinearScan 寄存器分配器, Pass 管理器 |
| MC | 汇编打印器（AT&T 语法）, ELF 对象文件写入器 |

## 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `AURORA_BUILD_TESTS` | ON | 构建测试套件 (GoogleTest) |
| `AURORA_BUILD_BENCHMARKS` | ON | 构建基准测试 (Google Benchmark) |
| `AURORA_BUILD_SHARED_LIBS` | ON | 构建共享库 |
| `AURORA_ENABLE_COVERAGE` | OFF | 启用 lcov 代码覆盖率 |

## 覆盖率

```bash
cmake -B build -S . -DAURORA_ENABLE_COVERAGE=ON -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/_deps/*' '*/tests/*' -o coverage.info
genhtml coverage.info --output-directory coverage_report
```

## 工具

- **minic**: 迷你语言编译器（表达式 → AIR → 汇编）
- **aurorac**: 示例 AIR 构建和汇编输出
- **aurora-obj**: AIR → ELF .o 文件写入器

## 许可证

MIT License
