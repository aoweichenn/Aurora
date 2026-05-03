# 模块设计文档

## 1. ADT 模块 (AuroraADT)

### 1.1 概述

基础数据结构库，为所有其他模块提供高性能、零依赖的数据结构。

### 1.2 组件

#### SmallVector<T, N>

栈预分配的动态数组，类似 LLVM 的 `SmallVector`。

```cpp
template <typename T, unsigned N = 8>
class SmallVector {
    // 少于 N 个元素时使用栈内存储 (inline buffer)
    // 超出后自动切换到堆分配
    // 支持所有标准容器操作
};
```

**设计要点**：
- 小容量时零堆分配
- 增长策略：2x 扩容
- 支持跨 N 的拷贝赋值 (SmallVector<int,4> = SmallVector<int,8>)

#### BitVector

高效的位向量，基于 `uint64_t` 字数组。

```cpp
class BitVector {
    // 支持 set/test/reset 单 bit 操作
    // 支持位运算 (|, &, ^, ~)
    // 支持 find_first / find_next 扫描
    // 使用 __builtin_ctzll / __builtin_popcountll 内建函数
};
```

#### DirectedGraph<NodeTy>

泛型有向图，用于 CFG、DAG、支配树等。

```cpp
template <typename NodeTy>
class DirectedGraph {
    // O(1) 后继/前驱查询
    // 支持逆后序遍历 (RPO)
    // 支持后序遍历 (PO)
};
```

#### BumpPtrAllocator

Bump-pointer (Arena) 分配器，用于 Pass 内的临时对象。

```cpp
class BumpPtrAllocator {
    // 4KB Slab 链式分配
    // placement new 支持 (create<T>(args...))
    // reset() 复用内存不释放
};
```

---

## 2. AIR 模块 (AuroraAir)

### 2.1 概述

后端通用 SSA 中间表示。设计参考 LLVM IR 但更简化。

### 2.2 类型系统 (Type)

```
Type 层次:
  VoidType   (0 bit)
  IntegerType (1/8/16/32/64 bit)
  FloatType   (32/64 bit)
  PointerType (指向任意 Type)
  ArrayType   (固定长度 + 元素类型)
  StructType  (成员列表)
  FunctionType (返回类型 + 参数列表)
```

**核心设计**：类型唯一化 (Flyweight pattern)。全局类型池确保相同参数的类型对象复用。

```cpp
auto* p1 = Type::getPointerTy(Type::getInt32Ty());
auto* p2 = Type::getPointerTy(Type::getInt32Ty());
assert(p1 == p2); // 同一对象
```

### 2.3 常量系统 (Constant)

```
Constant (基类)
  ├── ConstantInt    (整数常量: 1/8/16/32/64 bit)
  ├── ConstantFP     (浮点常量: float/double)
  ├── ConstantArray  (数组常量)
  ├── ConstantStruct (结构体常量)
  ├── Function       (函数引用)
  └── GlobalVariable (全局变量)
```

### 2.4 指令 (AIRInstruction)

50 条细粒度指令，每条 ≈ 一次 CPU 操作。

**生命周期**：
```
创建: AIRBuilder::createAdd(...) → new AIRInstruction(...)
归属: BasicBlock 拥有所有指令 (双向链表)
析构: BasicBlock::~BasicBlock() 遍历删除
```

**SSA 形式**：
- 每条产生值的指令有唯一的 `destVReg_` (虚拟寄存器 ID)
- Φ 指令在交汇基本块合并多个来源的值
- `replaceUse()` 支持 use-def 链改写

### 2.5 Builder (AIRBuilder)

RAII 风格的 IR 构造器。

```cpp
AIRBuilder builder(entryBlock);

// 算术: 返回 vreg ID
unsigned v0 = builder.createAdd(ty, lhs, rhs);
unsigned v1 = builder.createMul(ty, v0, 3);

// 控制流
builder.createCondBr(cond, trueBB, falseBB);

// 切换到另一块继续构造
builder.setInsertPoint(anotherBB);
unsigned phi = builder.createPhi(ty, incomings);
```

---

## 3. Target 模块 (AuroraTarget)

### 3.1 概述

目标机器抽象层，定义后端必须实现的所有接口。

### 3.2 抽象接口

| 接口 | 职责 |
|------|------|
| TargetMachine | 工厂，聚合所有子组件 |
| TargetRegisterInfo | 寄存器描述 (GPR/XMM/Flag) |
| TargetInstrInfo | 机器指令描述 (opcode → 名称/操作数) |
| TargetLowering | 类型/操作合法化 |
| TargetCallingConv | 调用约定 (参数/返回值分配) |
| TargetFrameLowering | 栈帧管理 (Prologue/Epilogue) |

### 3.3 扩展新目标

添加新目标 (如 ARM64) 只需：

```cpp
class ARM64TargetMachine : public TargetMachine { ... };
class ARM64RegisterInfo : public TargetRegisterInfo { ... };
class ARM64InstrInfo   : public TargetInstrInfo { ... };
class ARM64TargetLowering : public TargetLowering { ... };
class ARM64CallingConv : public TargetCallingConv { ... };
class ARM64FrameLowering : public TargetFrameLowering { ... };
class ARM64AsmPrinter   : public AsmPrinter { ... };
```

