# Aurora - Production-Grade x86-64 Compiler Backend

Aurora is a production-grade x86-64 compiler backend framework written in C++17, inspired by LLVM's architecture. It converts custom SSA IR (AIR) to x86-64 AT&T assembly and ELF object files.

## Quick Start

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/tools/minic/minic test.mini
./build/tools/aurora-obj/aurora-obj test.o
```

## Module Architecture

```
ADT (data structures) → Air (SSA IR) → Target (platform abstraction)
                                        ↓
                                      X86 (x86-64 implementation)
                                        ↓
CodeGen (instruction selection, register allocation) → MC (assembly/ELF output)
```

| Module | Purpose |
|--------|---------|
| ADT | SmallVector, BitVector, Graph, BumpAllocator, SparseSet |
| Air | SSA IR with ~40 opcodes, Type system, AIRBuilder |
| Target | Abstract base classes for target machines |
| X86 | x86-64 register info, instruction set, ISel patterns, calling convention, frame lowering |
| CodeGen | SelectionDAG, InstructionSelector, LinearScan register allocator, pass manager |
| MC | Assembly printer (AT&T syntax), ELF object writer |

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `AURORA_BUILD_TESTS` | ON | Build test suites (GoogleTest) |
| `AURORA_BUILD_BENCHMARKS` | ON | Build benchmarks (Google Benchmark) |
| `AURORA_BUILD_SHARED_LIBS` | ON | Build shared libraries |
| `AURORA_ENABLE_COVERAGE` | OFF | Enable lcov code coverage |

## Coverage

```bash
cmake -B build -S . -DAURORA_ENABLE_COVERAGE=ON -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/_deps/*' '*/tests/*' -o coverage.info
genhtml coverage.info --output-directory coverage_report
```

## Tools

- **minic**: Mini-language compiler (expression → AIR → assembly)
- **aurorac**: Sample AIR construction and assembly emission
- **aurora-obj**: AIR → ELF .o file writer

## License

MIT License
