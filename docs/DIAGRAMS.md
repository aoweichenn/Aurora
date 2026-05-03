# 类图与时序图

## 1. 类图

### 1.1 ADT 模块

```mermaid
classDiagram
    class SmallVector~T,N~ {
        -inlineBuf_[N]
        -begin_ : T*
        -end_ : T*
        -capacity_ : T*
        +size() size_type
        +push_back(T)
        +pop_back()
        +clear()
        +reserve(n)
        +operator[](i) T&
    }
    
    class BitVector {
        -words_ : uint64_t*
        -numWords_ : unsigned
        -numBits_ : unsigned
        +set(idx, val)
        +test(idx) bool
        +operator[](idx) bool
        +count() unsigned
        +find_first() int
        +operator|=(BitVector)
        +operator~() BitVector
    }
    
    class DirectedGraph~NodeTy~ {
        -nodes_ : SmallVector~GraphNode~
        +addNode(data) unsigned
        +addEdge(from, to)
        +successors(id) SmallVector
        +predecessors(id) SmallVector
        +postOrder(entry) SmallVector
        +reversePostOrder(entry) SmallVector
    }
    
    class BumpPtrAllocator {
        -currentSlab_ : Slab*
        -currentPtr_ : uint8_t*
        -currentEnd_ : uint8_t*
        +allocate(size, align) void*
        +create~T~(args) T*
        +reset()
        +totalSize() size_t
    }
```

### 1.2 AIR 模块

```mermaid
classDiagram
    class Type {
        -kind_ : TypeKind
        -sizeInBits_ : unsigned
        -alignInBits_ : unsigned
        +getInt32Ty() Type*
        +getPointerTy(elem) Type*
        +getKind() TypeKind
        +toString() string
    }
    
    class Constant {
        -type_ : Type*
    }
    
    class ConstantInt {
        -value_ : uint64_t
        +getInt32(val) ConstantInt*
        +getSExtValue() int64_t
        +isZero() bool
    }
    
    class AIRInstruction {
        -opcode_ : AIROpcode
        -type_ : Type*
        -destVReg_ : unsigned
        -operands_ : SmallVector
        -parent_ : BasicBlock*
        +getOpcode() AIROpcode
        +hasResult() bool
        +getOperand(i) unsigned
        +replaceUse(old, new)
        +toString() string
    }
    
    class BasicBlock {
        -first_ : AIRInstruction*
        -last_ : AIRInstruction*
        -successors_ : SmallVector
        -predecessors_ : SmallVector
        +pushBack(inst)
        +getTerminator() AIRInstruction*
        +addSuccessor(bb)
    }
    
    class Function {
        -fnType_ : FunctionType*
        -blocks_ : vector~unique_ptr~BasicBlock~~
        -nextVReg_ : unsigned
        +createBasicBlock(name) BasicBlock*
        +nextVReg() unsigned
        +getEntryBlock() BasicBlock*
    }
    
    class Module {
        -functions_ : vector~unique_ptr~Function~~
        -globals_ : vector~unique_ptr~GlobalVariable~~
        +createFunction(ty, name) Function*
        +getDataLayout() DataLayout
    }
    
    class AIRBuilder {
        -insertBlock_ : BasicBlock*
        -insertPoint_ : AIRInstruction*
        +createAdd(ty, lhs, rhs) unsigned
        +createLoad(ty, ptr) unsigned
        +createRet(val)
        +createCondBr(cond, trueBB, falseBB)
        +createPhi(ty, incomings) unsigned
    }
    
    Constant <|-- ConstantInt
    Constant <|-- Function
    Function --> BasicBlock
    BasicBlock --> AIRInstruction
    Module --> Function
    AIRBuilder --> AIRInstruction
    AIRBuilder --> BasicBlock
```

### 1.3 Target 模块

