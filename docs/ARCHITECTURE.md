# Aurora 架构设计文档

## 1. 设计哲学

### 1.1 核心原则

- **关注点分离**：每个模块只暴露最小接口，实现全在 `.cpp` 文件
- **零循环依赖**：模块依赖图是严格的 DAG，无环
- **独立编译**：每个模块产出独立的静态库 + 动态库
- **工业级质量**：C++17 标准，-Wall -Wextra -Wpedantic 零警告

### 1.2 与 LLVM 的对应关系

| LLVM 概念 | Aurora 对应 | 简化点 |
|-----------|------------|--------|
| LLVM IR | AIR | 50条指令，只用整数+浮点+指针 |
| TableGen | 静态 C++ 表 (ISelPatterns) | 无需代码生成工具 |
| SelectionDAG | SelectionDAG | 保留核心 DAG 概念 |
| FastISel | 直接 AIR→MIr 降级 | 1-to-N 映射代替完整 DAG 匹配 |
| GreedyRegAlloc | LinearScanRegAlloc | 单遍线性扫描 |
| MC Layer | MC 模块 | AT&T 语法 + 二进制编码 |

### 1.3 技术选型

| 决策 | 选择 | 理由 |
|------|------|------|
| 语言 | C++17 | 性能、与现有项目一致 |
| 构建系统 | CMake 3.16+ | 跨平台、FetchContent 集成 |
| 测试框架 | GoogleTest v1.15 | 工业标准 |
| 性能测试 | Google Benchmark v1.9 | 微基准测试 |
| IR 粒度 | 细粒度 (LLVM 风格) | 每条指令≈一次CPU操作 |
| 指令选择 | SelectionDAG + 模式表 | LLVM 标准方案 |
| 寄存器分配 | Linear Scan | 编译快、代码质量好 |
| 目标架构 | x86-64 (System V AMD64 ABI) | 最常用平台 |

---

## 2. 整体架构

### 2.1 分层架构图

```
┌──────────────────────────────────────────────────┐
│                  Front-end (用户项目)               │
│                自定义 SSA + CFG IR                  │
└─────────────────────┬────────────────────────────┘
                      │ IR Bridge
┌─────────────────────▼────────────────────────────┐
│                    AIR 层                           │
│  ┌─────────────────────────────────────────────┐ │
│  │ Type   │ Constant │ Instruction │ ~50 ops   │ │
│  │ BasicBlock │ Function │ Module │ Builder   │ │
│  └─────────────────────────────────────────────┘ │
└─────────────────────┬────────────────────────────┘
                      │
┌─────────────────────▼────────────────────────────┐
│                  Target 层                         │
│  ┌───────────────┐  ┌─────────────────────────┐  │
│  │ TargetMachine │  │ X86 RegisterInfo         │  │
│  │ TargetLowering│  │ X86 InstrInfo (300+ opc)  │  │
│  │ TargetCalling │  │ X86 CallingConv (ABI)    │  │
│  │ TargetFrame   │  │ X86 FrameLowering        │  │
│  └───────────────┘  │ X86 ISelPatterns         │  │
│                      │ X86 InstrEncode (ModRM)  │  │
│                      └─────────────────────────┘  │
└─────────────────────┬────────────────────────────┘
                      │
┌─────────────────────▼────────────────────────────┐
│                 CodeGen 层                         │
│  ┌─────────────┐  ┌───────┐  ┌──────────────┐   │
│  │ MachineInstr │→│ SelDAG│→│ ISel          │   │
│  │ MachineBB    │  │       │  │ PatternMatch │   │
│  │ MachineFunc  │  └───────┘  └──────┬───────┘   │
│  └─────────────┘                     │           │
│                   ┌──────────────────▼────────┐  │
│                   │ RegisterAllocator         │  │
│                   │ (Linear Scan + Spill)     │  │
│                   └──────────────────┬────────┘  │
│                   ┌──────────────────▼────────┐  │
│                   │ PrologueEpilogueInserter   │  │
│                   └──────────────────┬────────┘  │
│                   ┌──────────────────▼────────┐  │
│                   │ PassManager               │  │
│                   │ (AIR→MIr | Peephole |     │  │
│                   │  PhiElim | Coalescer |    │  │
│                   │  BranchFold)              │  │
│                   └──────────────────────────┘  │
└─────────────────────┬────────────────────────────┘
                      │
┌─────────────────────▼────────────────────────────┐
│                    MC 层                           │
│  ┌──────────────┐  ┌────────────────────────┐    │
│  │ MCStreamer   │  │ X86AsmPrinter (AT&T)   │    │
│  │ AsmTextStream│  │ X86InstEncoder (binary) │    │
│  └──────────────┘  │ ELFObjectWriter (future) │   │
│                     └────────────────────────┘    │
└─────────────────────┬────────────────────────────┘
                      │
                      ▼
              x86-64 .s / ELF .o
```

