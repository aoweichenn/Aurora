# 架构

Aurora 当前阶段是一个以 x86-64 为目标的编译后端和示例前端组合项目。
整体流水线如下：

```text
Mini 源码 -> Lexer -> Parser -> AST -> MiniC CodeGen -> AIR Module
-> PassManager -> MachineFunction/MIR -> AsmPrinter 或 ObjectWriter -> 汇编/ELF 对象
```

## 分层

| 层级 | 作用 |
|------|------|
| `ADT` | 基础容器和工具：`SmallVector`、`BitVector`、`Graph`、`SparseSet`、`BumpPtrAllocator` |
| `AIR` | SSA 风格中间表示：类型、常量、函数、基本块、指令、Builder、Module |
| `Target` | 目标抽象：寄存器、指令、lowering、调用约定、栈帧 |
| `Target/X86` | x86-64 的具体实现 |
| `CodeGen` | AIR 到 MIR、指令选择、寄存器分配、栈帧插入、branch folding |
| `MC` | 汇编打印、机器码编码、ELF relocatable 输出 |
| `tools/minic` | Mini 语言前端 |

## 关键对象

- `Module` 持有 `Function` 和 `GlobalVariable`。
- `Function` 持有 `BasicBlock`，并管理虚拟寄存器编号和类型信息。
- `AIRBuilder` 负责在当前插入点生成 AIR 指令。
- `PassManager` 运行标准流水线，`CodeGenContext::addStandardPasses` 会注册默认 passes。
- `MachineFunction` 承载 MIR、栈对象、虚拟寄存器类型和目标信息。
- `SelectionDAG` 提供 DAG 节点、常量、寄存器引用和基本的选择/调度接口。
- `AsmPrinter` 与 `ObjectWriter` 是两条后端输出路径，分别面向文本汇编和 ELF 对象。

## 当前流水线

1. AIR to MachineIR：把 AIR 指令直接映射到 MIR。
2. Instruction Selection：把 AIR 操作降成目标指令。
3. Register Allocation：线性扫描分配物理寄存器，必要时溢出到栈。
4. Prologue/Epilogue Insertion：插入函数序言和尾声。
5. Branch Folding：折叠冗余跳转并线程化简单跳转链。

## 约束

- 目前实现重点是可运行与可测试，不是完整 LLVM 等级优化器。
- 选择 DAG 相关接口已经存在，但更多是当前阶段的结构入口。
- 文档和代码都应保持中英文对应。
