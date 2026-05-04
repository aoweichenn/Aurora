# Architecture

## Code Generation Pipeline

```
Source (AIR IR)
    ↓
AIRToMachineIRPass        -- Translate AIR to MachineInstrs
    ↓
InstructionSelectionPass  -- Pattern-match to x86 opcodes
    ↓
CopyCoalescingPass        -- Merge redundant copies
    ↓
PeepholePass              -- Remove no-op instructions
    ↓
DeadCodeEliminationPass   -- Remove unused instructions
    ↓
RegisterAllocationPass    -- Linear scan + spill
    ↓
PrologueEpilogueInserter  -- Frame setup/teardown
    ↓
BranchFoldingPass         -- Merge consecutive jumps
    ↓
Assembly/ELF output       -- X86AsmPrinter / ObjectWriter
```

## AIR Instruction Set

| Category | Opcodes |
|----------|---------|
| Terminators | Ret, Br, CondBr, Unreachable |
| Arithmetic | Add, Sub, Mul, UDiv, SDiv, URem, SRem |
| Bitwise | And, Or, Xor, Shl, LShr, AShr |
| Comparison | ICmp, FCmp |
| Memory | Alloca, Load, Store, GetElementPtr |
| Conversion | SExt, ZExt, Trunc, FpToSi, SiToFp, BitCast |
| Control | Phi, Select |
| Call | Call |
| Struct | ExtractValue, InsertValue |
| Constant | ConstantInt |
| Switch | Switch |

## Register Allocation

- Linear scan algorithm
- Separate GPR and XMM pools
- Spill code generation for register pressure
- Return value forced to RAX/XMM0

## ELF Output

The `ObjectWriter` produces standard ELF64 .o files with:
- `.text` section (encoded x86-64 machine code)
- `.data` section (global variable initialization)
- `.symtab` (function and data symbols)
- `.strtab` (symbol name strings)
- `.rela.text` (R_X86_64_PLT32/PC32/32S relocations)
- `.shstrtab` (section names)
