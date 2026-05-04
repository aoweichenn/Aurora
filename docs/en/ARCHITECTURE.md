# Architecture

Aurora is currently a combination of x86-64 and macOS arm64 assembly backends with a small sample front-end.
The end-to-end flow is:

```text
Mini source -> Lexer -> Parser -> AST -> MiniC CodeGen -> AIR Module
-> PassManager -> MachineFunction/MIR -> AsmPrinter or ObjectWriter -> assembly/x86-64 ELF object
```

## Layers

| Layer | Responsibility |
|-------|----------------|
| `ADT` | Foundational utilities: `SmallVector`, `BitVector`, `Graph`, `SparseSet`, `BumpPtrAllocator` |
| `AIR` | SSA-style IR: types, constants, functions, basic blocks, instructions, builders, modules |
| `Target` | Target abstraction: registers, instructions, lowering, calling convention, stack frame |
| `Target/X86` | Concrete x86-64 implementation |
| `Target/AArch64` | Concrete macOS arm64 implementation for assembly output |
| `CodeGen` | AIR to MIR, instruction selection, register allocation, stack frame insertion, branch folding |
| `MC` | Assembly printing, x86-64 machine-code encoding, x86-64 ELF relocatable output |
| `tools/minic` | Mini language front-end |

## Core Objects

- `Module` owns `Function` and `GlobalVariable` instances.
- `Function` owns `BasicBlock` objects and tracks virtual register numbering and types.
- `AIRBuilder` emits AIR instructions at the current insertion point.
- `PassManager` runs the standard pipeline; `CodeGenContext::addStandardPasses` registers the default passes.
- `MachineFunction` carries MIR, stack objects, virtual register types, and target information.
- `SelectionDAG` exposes DAG nodes, constants, register references, and basic select/schedule hooks.
- `AsmPrinter` and `ObjectWriter` are the two backend output paths for text assembly and x86-64 ELF objects.

## Current Pipeline

1. AIR to MachineIR: maps AIR instructions directly to MIR.
2. Instruction Selection: lowers AIR operations to target instructions.
3. Register Allocation: linear-scan register allocation with spills to stack slots.
4. Prologue/Epilogue Insertion: inserts function prologue and epilogue code.
5. Branch Folding: removes redundant jumps and threads simple jump chains.

## Notes

- The current implementation focuses on correctness and test coverage, not full LLVM-grade optimization.
- SelectionDAG interfaces are present, but primarily as the current-stage structure and extension point.
- Documentation and code should stay mirrored between Chinese and English.
