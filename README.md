# Aurora

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-green.svg)](https://cmake.org)

Aurora is a C++17 x86-64 compiler backend and code generation playground.
The repository currently provides an AIR IR, a target abstraction layer, x86-specific lowering and encoding, an ELF object writer, and a MiniC front-end used by the sample tools and tests.

## Quick Start

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
./build/tools/minic/minic test.mini
```

## Documentation

- Chinese: `docs/zh/README.md`
- English: `docs/en/README.md`

## Current Scope

- `ADT`: `SmallVector`, `BitVector`, `Graph`, `SparseSet`, `BumpPtrAllocator`
- `AIR`: types, constants, functions, basic blocks, instructions, builders, modules
- `Target`: register, instruction, lowering, calling convention, and frame-lowering abstractions
- `X86`: concrete x86-64 implementation for the target layer
- `CodeGen`: selection DAG scaffolding, instruction selection, register allocation, and pass pipeline
- `MC`: assembly printer, object encoder, and ELF relocatable writer
- `tools/minic`: lexer, parser, AST, and AIR generation for the Mini language
- `tools/aurorac`: sample driver for the backend pipeline
- `tools/aurora-obj`: ELF object writer sample driver

## Notes

- Keep the Chinese and English docs in sync.
- The repo ships with tests for ADT, AIR, Target, CodeGen, and MC.
- For implementation details and public interfaces, see the mirrored documentation under `docs/zh` and `docs/en`.