---

## 4. X86 模块 (AuroraX86)

### 4.1 概述

x86-64 目标的完整实现。

### 4.2 寄存器

16 个 64-bit GPR: RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8-R15
16 个 128-bit XMM: XMM0-XMM15

**寄存器类**: GPR8 ⊂ GPR16 ⊂ GPR32 ⊂ GPR64

### 4.3 指令描述

约 100 条 x86-64 指令，每条有完整的元数据：

```cpp
MachineOpcodeDesc {
    .opcode       = X86::ADD64rr,
    .asmString    = "addq\t$src, $dst",
    .numOperands  = 2,
    .isTerminator = false,
    .isCompare    = false,
    .isMove       = false,
}
```

### 4.4 指令选择模式

```cpp
ISelPattern {
    .airOp    = AIROpcode::Add,
    .opKinds  = {Reg, Reg},
    .opSizes  = {S64, S64},
    .x86Opcode = X86::ADD64rr,
}
```

### 4.5 编码表

每个 x86 opcode 有对应的编码信息：

```cpp
X86EncodeEntry {
    .opcode     = X86::ADD64rr,
    .baseOpcode = {0x01},
    .opcodeSize = 1,
    .hasModRM   = true,
    .hasREX     = true,  // REX.W for 64-bit
}
```

---

## 5. CodeGen 模块 (AuroraCodeGen)

### 5.1 Machine IR

- **MachineInstr**: 机器指令 (opcode + 操作数列表)
- **MachineBasicBlock**: 机器基本块 (指令链表 + CFG 边)
- **MachineFunction**: 机器函数 (MBB 列表 + 虚拟寄存器 + 栈槽)

### 5.2 SelectionDAG

每个基本块内构建 DAG，节点为 AIR 操作用 use-def 链连接。

```
阶段:
  1. buildFromBasicBlock:    AIR inst → SDNode
  2. dagCombine:             合并冗余节点
  3. legalize:               类型/操作合法化
  4. select:                 模式匹配 → 产生 MachineInstr
  5. schedule:               DAG → 线性指令序列
```

### 5.3 寄存器分配器

**Linear Scan 算法**：

```
1. computeLiveIntervals(): 数据流分析, 计算每个 vreg 的活性区间
2. linearScan():
   - 按 start 排序所有 intervals
   - 维护 Active 列表
   - 对于每个 interval:
     a. expireOldIntervals: 移除已结束的
     b. tryAllocateFreeReg: 尝试分配空闲寄存器
     c. 若无空闲 → selectRegToSpill (选择溢出)
3. rewriteAfterAllocation: 虚拟寄存器 → 物理寄存器
```

**Spill 策略**：
- 选择最远下次使用的 interval 溢出
- 溢出到栈：`createStackSlot(8, 8)`
- 插入 load/store spill code

### 5.4 Pass 管理器

```cpp
class PassManager {
    std::vector<std::unique_ptr<CodeGenPass>> passes_;
public:
    void addPass(std::unique_ptr<CodeGenPass>);
    void run(MachineFunction&);
};

// 标准管线
void addStandardPasses(PassManager& pm) {
    pm.addPass(AIRToMachineIR);
    pm.addPass(Peephole);
    pm.addPass(PhiElimination);
    pm.addPass(RegisterCoalescer);
    pm.addPass(RegisterAllocator);
    pm.addPass(PrologueEpilogueInserter);
    pm.addPass(BranchFolder);
}
```

---

## 6. MC 模块 (AuroraMC)

### 6.1 输出流 (MCStreamer)

抽象输出接口，支持文本和二进制两种模式：

```
MCStreamer (抽象)
  ├── AsmTextStreamer  → 输出到 std::ostream
  └── (未来) BinaryStreamer → 输出到内存 buffer
```

### 6.2 汇编打印 (X86AsmPrinter)

将 MachineInstr 转换为 AT&T 语法汇编文本：

```
格式:  <mnemonic>\t<src>, <dst>

例:
  ADD64rr (vreg0, vreg1) → addq %rax, %rbx
  MOV64ri32 (42, vreg0)  → movq $42, %rax
  RETQ                   → ret
```

### 6.3 指令编码 (X86InstEncoder)

x86-64 变长指令编码引擎：

```
编码流程:
  1. 查找 X86EncodeTable 获取编码模板
  2. emitPrefixes: 输出 0x66/0xF2/0xF3/REX 前缀
  3. emitOpcode: 输出 1-3 字节主操作码
  4. emitModRM: 编码 ModRM 字节 (mod:2bit, reg:3bit, rm:3bit)
  5. emitSIB: 编码 SIB 字节 (scale:2bit, index:3bit, base:3bit)
  6. emitDisp: 输出偏移量 (1/2/4 字节)
  7. emitImm: 输出立即数 (1/2/4/8 字节)
```
