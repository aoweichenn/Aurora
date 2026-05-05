# API 参考

本文档按模块列出当前公开头文件中的主要接口。更完整的细节以 `include/Aurora/**` 下的头文件为准。

## ADT

- `SmallVector<T, N>`：小对象优化容器，提供 `push_back`、`pop_back`、`reserve`、`resize`、`erase`、`clear`、迭代器接口。
- `BitVector`：`set`、`reset`、`test`、`resize`、`count`、`any`、`none`、`all`、`find_first`、`find_next`、位运算。
- `DirectedGraph<NodeTy>`：`addNode`、`addEdge`、`successors`、`predecessors`、`reversePostOrder`、`postOrder`。
- `SparseSet<T>`：`setUniverse`、`insert`、`erase`、`contains`、`clear`、`begin`、`end`。
- `BumpPtrAllocator`：`allocate`、`create`、`reset`、`totalSize`。

## AIR

- `Type`：基础类型与派生类型工厂，例如 `getInt32Ty()`、`getPointerTy()`、`getFunctionTy()`，以及 `getSizeInBits()`、`toString()`。
- `Constant` / `ConstantInt` / `ConstantFP` / `GlobalVariable`：常量、浮点常量和全局变量包装。
- `FunctionType`：`getReturnType()`、`getParamTypes()`、`getNumParams()`、`isVarArg()`。
- `Function`：`createBasicBlock()`、`getBlocks()`、`getEntryBlock()`、`nextVReg()`、`recordVRegType()`。
- `BasicBlock`：`pushBack()`、`pushFront()`、`insertBefore()`、`insertAfter()`、`erase()`、`addSuccessor()`、`addPredecessor()`。
- `AIRInstruction`：所有 `create...` 工厂方法，包括算术、比较、内存、转换、控制流、调用、常量、全局地址、`switch`、`extractvalue`、`insertvalue`。
- `AIRBuilder`：与 `AIRInstruction` 对应的高层包装，另外提供 `createRet()`、`createRetVoid()`、`createBr()`、`createCondBr()`、`createUnreachable()`。
- `Module`：`createFunction()`、`createGlobal()`、`getFunctions()`、`getGlobals()`、`getDataLayout()`。
- `DataLayout`：`setLittleEndian()`、`setPointerSize()`、`isLittleEndian()`、`getPointerSize()`。

## Target

- `TargetMachine`：`getRegisterInfo()`、`getInstrInfo()`、`getLowering()`、`getCallingConv()`、`getFrameLowering()`、`createAsmPrinter()`、`getDataLayout()`、`getTargetTriple()`。
- `TargetRegisterInfo`：寄存器类、调用保存/调用者保存寄存器、分配顺序等接口。
- `Register` / `RegisterClass`：寄存器描述与寄存器类描述。
- `TargetInstrInfo`：`get()`、`getNumOpcodes()`、`isMoveImmediate()`、`copyPhysReg()`、`storeRegToStackSlot()`、`loadRegFromStackSlot()`。
- `TargetLowering`：`getOperationAction()`、`isTypeLegal()`、`getRegisterSizeForType()`。
- `TargetCallingConv`：`analyzeArguments()`、`analyzeReturn()`、`getStackAlignment()`、`getShadowStoreSize()`。
- `TargetFrameLowering`：`emitPrologue()`、`emitEpilogue()`、`getFrameIndexReference()`、`hasFP()`、`getStackAlignment()`。

## Target/X86

- `X86TargetMachine`：x86-64 目标机器实现。
- `X86RegisterInfo`：x86 寄存器描述、`getReg()`、`get32Reg()`、寄存器类与分配顺序。
- `X86InstrInfo`：x86 指令表与 opcode 查询。
- `X86CallingConv`：x86-64 SysV 风格调用约定分析。
- `X86FrameLowering`：函数序言/尾声与栈帧引用计算。
- `X86TargetLowering`：目标合法化和类型注册尺寸规则。
- `X86InstrEncode`：面向编码表的机器码编码入口。

## Target/AArch64

- `AArch64TargetMachine`：macOS arm64 目标机器实现。
- `AArch64RegisterInfo`：AArch64 寄存器描述、寄存器类与分配顺序。
- `AArch64InstrInfo`：AArch64 指令表与 opcode 查询。
- `AArch64CallingConv`：AAPCS 风格的寄存器和栈参数分配。
- `AArch64FrameLowering`：函数序言/尾声与栈帧引用计算。
- `AArch64TargetLowering`：目标合法化和类型寄存器尺寸规则。

## CodeGen

- `MachineOperand`：`createReg()`、`createVReg()`、`createImm()`、`createMBB()`、`createFrameIndex()`、`createGlobalSym()`，以及对应访问器。
- `MachineInstr`：操作码、操作数、标志、前后链接、父块访问。
- `MachineBasicBlock`：指令链、前驱后继、`pushBack()`、`insertBefore()`、`insertAfter()`、`remove()`、`getLiveIns()`、`getLiveOuts()`。
- `MachineFunction`：`createBasicBlock()`、`createVirtualRegister()`、`getVRegType()`、`createStackSlot()`、`getStackObjects()`、`getNumVRegs()`。
- `LiveInterval`：`addRange()`、`overlaps()`、`liveAt()`、`setAssignedReg()`、`setSpillSlot()`、`setSpillWeight()`。
- `SelectionDAG` / `SDNode` / `SDValue`：`createNode()`、`createConstant()`、`createRegister()`、`buildFromBasicBlock()`、`dagCombine()`、`legalize()`、`select()`、`schedule()`。
- `PassManager`：`addPass()`、`setInstrumentation()`、`getPassCount()`、`run()`。
- `CodeGenContext`：`run()` 和 `addStandardPasses()`。
- `CodeGenPass` / `PassInstrumentation`：pass 扩展接口。
- `ISelContext`：记录常量、栈槽、store 信息。
- `LoweringStrategy` / `LoweringRegistry`：低层策略注册与执行。
- `X86OpcodeMapper`：将 AIR 条件码与二进制 opcode 对应。
- `X86LoweringStrategies`：当前 x86 常量 lowering 策略。
- `LinearScanRegAlloc`：`allocateRegisters()`。
- `PrologueEpilogueInserter`：`run()`。

## MC

- `MCStreamer`：汇编文本输出抽象。
- `AsmPrinter`：遍历 `MachineFunction` 并打印汇编。
- `X86AsmPrinter`：x86-64 AT&T 语法打印。
- `AArch64AsmPrinter`：macOS arm64 汇编打印。
- `ObjectWriter`：`addFunction()`、`addGlobal()`、`addExternSymbol()`、`write()`。
- `X86ObjectEncoder`：机器指令到字节流编码。
- `X86InstEncoder`：基于编码表的指令编码。

## tools/minic

- `TokenKind` / `Token` / `Lexer`：词法分析接口。
- `Parser`：`parseProgram()`，以及函数定义 / 原型、顶层标量和定长数组全局变量、命名 `struct` / `union` 定义、designated initializer、表达式、比较、加减乘除、`sizeof`、`alignof` / `_Alignof`、`if`、`call`、字面量、变量解析。
- AST 结构：表达式、语句、函数和声明节点，包括 `SizeofExpr`、`AlignofExpr`、`MemberExpr`、`Function`、`GlobalDecl`。
- `CodeGen`：`generate()` 将 AST 转成 `aurora::Module`。

## 示例工具

- `tools/aurorac`：读取样例 `Module` 并通过所选后端打印汇编。
- `tools/aurora-obj`：生成 x86-64 ELF relocatable 对象文件。
