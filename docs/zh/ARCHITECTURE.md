# 架构

## 代码生成管线

```
源码 (AIR IR)
    ↓
AIRToMachineIRPass        -- 将 AIR 翻译为 MachineInstr
    ↓
InstructionSelectionPass  -- 模式匹配到 x86 操作码
    ↓
CopyCoalescingPass        -- 合并冗余复制
    ↓
PeepholePass              -- 删除无用指令
    ↓
DeadCodeEliminationPass   -- 删除死代码
    ↓
RegisterAllocationPass    -- 线性扫描 + 溢出
    ↓
PrologueEpilogueInserter  -- 栈帧设置/恢复
    ↓
BranchFoldingPass         -- 合并连续跳转
    ↓
Assembly/ELF output       -- X86AsmPrinter / ObjectWriter
```

## AIR 指令集

| 类别 | 操作码 |
|------|--------|
| 终止指令 | Ret, Br, CondBr, Unreachable |
| 算术运算 | Add, Sub, Mul, UDiv, SDiv, URem, SRem |
| 位运算 | And, Or, Xor, Shl, LShr, AShr |
| 比较 | ICmp, FCmp |
| 内存 | Alloca, Load, Store, GetElementPtr |
| 转换 | SExt, ZExt, Trunc, FpToSi, SiToFp, BitCast |
| 控制流 | Phi, Select |
| 函数调用 | Call |
| 结构体 | ExtractValue, InsertValue |
| 常量 | ConstantInt |
| 分支 | Switch |

## 寄存器分配

- 线性扫描算法
- 独立的 GPR 和 XMM 寄存器池
- 寄存器压力溢出代码生成
- 返回值强制映射到 RAX/XMM0

## ELF 输出

`ObjectWriter` 生成标准的 ELF64 .o 文件，包含：
- `.text` 段（编码的 x86-64 机器码）
- `.data` 段（全局变量初始化）
- `.symtab`（函数和数据符号）
- `.strtab`（符号名字符串）
- `.rela.text`（R_X86_64_PLT32/PC32/32S 重定位）
- `.shstrtab`（段名称）