```mermaid
classDiagram
    class TargetMachine {
        +getRegisterInfo() TargetRegisterInfo&
        +getInstrInfo() TargetInstrInfo&
        +getLowering() TargetLowering&
        +getCallingConv() TargetCallingConv&
        +createAsmPrinter(stream) AsmPrinter*
        +createX86_64() unique_ptr~TargetMachine~
    }
    
    class TargetRegisterInfo {
        +getRegClass(id) RegisterClass&
        +getFramePointer() Register
        +getStackPointer() Register
        +getCalleeSavedRegs() BitVector
    }
    
    class RegisterClass {
        -regs_ : vector~Register~
        +getNumRegs() unsigned
        +contains(reg) bool
    }
    
    class X86RegisterInfo {
        +RAX = 0, XMM0 = 16
        +get32Reg(reg64) unsigned
    }
    
    class X86InstrInfo {
        +get(opcode) MachineOpcodeDesc&
        +getMoveOpcode(size) unsigned
        +getArithOpcode(op, size, isImm) unsigned
    }
    
    class X86CallingConv {
        +analyzeArguments(args) SmallVector
        +analyzeReturn(retTy) SmallVector
        +getStackAlignment() unsigned
    }
    
    class X86TargetLowering {
        +getOperationAction(op, vtSize) LegalizeAction
        +isTypeLegal(typeSize) bool
    }
    
    class X86FrameLowering {
        +emitPrologue(mf, entry)
        +emitEpilogue(mf, ret)
        +getStackAlignment() unsigned
    }
    
    class X86ISelPatterns {
        +matchPattern(airOp, resultTy, types, op0, op1) ISelMatchResult
        +getAllPatterns() vector~ISelPattern~
    }
    
    TargetMachine <|-- X86TargetMachine
    TargetRegisterInfo <|-- X86RegisterInfo
    TargetInstrInfo <|-- X86InstrInfo
    TargetLowering <|-- X86TargetLowering
    TargetFrameLowering <|-- X86FrameLowering
    X86TargetMachine --> X86RegisterInfo
    X86TargetMachine --> X86InstrInfo
    X86TargetMachine --> X86TargetLowering
    X86TargetMachine --> X86CallingConv
    X86TargetMachine --> X86FrameLowering
```

### 1.4 CodeGen 模块

```mermaid
classDiagram
    class MachineInstr {
        -opcode_ : uint16_t
        -operands_ : SmallVector~MachineOperand~
        -parent_ : MachineBasicBlock*
        +getOpcode() uint16_t
        +getOperand(i) MachineOperand&
        +addOperand(mo)
    }
    
    class MachineBasicBlock {
        -first_ : MachineInstr*
        -last_ : MachineInstr*
        -successors_ : SmallVector
        +pushBack(mi)
        +addSuccessor(mbb)
    }
    
    class MachineFunction {
        -airFunc_ : Function&
        -blocks_ : SmallVector~unique_ptr~MBB~
        +createBasicBlock(name) MBB*
        +createVirtualRegister(ty) unsigned
        +createStackSlot(size, align) int
    }
    
    class SelectionDAG {
        -allNodes_ : vector~SDNode*~
        -roots_ : SmallVector~SDValue~
        +createNode(op, ty, ops) SDValue
        +buildFromBasicBlock(airBB, mbb)
        +select(mbb)
    }
    
    class SDNode {
        -opcode_ : AIROpcode
        -operands_ : SmallVector~SDNode*~
        -users_ : SmallVector~SDNode*~
        +getOperand(i) SDValue
        +isSelected() bool
        +createMachineInstr(opcode)
    }
    
    class InstructionSelector {
        -target_ : TargetMachine&
        +run(dag, mbb)
    }
    
    class LinearScanRegAlloc {
        -intervals_ : vector~LiveInterval~
        +allocateRegisters()
        +computeLiveIntervals()
        +linearScan()
    }
    
    class LiveInterval {
        -vreg_ : unsigned
        -ranges_ : vector~LiveRange~
        -assignedReg_ : unsigned
        -spillSlot_ : int
        +overlaps(other) bool
        +liveAt(slot) bool
    }
    
    class PassManager {
        -passes_ : vector~unique_ptr~CodeGenPass~~
        +addPass(pass)
        +run(mf)
    }
    
    class CodeGenContext {
        +run()
        +addStandardPasses(pm, tm)
    }
    
    MachineFunction --> MachineBasicBlock
    MachineBasicBlock --> MachineInstr
    SelectionDAG --> SDNode
    InstructionSelector --> SelectionDAG
    LinearScanRegAlloc --> MachineFunction
    LinearScanRegAlloc --> LiveInterval
    CodeGenContext --> PassManager
```

