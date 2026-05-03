# API 参考文档

## AIR 层 API

### Type

| 方法 | 返回 | 说明 |
|------|------|------|
| `Type::getVoidTy()` | `Type*` | void 类型 |
| `Type::getInt1Ty()` | `Type*` | 1-bit 整数 |
| `Type::getInt8Ty()` | `Type*` | 8-bit 整数 |
| `Type::getInt16Ty()` | `Type*` | 16-bit 整数 |
| `Type::getInt32Ty()` | `Type*` | 32-bit 整数 |
| `Type::getInt64Ty()` | `Type*` | 64-bit 整数 |
| `Type::getFloatTy()` | `Type*` | 32-bit 浮点 |
| `Type::getDoubleTy()` | `Type*` | 64-bit 浮点 |
| `Type::getPointerTy(Type* elem)` | `Type*` | 指针类型 |
| `Type::getArrayTy(Type* elem, unsigned n)` | `Type*` | 数组类型 |
| `Type::getStructTy(SmallVector<Type*,8> members)` | `Type*` | 结构体类型 |
| `Type::getFunctionTy(Type* ret, SmallVector<Type*,8> params)` | `Type*` | 函数类型 |
| `getKind()` | `TypeKind` | 类型种类 |
| `getSizeInBits()` | `unsigned` | 位大小 |
| `isInteger()` | `bool` | 是否整数 |
| `isPointer()` | `bool` | 是否指针 |
| `toString()` | `std::string` | 类型字符串 |

### Constant

| 方法 | 返回 | 说明 |
|------|------|------|
| `ConstantInt::getInt32(int32_t)` | `ConstantInt*` | 创建 i32 常量 |
| `ConstantInt::getInt64(int64_t)` | `ConstantInt*` | 创建 i64 常量 |
| `getSExtValue()` | `int64_t` | 有符号扩展值 |
| `getZExtValue()` | `uint64_t` | 零扩展值 |
| `isZero()` | `bool` | 是否为零 |
| `isOne()` | `bool` | 是否为一 |

### AIRInstruction