### 2.2 代码生成流水线

```
   AIR Function (SSA)
         │
    ┌────▼─────────────────────┐
    │ AIRToMachineIR Pass       │  每个 AIR BB → Machine BB
    │ (直译: opcode → opcode)   │  保持 SSA 形式
    └────┬─────────────────────┘
         │ MachineFunction (虚拟寄存器 SSA)
    ┌────▼─────────────────────┐
    │ Build SelectionDAG        │  每个 MBB 内建 DAG
    │ (操作数use-def链)         │
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ DAGCombine               │  合并冗余节点
    │ (例如: x+ x → lea)       │
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ LegalizeTypes             │  i128 → 2×i64
    │                          │  f64 → SSE
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ LegalizeOps               │  udiv i8 → promote → udiv i32
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Instruction Select        │  查找 ISEL_PATTERN
    │ (PatternMatch)            │  产生 MachineInstr
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Schedule DAG              │  依赖排序 → 线性指令序列
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Phi Elimination           │  关键边分割 + COPY 插入
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Two-Address               │  x86 dest=src 约束处理
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Register Coalescing       │  合并 COPY 链
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Register Allocation       │  虚拟寄存器 → 物理寄存器
    │ (Linear Scan)             │  活性分析 + 分配 + Spill
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Prologue/Epilogue Insert  │  push rbp; sub rsp,N;
    │                          │  ... ; pop rbp; ret
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Branch Folding            │  合并连续跳转
    └────┬─────────────────────┘
    ┌────▼─────────────────────┐
    │ Peephole                  │  mov rax,0 → xor eax,eax
    └────┬─────────────────────┘
         │ 物理寄存器 + 栈布局完整
    ┌────▼─────────────────────┐
    │ MC 发射                   │
    │ X86AsmPrinter → .s       │
    │ X86InstEncoder → .o      │
    └──────────────────────────┘
```

---

## 3. 关键设计决策

### 3.1 AIR 指令设计

**选择细粒度 LLVM 风格**，约 50 条指令，每条对应一次 CPU 操作：

```
终结指令:  Ret, Br, CondBr, Unreachable
算术指令:  Add, Sub, Mul, UDiv, SDiv, URem, SRem
位运算:    And, Or, Xor, Shl, LShr, AShr
比较:      ICmp (9 种条件)
内存:      Alloca, Load, Store, GetElementPtr
转换:      SExt, ZExt, Trunc, FpToSi, SiToFp, BitCast
控制流:    Phi, Select
调用:      Call
```

### 3.2 SSA 保持策略

AIR 和 MachineIR 都保持 SSA 形式（含 φ 节点），直到寄存器分配阶段才 SSA 析构。这允许：
- 前端的 SSA 优化 pass 直接工作
- 寄存器分配时利用 use-def 链做活性分析

### 3.3 虚拟寄存器设计

```
函数参数:  vreg 0, 1, 2, ...  (自动分配)
局部变量:  由 Builder 自动递增分配
数量:      无上限，寄存器分配时映射到物理寄存器或栈
```

### 3.4 目标描述方式

不使用 TableGen 代码生成工具，改用 **C++ 静态表** 驱动：

```cpp
struct ISelPattern {
    AIROpcode airOp;        // 匹配哪个 AIR 操作
    uint8_t operandCount;   // 操作数数量
    OperandKind opKinds[4]; // 操作数约束 [REG|IMM|MEM]
    OperandSize opSizes[4]; // 操作数大小 [S8|S16|S32|S64]
    uint16_t x86Opcode;     // 产生哪条 x86 指令
};
```

优势：无需额外构建步骤，编译器直接看到所有模式。

### 3.5 指令编码方式

x86-64 变长指令编码通过 **静态编码表** 描述：

```cpp
struct X86EncodeEntry {
    uint16_t opcode;           // 内部 opcode
    uint8_t prefixes[4];       // 0x66, 0xF2, 0xF3
    uint8_t baseOpcode[3];     // 主操作码 (1-3 字节)
    uint8_t hasModRM : 1;      // 是否需要 ModR/M 字节
    uint8_t hasSIB : 1;        // 是否需要 SIB 字节
    uint8_t hasREX : 1;        // 是否需要 REX 前缀
    uint8_t immSize : 3;       // 立即数大小: 0,1,2,4,8
    uint8_t dispSize : 3;      // 偏移大小
};
```