### 1.5 MC 模块

```mermaid
classDiagram
    class MCStreamer {
        +emitInstruction(mi)
        +emitLabel(name)
        +emitComment(text)
        +emitGlobalSymbol(name)
        +emitAlignment(bytes)
    }
    
    class AsmTextStreamer {
        -os_ : ostream&
        +emitInstruction(mi)
        +emitLabel(name)
    }
    
    class AsmPrinter {
        -streamer_ : MCStreamer&
        +emitFunction(mf)
        +emitBasicBlock(mbb)
        +emitInstruction(mi)
    }
    
    class X86AsmPrinter {
        -regInfo_ : X86RegisterInfo&
        +emitInstruction(mi)
        +printOperand(mo, os)
    }
    
    class X86InstEncoder {
        +encode(mi, out)
        -emitModRM(mod, reg, rm, out)
        -emitREX(w, r, x, b, out)
        -emitSIB(scale, index, base, out)
    }
    
    MCStreamer <|-- AsmTextStreamer
    AsmPrinter <|-- X86AsmPrinter
    AsmPrinter --> MCStreamer
    X86AsmPrinter --> X86RegisterInfo
```

---

## 2. 时序图

### 2.1 完整编译流程

```mermaid
sequenceDiagram
    participant User as 用户代码
    participant AIR as AIR Builder
    participant TM as TargetMachine
    participant CG as CodeGenContext
    participant PM as PassManager
    participant RA as RegisterAllocator
    participant MC as AsmPrinter

    User->>AIR: createAdd(ty, lhs, rhs)
    AIR->>AIR: allocateVReg(ty)
    AIR->>AIR: new AIRInstruction(Add, ty)
    AIR->>AIR: setDestVReg(vreg)
    AIR-->>User: return vreg

    User->>TM: createX86_64()
    TM-->>User: X86TargetMachine

    User->>CG: run()
    CG->>PM: addStandardPasses()
    
    loop 每个 Function
        CG->>PM: run(MachineFunction)
        PM->>PM: AIRToMachineIRPass.run()
        Note over PM: AIR inst → MachineInstr
        PM->>PM: PeepholePass.run()
        PM->>PM: PhiEliminationPass.run()
        PM->>PM: RegisterCoalescerPass.run()
        PM->>RA: allocateRegisters()
        RA->>RA: computeLiveIntervals()
        RA->>RA: linearScan()
        Note over RA: 虚拟寄存器 → 物理寄存器
        PM->>PM: PrologueEpilogueInserter.run()
        PM->>PM: BranchFoldingPass.run()
    end
    
    User->>MC: emitFunction(mf)
    MC->>MC: emitFunctionHeader()
    loop 每个 BasicBlock
        MC->>MC: emitBasicBlock()
        loop 每条 MachineInstr
            MC->>MC: emitInstruction()
            Note over MC: AT&T 语法输出
        end
    end
    MC->>MC: emitFunctionFooter()
```

### 2.2 指令选择流程

```mermaid
sequenceDiagram
    participant AIR as AIRBasicBlock
    participant DAG as SelectionDAG
    participant ISEL as InstructionSelector
    participant PAT as X86ISelPatterns
    participant MBB as MachineBasicBlock

    ISEL->>DAG: buildFromBasicBlock(airBB)
    loop 每条 AIR 指令
        DAG->>DAG: createNode(op, ty, ops)
        DAG->>DAG: SDNode 构造 + use-def 链
    end

    ISEL->>DAG: legalize()
    Note over DAG: 类型合法化 (i8→i32, etc.)

    ISEL->>DAG: select()
    loop 每个 SDNode
        DAG->>PAT: matchPattern(airOp, ty, ...)
        PAT-->>DAG: ISelMatchResult{x86Opcode, matched}
        DAG->>DAG: node.createMachineInstr(x86Opcode)
    end

    ISEL->>DAG: schedule()
    Note over DAG: DAG → 线性指令序列
    DAG->>MBB: pushBack(MachineInstr)
```