| 工厂方法 | 返回 | 说明 |
|----------|------|------|
| `createRet(unsigned val)` | `AIRInstruction*` | 返回指令 |
| `createBr(BasicBlock* target)` | `AIRInstruction*` | 无条件跳转 |
| `createCondBr(unsigned cond, BasicBlock* t, BasicBlock* f)` | `AIRInstruction*` | 条件跳转 |
| `createAdd(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 整数加法 |
| `createSub(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 整数减法 |
| `createMul(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 整数乘法 |
| `createUDiv(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 无符号除法 |
| `createSDiv(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 有符号除法 |
| `createAnd(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 按位与 |
| `createOr(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 按位或 |
| `createXor(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 按位异或 |
| `createShl(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 左移 |
| `createLShr(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 逻辑右移 |
| `createAShr(Type* ty, unsigned l, unsigned r)` | `AIRInstruction*` | 算术右移 |
| `createICmp(Type* ty, ICmpCond, unsigned l, unsigned r)` | `AIRInstruction*` | 整数比较 |
| `createAlloca(Type* allocaTy)` | `AIRInstruction*` | 栈分配 |
| `createLoad(Type* ty, unsigned ptr)` | `AIRInstruction*` | 内存加载 |
| `createStore(unsigned val, unsigned ptr)` | `AIRInstruction*` | 内存存储 |
| `createGEP(Type* ty, unsigned ptr, indices)` | `AIRInstruction*` | 地址计算 |
| `createSExt(Type* dstTy, unsigned src)` | `AIRInstruction*` | 有符号扩展 |
| `createZExt(Type* dstTy, unsigned src)` | `AIRInstruction*` | 零扩展 |
| `createTrunc(Type* dstTy, unsigned src)` | `AIRInstruction*` | 截断 |
| `createPhi(Type* ty, incomings)` | `AIRInstruction*` | φ 节点 |
| `createSelect(Type* ty, unsigned c, unsigned t, unsigned f)` | `AIRInstruction*` | 条件选择 |
| `createCall(Type* retTy, Function* callee, args)` | `AIRInstruction*` | 函数调用 |

| 查询方法 | 返回 | 说明 |
|----------|------|------|
| `getOpcode()` | `AIROpcode` | 操作码 |
| `getType()` | `Type*` | 结果类型 |
| `hasResult()` | `bool` | 是否产生值 |
| `getNumOperands()` | `unsigned` | 操作数数量 |
| `getOperand(unsigned i)` | `unsigned` | 第 i 个操作数 (vreg) |
| `getDestVReg()` | `unsigned` | 目标虚拟寄存器 |
| `getParent()` | `BasicBlock*` | 所属基本块 |

### AIRBuilder

| 方法 | 返回 | 说明 |
|------|------|------|
| `setInsertPoint(BasicBlock* bb, AIRInstruction* before)` | `void` | 设置插入点 |
| `createAdd(ty, l, r)` | `unsigned` | 创建 add → vreg |
| `createSub(ty, l, r)` | `unsigned` | 创建 sub → vreg |
| `createMul(ty, l, r)` | `unsigned` | 创建 mul → vreg |
| `createSDiv(ty, l, r)` | `unsigned` | 创建 sdiv → vreg |
| `createUDiv(ty, l, r)` | `unsigned` | 创建 udiv → vreg |
| `createAnd(ty, l, r)` | `unsigned` | 创建 and → vreg |
| `createOr(ty, l, r)` | `unsigned` | 创建 or → vreg |
| `createXor(ty, l, r)` | `unsigned` | 创建 xor → vreg |
| `createShl(ty, l, r)` | `unsigned` | 创建 shl → vreg |
| `createLShr(ty, l, r)` | `unsigned` | 创建 lshr → vreg |
| `createAShr(ty, l, r)` | `unsigned` | 创建 ashr → vreg |
| `createICmp(cond, l, r)` | `unsigned` | 创建 icmp → vreg |
| `createAlloca(ty)` | `unsigned` | 创建 alloca → vreg |
| `createLoad(ty, ptr)` | `unsigned` | 创建 load → vreg |
| `createGEP(ty, ptr, indices)` | `unsigned` | 创建 gep → vreg |
| `createSExt(dstTy, src)` | `unsigned` | 创建 sext → vreg |
| `createZExt(dstTy, src)` | `unsigned` | 创建 zext → vreg |
| `createTrunc(dstTy, src)` | `unsigned` | 创建 trunc → vreg |
| `createSelect(ty, c, t, f)` | `unsigned` | 创建 select → vreg |
| `createCall(callee, args)` | `unsigned` | 创建 call → vreg |
| `createPhi(ty, incomings)` | `unsigned` | 创建 phi → vreg |
| `createRet(val)` | `void` | 创建返回指令 |
| `createRetVoid()` | `void` | 创建 void 返回 |
| `createBr(target)` | `void` | 创建无条件跳转 |
| `createCondBr(cond, t, f)` | `void` | 创建条件跳转 |

### Module & Function

| 方法 | 返回 | 说明 |
|------|------|------|
| `Module::createFunction(ty, name)` | `Function*` | 创建函数 |
| `Module::createGlobal(ty, name)` | `GlobalVariable*` | 创建全局变量 |
| `Function::createBasicBlock(name)` | `BasicBlock*` | 创建基本块 |
| `Function::getEntryBlock()` | `BasicBlock*` | 入口块 |
| `Function::nextVReg()` | `unsigned` | 分配新虚拟寄存器 |

### ICmpCond 枚举

| 值 | 说明 |
|----|------|
| `EQ` | 等于 |
| `NE` | 不等于 |
| `UGT` | 无符号大于 |
| `UGE` | 无符号大于等于 |
| `ULT` | 无符号小于 |
| `ULE` | 无符号小于等于 |
| `SGT` | 有符号大于 |
| `SGE` | 有符号大于等于 |
| `SLT` | 有符号小于 |
| `SLE` | 有符号小于等于 |

---

## Target 层 API

### TargetMachine

| 方法 | 返回 | 说明 |
|------|------|------|
| `TargetMachine::createX86_64()` | `unique_ptr<TargetMachine>` | 工厂 |
| `getRegisterInfo()` | `const TargetRegisterInfo&` | 寄存器信息 |
| `getInstrInfo()` | `const TargetInstrInfo&` | 指令信息 |
| `getLowering()` | `const TargetLowering&` | 合法化 |
| `getCallingConv()` | `const TargetCallingConv&` | 调用约定 |
| `getTargetTriple()` | `const char*` | 目标三元组 |

### X86RegisterInfo (寄存器 ID)

| 常量 | 值 | 说明 |
|------|-----|------|
| `RAX/RCX/RDX/RBX` | 0-3 | 通用寄存器 |
| `RSP/RBP/RSI/RDI` | 4-7 | 栈/帧/源/目标 |
| `R8-R15` | 8-15 | 扩展寄存器 |
| `XMM0-XMM15` | 16-31 | SSE 寄存器 |

### X86InstrInfo (指令 opcode 示例)

| Opcode | 汇编 | 说明 |
|--------|------|------|
| `MOV64rr` | `movq $src, $dst` | 64位寄存器移动 |
| `ADD64rr` | `addq $src, $dst` | 64位加法 |
| `SUB64rr` | `subq $src, $dst` | 64位减法 |
| `IMUL64rr` | `imulq $src, $dst` | 64位乘法 |
| `AND64rr` | `andq $src, $dst` | 按位与 |
| `OR64rr` | `orq $src, $dst` | 按位或 |
| `XOR64rr` | `xorq $src, $dst` | 按位异或 |
| `CMP64rr` | `cmpq $src, $dst` | 比较 |
| `CALL64pcrel32` | `call $dst` | 函数调用 |
| `RETQ` | `retq` | 返回 |
| `PUSH64r` | `pushq $src` | 入栈 |
| `POP64r` | `popq $dst` | 出栈 |
| `JE_1/JE_4` | `je $dst` | 相等跳转 |
| `JMP_1/JMP_4` | `jmp $dst` | 无条件跳转 |

---

## CodeGen 层 API

### PassManager

| 方法 | 返回 | 说明 |
|------|------|------|
| `addPass(unique_ptr<CodeGenPass>)` | `void` | 添加 pass |
| `run(MachineFunction&)` | `void` | 按序执行所有 pass |

### CodeGenContext

| 方法 | 返回 | 说明 |
|------|------|------|
| `CodeGenContext(TargetMachine&, Module&)` | - | 构造 |
| `run()` | `void` | 对模块中所有函数执行代码生成 |
| `addStandardPasses(PassManager&, TargetMachine&)` | `void` | 添加标准管线 |

---

## MC 层 API

### MCStreamer

| 方法 | 返回 | 说明 |
|------|------|------|
| `emitInstruction(mi)` | `void` | 发射指令 |
| `emitLabel(name)` | `void` | 发射标签 |
| `emitComment(text)` | `void` | 发射注释 |
| `emitGlobalSymbol(name)` | `void` | 声明全局符号 |
| `emitAlignment(bytes)` | `void` | 对齐 |
| `emitRawText(text)` | `void` | 原始文本 |

### AsmTextStreamer

| 构造函数 | 说明 |
|----------|------|
| `AsmTextStreamer(ostream&)` | 输出到流 |

---

## ADT 层 API

### SmallVector

| 方法 | 复杂度 | 说明 |
|------|--------|------|
| `push_back(val)` | O(1)* | 追加 |
| `pop_back()` | O(1) | 移除末尾 |
| `operator[](i)` | O(1) | 随机访问 |
| `size()` / `capacity()` | O(1) | 大小/容量 |
| `reserve(n)` | O(n) | 预分配 |
| `clear()` | O(n) | 清空 |
| `erase(pos)` | O(n) | 删除 |

### BitVector

| 方法 | 复杂度 | 说明 |
|------|--------|------|
| `set(idx, val)` | O(1) | 设置位 |
| `test(idx)` | O(1) | 测试位 |
| `count()` | O(n_words) | 计数 |
| `find_first()` | O(n_words) | 找第一个 1 |
| `find_next(idx)` | O(n_words) | 找下一个 1 |
| `any()` / `all()` / `none()` | O(n_words) | 批量检测 |

### BumpPtrAllocator

| 方法 | 复杂度 | 说明 |
|------|--------|------|
| `allocate(size, align)` | O(1)* | 分配 |
| `create<T>(args...)` | O(1)* | placement new |
| `reset()` | O(n_slabs) | 重置 |
| `totalSize()` | O(1) | 总大小 |
