# 开发者指南

## 1. 目录结构

- `include/Aurora/ADT`：基础容器与工具
- `include/Aurora/Air`：AIR IR 与 Builder
- `include/Aurora/Target`：目标抽象与 x86-64/AArch64 具体实现
- `include/Aurora/CodeGen`：MIR、SelectionDAG、Pass、寄存器分配
- `include/Aurora/MC`：汇编打印、对象写出、编码器
- `tools/minic`：C-like 示例前端
- `tools/aurorac`：后端样例驱动
- `tools/aurora-obj`：对象文件样例驱动
- `tests`：按模块组织的单元测试与覆盖测试

## 2. 代码约定

- 公共接口优先放在 `include/`，实现放在 `lib/`。
- 新行为必须补测试。
- 文档改动要同步中英文两套文件。
- 当前阶段以可运行、可测试、可维护为目标，不要为暂时不会使用的功能引入复杂抽象。

## 3. 添加新的 AIR 指令

1. 在 `include/Aurora/Air/Instruction.h` 的 `AIROpcode` 中添加枚举。
2. 在 `lib/Air/Instruction.cpp` 中添加工厂方法和访问器。
3. 在 `include/Aurora/Air/Builder.h` 与 `lib/Air/Builder.cpp` 中增加 Builder 包装。
4. 更新 `tools/minic` 的语法/代码生成，如需要。
5. 在 `lib/CodeGen/Passes/PassManager.cpp` 中补 lowering。
6. 在 `lib/MC/X86/X86AsmPrinter.cpp` 和 `lib/MC/X86/X86ObjectEncoder.cpp` 中补打印与编码。
7. 在 `tests/Air`、`tests/CodeGen`、`tests/MC` 中补测试。

## 4. 添加新的 x86 指令

1. 在 `include/Aurora/Target/X86/X86InstrInfo.h` 中添加 opcode。
2. 在 `lib/Target/X86/X86InstrInfo.cpp` 中补 opcode 描述。
3. 在 `lib/MC/X86/X86AsmPrinter.cpp` 中补汇编打印。
4. 在 `lib/MC/X86/X86ObjectEncoder.cpp` 中补编码。
5. 如需要，在 `lib/CodeGen/Passes/PassManager.cpp` 中让 lowering 使用它。
6. 为编码和打印补回归测试。

## 5. 添加新的 Pass

1. 新建 `CodeGenPass` 子类。
2. 在 `include/Aurora/CodeGen/Passes/PassFactories.h` 声明工厂。
3. 在 `lib/CodeGen/Passes/PassManager.cpp` 提供工厂实现。
4. 在 `CodeGenContext::addStandardPasses()` 中注册。
5. 给 pass 行为补 `tests/CodeGen` 覆盖。

## 6. 添加新的前端语法

1. 扩展 `tools/minic/Lexer.h` 与 `tools/minic/Lexer.cpp`。
2. 扩展 `tools/minic/Parser.h` 与 `tools/minic/Parser.cpp`。
3. 更新 `tools/minic/AST.h` 和 `tools/minic/CodeGen.cpp`。
4. 为新语法写 `tests/` 覆盖。

## 7. 调试建议

- 先跑最小化单测，再跑对应模块测试。
- 需要看汇编时优先用 `tools/minic`。
- 需要看对象文件时优先用 `tools/aurora-obj`。
- 新接口要先在头文件定型，再补实现和测试。
