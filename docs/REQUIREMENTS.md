# 需求分析文档

## 1. 项目背景

### 1.1 项目概述

**Aurora** 是一个面向 C++ 编译器的生产级后端代码生成框架。用户已拥有一个自定义的 SSA IR 和 CFG 前/中端，需要设计一个类似 LLVM 的后端将其 IR 转换为 x86-64 汇编代码。

### 1.2 目标用户

- 编译器开发者，已实现前端 SSA/CFG
- 学习 LLVM 后端架构的开发者
- 需要自定义后端但不想依赖 LLVM 完整工具链的项目

### 1.3 设计目标

1. **模块化**：每个独立模块单独编译为库，静态 + 动态
2. **解耦合**：头文件只暴露最少接口，实现全在 .cpp
3. **工业级质量**：生产级代码，完善的测试覆盖
4. **可扩展**：支持添加新目标架构 (ARM64, RISC-V)
5. **性能**：指令选择 ~11ns/条，寄存器分配线性复杂度

---

## 2. 功能需求

### 2.1 核心功能

| ID | 功能 | 优先级 | 状态 |
|----|------|--------|------|
| F1 | SSA 中间表示 (AIR) 定义 | P0 | 完成 |
| F2 | AIR Builder (便捷 IR 构造器) | P0 | 完成 |
| F3 | x86-64 目标描述 (寄存器/指令/ABI) | P0 | 完成 |
| F4 | 指令选择 (AIR → x86 MachineInstr) | P0 | 完成 |
| F5 | 寄存器分配 (Linear Scan) | P0 | 完成 |
| F6 | 栈帧管理 (Prologue/Epilogue) | P0 | 完成 |
| F7 | AT&T 汇编文本输出 | P0 | 完成 |
| F8 | x86-64 二进制指令编码 | P1 | 完成 |
| F9 | Pass 管理器 (可组合 pass) | P1 | 完成 |
| F10 | ELF 对象文件输出 | P2 | 待实现 |

### 2.2 AIR 指令需求

| 类别 | 指令 | 说明 |
|------|------|------|
| 终结指令 | Ret, Br, CondBr, Unreachable | 控制流终止 |
| 二元算术 | Add, Sub, Mul, UDiv, SDiv, URem, SRem | 整数运算 |
| 位运算 | And, Or, Xor, Shl, LShr, AShr | 按位操作 |
| 比较 | ICmp (EQ/NE/UGT/UGE/ULT/ULE/SGT/SGE/SLT/SLE) | 整数比较 |
| 内存 | Alloca, Load, Store, GetElementPtr | 栈分配、读/写、地址计算 |
| 转换 | SExt, ZExt, Trunc, FpToSi, SiToFp, BitCast | 类型转换 |
| 控制流 | Phi, Select | SSA φ 节点、条件选择 |
| 调用 | Call | 函数调用 |

### 2.3 x86-64 指令覆盖

| 类别 | 数量 | 示例 |
|------|------|------|
| 数据传送 | ~15 | MOV32rr, MOV64rm, MOVSX32rr8 |
| 算术 | ~20 | ADD64rr, SUB64ri32, IMUL64rr, IDIV64r |
| 位运算 | ~20 | AND64rr, OR64ri8, XOR64rr, SHL64rCL |
| 比较/设置 | ~14 | CMP64rr, SETEr, SETLr |
| 分支 | ~20 | JE_1, JNE_4, JL_1, JMP_4 |
| 调用/返回 | ~3 | CALL64pcrel32, RETQ |
| 栈操作 | ~3 | PUSH64r, POP64r, LEA64r |
| 其他 | ~5 | NOP, CQO, MOVSDrm |

### 2.4 ABI 需求

- System V AMD64 调用约定
- 参数寄存器: RDI, RSI, RDX, RCX, R8, R9
- 浮点参数: XMM0-XMM7
- 返回值: RAX (整数), XMM0 (浮点)
- 被调用者保存: RBX, RBP, R12-R15
- 调用者保存: 其余 GPR
- 栈对齐: 16 字节
- Red Zone: 128 字节

---

## 3. 非功能需求

### 3.1 性能需求

| 指标 | 目标 | 实际 |
|------|------|------|
| 指令选择 (单条) | < 50 ns | 11 ns |
| 指令选择 (全部 30 条) | < 1 μs | 262 ns |
| BitVector Set | < 10 ns/bit | ~3 ns/bit |
| 编译大型模块 | < 1s (10K functions) | TBD |
| 汇编输出吞吐 | > 100K instr/s | TBD |

### 3.2 质量需求

- 编译零警告 (-Wall -Wextra -Wpedantic)
- 单元测试覆盖 > 90%
- 基准测试回归检测
- 内存管理清晰（无悬垂指针、无泄漏在关键路径）

### 3.3 可维护性需求

- 头文件/实现严格分离
- 每个模块独立编译为静态库 + 动态库
- CMake 构建系统，一键配置 + 编译 + 测试
- 完整文档（架构/API/开发/示例）

### 3.4 可扩展性需求

- 新目标架构添加只需实现 TargetMachine 子类
- 新指令选择模式只需添加 ISelPattern 条目
- 新 Pass 只需继承 CodeGenPass
- 新汇编语法只需继承 AsmPrinter

---

## 4. 约束条件

| 约束 | 说明 |
|------|------|
| 语言 | C++17 |
| 编译器 | GCC 15+ / Clang 18+ |
| 平台 | Linux x86-64 (WSL2 可编译) |
| 构建 | CMake 3.16+ |
| 依赖 | GoogleTest (v1.15) / Google Benchmark (v1.9) (自动下载) |
| 编码规范 | LLVM 风格 (lower_case 函数, CamelCase 类) |

---

## 5. 使用场景

### 场景 1：简单算术函数编译

```cpp
int add(int a, int b) { return a + b; }
```
→ AIR → SelectionDAG → ISel(ADD32rr) → RA → `addl %esi, %edi` → `movl %edi, %eax` → `ret`

### 场景 2：带控制流的函数

```cpp
int abs(int x) { return x < 0 ? -x : x; }
```
→ condbr + neg + phi + ret
→ CMP + JGE + NEG + JMP + L.add

### 场景 3：函数调用

```cpp
int square(int x) { return x * x; }
int main() { return square(5); }
```
→ call square → RAX 返回值

### 场景 4：内存操作

```cpp
void swap(int* a, int* b) { int t = *a; *a = *b; *b = t; }
```
→ Load + Load + Store + Store

---

## 6. 风险分析

| 风险 | 概率 | 影响 | 缓解 |
|------|------|------|------|
| x86-64 编码复杂度 | 高 | 中 | 使用静态编码表简化 |
| 寄存器分配正确性 | 中 | 高 | 充分测试 + ASan |
| 多目标扩展困难 | 低 | 中 | 清晰的目标抽象接口 |
| 性能不达预期 | 低 | 中 | 基准测试持续监控 |

---

## 7. 术语表

| 术语 | 全称 | 说明 |
|------|------|------|
| AIR | Aurora Intermediate Representation | 后端通用 SSA IR |
| MIr | Machine IR | 机器级 SSA IR |
| ISel | Instruction Selection | 指令选择 |
| RA | Register Allocation | 寄存器分配 |
| MBB | Machine Basic Block | 机器基本块 |
| MI | Machine Instruction | 机器指令 |
| MO | Machine Operand | 机器操作数 |
| SDNode | SelectionDAG Node | DAG 节点 |
| DAG | Directed Acyclic Graph | 有向无环图 |
