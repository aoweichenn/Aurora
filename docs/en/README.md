# Aurora

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](../../LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-green.svg)](https://cmake.org)

Aurora is a C++17 x86-64 compiler backend and code generation playground.
The repository currently includes an AIR IR, a target abstraction layer, x86-specific lowering and encoding, an ELF relocatable object writer, and a MiniC front-end used by the sample tools and tests.

## Quick Start

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
./build/tools/minic/minic test.mini
```

## Documentation Map

- `ARCHITECTURE.md`: current architecture, pipeline, and module layering
- `API_REFERENCE.md`: current public interface catalog
- `USER_GUIDE.md`: build, run, and integration workflows
- `DEVELOPER_GUIDE.md`: workflows for extending AIR, x86 instructions, passes, and MiniC syntax
- `EXAMPLES.md`: examples for AIR, MiniC, assembly output, and object-file output
- Chinese: `../zh/README.md`

## Current Scope

- `ADT`: `SmallVector`, `BitVector`, `Graph`, `SparseSet`, `BumpPtrAllocator`
- `AIR`: types, constants, functions, basic blocks, instructions, builders, modules
- `Target`: register, instruction, lowering, calling-convention, and frame-lowering abstractions
- `X86`: concrete x86-64 implementation of the target layer
- `CodeGen`: SelectionDAG scaffolding, instruction selection, register allocation, and pass pipeline
- `MC`: assembly printer, object encoder, and ELF relocatable writer
- `tools/minic`: lexer, parser, AST, and AIR generation for the Mini language
- `tools/aurorac`: sample backend driver
- `tools/aurora-obj`: sample ELF object writer driver

## Notes

- Keep the English and Chinese docs mirrored.
- The repository ships with tests for ADT, AIR, Target, CodeGen, and MC.
- The current implementation focuses on being runnable, tested, and extensible; the docs no longer describe it as “production-grade”.