---

## 4. 模块依赖关系

```
                    ┌──────────┐
                    │  ADT     │  SmallVector, BitVector, Graph, Allocator
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │  Air     │  Type, Instruction, BasicBlock, Function, Module
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │  Target  │  TargetMachine, TargetRegisterInfo, ...
                    └────┬─────┘
                         │
              ┌──────────▼──────────┐
              │       X86           │  X86RegisterInfo, X86InstrInfo, ...
              └──────────┬──────────┘
                         │
              ┌──────────▼──────────┐
              │     CodeGen         │  MachineInstr, SelectionDAG, ISel, RA
              └──────────┬──────────┘
                         │
              ┌──────────▼──────────┐
              │        MC           │  MCStreamer, AsmPrinter, X86AsmPrinter
              └──────────┬──────────┘
                         │
                    ┌────▼─────┐
                    │ aurorac  │  驱动工具
                    └──────────┘
```

| 模块 | 头文件数 | .cpp文件数 | 功能 |
|------|---------|-----------|------|
| ADT | 5 | 5 | 数据结构基础库 |
| Air | 7 | 7 | 后端通用 IR |
| Target | 6 | 6 | 目标机器抽象层 |
| X86 | 8 | 8 | x86-64 目标特化 |
| CodeGen | 9 | 9 | 代码生成管线 |
| MC | 3 | 4 | 机器码发射 |
| aurorac | 1 | 1 | 驱动入口 |
| **总计** | **40** | **40** | ~8000 行 C++ |

---

## 5. 数据流

### 5.1 类型数据流

```
Type::getInt32Ty()        ← 静态方法
     │
     ▼
  [全局类型池] (std::unordered_map)
     │  派生类型: Pointer, Array, Function 使用 TypeKey hash
     │  唯一化: 相同参数返回同一对象 (Flyweight)
     ▼
  Type* (不可变)
```

### 5.2 指令数据流

```
AIRBuilder.createAdd(ty, lhs, rhs)
     │
     ▼  allocateVReg(ty)  ← Function::nextVReg()
     │
     ▼  new AIRInstruction(AIROpcode::Add, ty)
     │  setDestVReg(vreg)
     │  insertInstruction(inst) ← BasicBlock::pushBack
     ▼
   BasicBlock 指令链表
     │
     ▼  AIRToMachineIRPass::run
     │  逐条翻译 AIR inst → MachineInstr
     ▼
   MachineBasicBlock 指令链表
```

### 5.3 寄存器数据流

```
MachineFunction (虚拟寄存器 SSA)
     │
     ▼  LiveInterval 计算
     │  def-use 数据流分析
     ▼
  [LiveInterval 列表]
     │
     ▼  LinearScanRegAlloc::allocateRegisters()
     │  排序 → 扫描 → 分配物理寄存器
     │  冲突 → Spill 到栈
     ▼
  物理寄存器 + Stack Slots
```

---

## 6. 内存管理

| 对象类型 | 分配方式 | 生命周期 |
|----------|---------|---------|
| Type | `new` (全局单例) | 程序生命周期 |
| AIRInstruction | `new` (BasicBlock 管理) | BasicBlock 生命周期 |
| MachineInstr | `new` (MachineBB 管理) | MachineBB 生命周期 |
| SDNode | `new` (SelectionDAG 管理) | DAG 生命周期 |
| 临时数据 | BumpPtrAllocator | 每 Pass 重置 |

---

## 7. 错误处理

当前采用 **fail-fast** 策略：
- 断言 (assert) 用于内部一致性检查
- 返回 nullptr 用于查找失败
- 未来计划：集成 LLVM 风格的 `Error`/`Expected<T>` 或 C++23 `std::expected`

---

## 8. 扩展计划

| 阶段 | 内容 | 预计工作量 |
|------|------|-----------|
| P1 | 完善 SelectionDAG 调度器 | 2 周 |
| P2 | Graph Coloring 寄存器分配器 | 3 周 |
| P3 | ELF 对象文件写入 | 2 周 |
| P4 | 更多 x86 扩展 (AVX, SSE4.2) | 4 周 |
| P5 | ARM64 目标 | 6 周 |
| P6 | JIT 编译引擎 | 4 周 |