### 2.3 寄存器分配流程

```mermaid
sequenceDiagram
    participant MF as MachineFunction
    participant LA as LinearScanRegAlloc
    participant LI as LiveInterval
    participant PR as PhysicalRegister

    LA->>MF: computeLiveIntervals()
    loop 每个 MBB + 每条 MI
        Note over LA: def-use 数据流分析
        LA->>LI: addRange(def, lastUse)
    end

    LA->>LA: 排序 intervals (按 start)

    loop 每个 LiveInterval (按 start 序)
        LA->>LA: expireOldIntervals(current)
        Note over LA: 移除已结束的 interval
        
        alt 有空闲寄存器
            LA->>PR: assignPhysReg(li, reg)
        else 无空闲寄存器
            LA->>LA: selectRegToSpill(current)
            LA->>MF: createStackSlot(8, 8)
            LA->>LI: setSpillSlot(slot)
        end
    end

    Note over LA: 改写: 虚拟寄存器 → 物理寄存器 + spill code
```

### 2.4 x86-64 栈帧布局

```mermaid
sequenceDiagram
    participant FE as PrologueEpilogueInserter
    participant MBB as MachineBasicBlock
    participant FL as X86FrameLowering

    FE->>FE: calculateFrameLayout(mf)
    Note over FE: 计算局部变量/溢出槽/对齐

    FE->>FL: emitPrologue(entry)
    Note over MBB: push %rbp
    Note over MBB: mov %rsp, %rbp
    Note over MBB: sub $N, %rsp  (N = 帧大小, 16-byte对齐)

    Note over MBB: ... 函数体 ...

    FE->>FL: emitEpilogue(exit)
    Note over MBB: mov %rbp, %rsp
    Note over MBB: pop %rbp
    Note over MBB: ret
```

### 2.5 System V AMD64 调用约定

```mermaid
sequenceDiagram
    participant Caller as 调用者
    participant Stack as 栈
    participant Callee as 被调用者

    Note over Caller: 准备参数
    Caller->>Callee: RDI ← arg0
    Caller->>Callee: RSI ← arg1
    Caller->>Callee: RDX ← arg2
    Caller->>Callee: RCX ← arg3
    Caller->>Callee: R8  ← arg4
    Caller->>Callee: R9  ← arg5
    Note over Stack: 第7个参数开始入栈
    Caller->>Callee: call instruction (push RIP)

    Note over Callee: 函数入口
    Callee->>Stack: push %rbp
    Callee->>Stack: sub $N, %rsp
    
    Note over Callee: 函数体 (可使用 RAX,RCX,RDX,RSI,RDI,R8-R11)
    
    Note over Callee: save callee-saved: RBX,RBP,R12-R15
    Note over Callee: XMM0-XMM7 用于浮点参数
    
    Callee->>Caller: RAX ← 返回值
    Callee->>Caller: ret (pop RIP)
```

---

## 3. 完整管线图

```mermaid
flowchart TD
    A[自定义 SSA IR] --> B[IR Bridge]
    B --> C[AIR Module]
    C --> D[AIR Builder]
    
    D --> E[PassManager]
    E --> F[AIR→MIr Pass]
    F --> G[MachineFunction]
    G --> H[SelectionDAG]
    H --> I[DAGCombine]
    I --> J[LegalizeTypes]
    J --> K[LegalizeOps]
    K --> L[ISel PatternMatch]
    L --> M[Schedule DAG]
    M --> N[Phi Elimination]
    N --> O[Two-Address]
    O --> P[Register Coalescing]
    P --> Q[Register Allocation]
    Q --> R[Prologue/Epilogue]
    R --> S[Peephole Optimizer]
    S --> T[Branch Folding]
    
    T --> U[MC Emission]
    U --> V{X86AsmPrinter}
    V --> W[AT&T .s file]
    U --> X{X86InstEncoder}
    X --> Y[ELF .o file]
    
    style A fill:#f9f
    style C fill:#9cf
    style Q fill:#fc9
    style V fill:#9f9
    style X fill:#9f9
```
