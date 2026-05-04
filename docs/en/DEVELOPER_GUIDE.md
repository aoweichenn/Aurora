# Developer Guide

## 1. Repository Layout

- `include/Aurora/ADT`: core containers and utilities
- `include/Aurora/Air`: AIR IR and builders
- `include/Aurora/Target`: target abstractions and x86 implementation
- `include/Aurora/CodeGen`: MIR, SelectionDAG, passes, and register allocation
- `include/Aurora/MC`: assembly printing, object writing, and encoders
- `tools/minic`: sample front-end
- `tools/aurorac`: backend sample driver
- `tools/aurora-obj`: object-file sample driver
- `tests`: module-oriented unit and coverage tests

## 2. Code Conventions

- Put public interfaces in `include/` and implementations in `lib/`.
- Any new behavior must include tests.
- Keep Chinese and English docs mirrored.
- Favor the simplest correct design for the current stage.

## 3. Adding a New AIR Instruction

1. Add the enum value to `include/Aurora/Air/Instruction.h`.
2. Add the factory method and accessors in `lib/Air/Instruction.cpp`.
3. Add the builder wrapper in `include/Aurora/Air/Builder.h` and `lib/Air/Builder.cpp`.
4. Update `tools/minic` grammar/codegen if the front-end needs the feature.
5. Add lowering in `lib/CodeGen/Passes/PassManager.cpp`.
6. Add assembly printing and object encoding in `lib/MC/X86/X86AsmPrinter.cpp` and `lib/MC/X86/X86ObjectEncoder.cpp`.
7. Add tests in `tests/Air`, `tests/CodeGen`, and `tests/MC`.

## 4. Adding a New x86 Instruction

1. Add the opcode in `include/Aurora/Target/X86/X86InstrInfo.h`.
2. Extend the opcode description table in `lib/Target/X86/X86InstrInfo.cpp`.
3. Add assembly printing in `lib/MC/X86/X86AsmPrinter.cpp`.
4. Add binary encoding in `lib/MC/X86/X86ObjectEncoder.cpp`.
5. Update lowering in `lib/CodeGen/Passes/PassManager.cpp` if needed.
6. Add regression tests for printing and encoding.

## 5. Adding a New Pass

1. Implement a `CodeGenPass` subclass.
2. Declare the factory in `include/Aurora/CodeGen/Passes/PassFactories.h`.
3. Define the factory in `lib/CodeGen/Passes/PassManager.cpp`.
4. Register it from `CodeGenContext::addStandardPasses()`.
5. Add coverage in `tests/CodeGen`.

## 6. Adding Front-End Syntax

1. Extend `tools/minic/Lexer.h` and `tools/minic/Lexer.cpp`.
2. Extend `tools/minic/Parser.h` and `tools/minic/Parser.cpp`.
3. Update `tools/minic/AST.h` and `tools/minic/CodeGen.cpp`.
4. Add test coverage for the new syntax.

## 7. Debugging Tips

- Start with the smallest relevant unit test.
- Use `tools/minic` to inspect generated AIR and assembly.
- Use `tools/aurora-obj` to inspect object-file output.
- Define the header interface first, then implement and test it.
