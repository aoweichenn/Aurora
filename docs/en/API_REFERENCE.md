# API Reference

This document lists the main public interfaces exposed by the current headers. The headers under `include/Aurora/**` remain the source of truth.

## ADT

- `SmallVector<T, N>`: small-buffer-optimized container with `push_back`, `pop_back`, `reserve`, `resize`, `erase`, `clear`, and iterator support.
- `BitVector`: `set`, `reset`, `test`, `resize`, `count`, `any`, `none`, `all`, `find_first`, `find_next`, and bitwise operators.
- `DirectedGraph<NodeTy>`: `addNode`, `addEdge`, `successors`, `predecessors`, `reversePostOrder`, and `postOrder`.
- `SparseSet<T>`: `setUniverse`, `insert`, `erase`, `contains`, `clear`, `begin`, `end`.
- `BumpPtrAllocator`: `allocate`, `create`, `reset`, `totalSize`.

## AIR

- `Type`: primitive and derived type factories such as `getInt32Ty()`, `getPointerTy()`, `getFunctionTy()`, plus `getSizeInBits()` and `toString()`.
- `Constant` / `ConstantInt` / `ConstantFP` / `GlobalVariable`: constant and global-variable wrappers.
- `FunctionType`: `getReturnType()`, `getParamTypes()`, `getNumParams()`, `isVarArg()`.
- `Function`: `createBasicBlock()`, `getBlocks()`, `getEntryBlock()`, `nextVReg()`, `recordVRegType()`.
- `BasicBlock`: `pushBack()`, `pushFront()`, `insertBefore()`, `insertAfter()`, `erase()`, `addSuccessor()`, `addPredecessor()`.
- `AIRInstruction`: the full family of `create...` factories for arithmetic, comparison, memory, conversion, control flow, calls, constants, global addresses, `switch`, `extractvalue`, and `insertvalue`.
- `AIRBuilder`: high-level wrappers over `AIRInstruction`, plus `createRet()`, `createRetVoid()`, `createBr()`, `createCondBr()`, and `createUnreachable()`.
- `Module`: `createFunction()`, `createGlobal()`, `getFunctions()`, `getGlobals()`, `getDataLayout()`.
- `DataLayout`: `setLittleEndian()`, `setPointerSize()`, `isLittleEndian()`, `getPointerSize()`.

## Target

- `TargetMachine`: `getRegisterInfo()`, `getInstrInfo()`, `getLowering()`, `getCallingConv()`, `getFrameLowering()`, `createAsmPrinter()`, `getDataLayout()`, `getTargetTriple()`.
- `TargetRegisterInfo`: register classes, callee-saved/caller-saved sets, allocation order, and frame/stack pointers.
- `Register` / `RegisterClass`: register descriptors and register-class descriptors.
- `TargetInstrInfo`: `get()`, `getNumOpcodes()`, `isMoveImmediate()`, `copyPhysReg()`, `storeRegToStackSlot()`, `loadRegFromStackSlot()`.
- `TargetLowering`: `getOperationAction()`, `isTypeLegal()`, `getRegisterSizeForType()`.
- `TargetCallingConv`: `analyzeArguments()`, `analyzeReturn()`, `getStackAlignment()`, `getShadowStoreSize()`.
- `TargetFrameLowering`: `emitPrologue()`, `emitEpilogue()`, `getFrameIndexReference()`, `hasFP()`, `getStackAlignment()`.

## Target/X86

- `X86TargetMachine`: x86-64 target-machine implementation.
- `X86RegisterInfo`: register description, `getReg()`, `get32Reg()`, register classes, and allocation order.
- `X86InstrInfo`: x86 instruction table and opcode queries.
- `X86CallingConv`: x86-64 SysV-style calling-convention analysis.
- `X86FrameLowering`: prologue/epilogue emission and frame-index references.
- `X86TargetLowering`: type legalization and register-size rules.
- `X86InstrEncode`: machine-code encoding entry points driven by the encoding table.

## Target/AArch64

- `AArch64TargetMachine`: macOS arm64 target-machine implementation.
- `AArch64RegisterInfo`: AArch64 register description, register classes, and allocation order.
- `AArch64InstrInfo`: AArch64 instruction table and opcode queries.
- `AArch64CallingConv`: AAPCS-style register and stack argument assignment.
- `AArch64FrameLowering`: prologue/epilogue emission and frame-index references.
- `AArch64TargetLowering`: type legalization and register-size rules.

## CodeGen

- `MachineOperand`: `createReg()`, `createVReg()`, `createImm()`, `createMBB()`, `createFrameIndex()`, `createGlobalSym()`, and the associated accessors.
- `MachineInstr`: opcode, operands, flags, parent/next/prev links, and parent-block access.
- `MachineBasicBlock`: instruction list, CFG edges, `pushBack()`, `insertBefore()`, `insertAfter()`, `remove()`, `getLiveIns()`, and `getLiveOuts()`.
- `MachineFunction`: `createBasicBlock()`, `createVirtualRegister()`, `getVRegType()`, `createStackSlot()`, `getStackObjects()`, `getNumVRegs()`.
- `LiveInterval`: `addRange()`, `overlaps()`, `liveAt()`, `setAssignedReg()`, `setSpillSlot()`, `setSpillWeight()`.
- `SelectionDAG` / `SDNode` / `SDValue`: `createNode()`, `createConstant()`, `createRegister()`, `buildFromBasicBlock()`, `dagCombine()`, `legalize()`, `select()`, `schedule()`.
- `PassManager`: `addPass()`, `setInstrumentation()`, `getPassCount()`, `run()`.
- `CodeGenContext`: `run()` and `addStandardPasses()`.
- `CodeGenPass` / `PassInstrumentation`: pass-extension interfaces.
- `ISelContext`: constant, stack-slot, and store tracking.
- `LoweringStrategy` / `LoweringRegistry`: lowering strategy registration and execution.
- `X86OpcodeMapper`: mapping AIR conditions to x86 opcodes.
- `X86LoweringStrategies`: the current x86 constant-lowering strategy.
- `LinearScanRegAlloc`: `allocateRegisters()`.
- `PrologueEpilogueInserter`: `run()`.

## MC

- `MCStreamer`: assembly-text emission abstraction.
- `AsmPrinter`: walks a `MachineFunction` and prints assembly.
- `X86AsmPrinter`: x86-64 AT&T syntax printer.
- `AArch64AsmPrinter`: macOS arm64 assembly printer.
- `ObjectWriter`: `addFunction()`, `addGlobal()`, `addExternSymbol()`, `write()`.
- `X86ObjectEncoder`: machine-instruction to byte-stream encoding.
- `X86InstEncoder`: table-driven instruction encoder.

## tools/minic

- `TokenKind` / `Token` / `Lexer`: lexical-analysis interfaces.
- `Parser`: `parseProgram()`, plus parsing for function definitions/prototypes, expressions, comparisons, arithmetic, `if`, calls, literals, and variables.
- AST nodes: `IntLitExpr`, `VarExpr`, `BinaryExpr`, `NegExpr`, `IfExpr`, `Function`.
- `CodeGen`: `generate()` turns AST into `aurora::Module`.

## Sample Tools

- `tools/aurorac`: reads a sample `Module` and prints assembly through the selected backend.
- `tools/aurora-obj`: produces an x86-64 ELF relocatable object.
