# User Guide

## 1. Build

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
```

MiniC coverage is gated at 90% line coverage and 80% branch coverage:

```bash
cmake -B build-coverage -S . -DCMAKE_BUILD_TYPE=Debug -DAURORA_BUILD_TESTS=ON -DAURORA_BUILD_BENCHMARKS=OFF -DAURORA_ENABLE_COVERAGE=ON
cmake --build build-coverage -j8 --target minic_coverage
```

The build produces:

- `build/tools/minic/minic`
- `build/tools/aurorac/aurorac`
- `build/tools/aurora-obj/aurora-obj`

## 2. Run MiniC

MiniC accepts a `.mini` source file and supports a C-like integer subset:

- function definitions and prototypes: `long name(long a, long b) { ... }`, `extern long name(long);`, with `void` returns, simple pointer parameters / return values, optional prototype parameter names, array parameters that decay to pointers, and common storage-class / qualifier parsing
- top-level globals: scalar globals such as `long counter = 7;`, `extern long imported;`, fixed-size one-dimensional arrays such as `long values[3] = {1, 2};`, and named `struct` / `union` globals or record arrays with braced initializers, including nested array and record fields, with reads, writes, indexing, field access, and address-of from functions
- local variables with optional initializers: scalars, pointers, named `struct` / `union` objects, and fixed-size arrays including record arrays with nested braced initializer lists plus array/record designators
- statements: `return`, blocks, `if` / `else`, `while`, `do` / `while`, `for`, `switch` / `case` / `default`, `break`, and `continue`
- expressions: assignment, compound assignment, calls, ternary `?:`, short-circuit `&&` / `||`, C-style casts, compound literals such as `(struct Pair){.x = 1}` and `(long[3]){[2] = 5}`, `sizeof`, `alignof` / `_Alignof`, `.` / `->` record field access, pointer arithmetic, pointer dereference / address-of, pointer subscripting, prefix/postfix `++` / `--`
- declarations and constants: `typedef`, `enum`, named `struct` / `union` definitions with scalar/pointer/array/record fields, `alignas` / `_Alignas` alignment specifiers, designated initializers such as `[2] = 7` and `.field = 7` in declarations and compound literals, `static_assert` / `_Static_assert`, `bool` / `_Bool`, `true`, `false`, `nullptr`
- operators: `+ - * / %`, signed and unsigned comparisons / division / remainder / right shift, logical operators, bitwise operators, shifts, and `&= |= ^= <<= >>=`
- legacy expression functions: `fn name(a, b) = expr`

Example:

```mini
long sum_to(long n) {
    long sum = 0;
    for (long i = 0; i <= n; i++) {
        sum += i;
    }
    return sum;
}

long main() {
    return sum_to(5);
}
```

Run it with:

```bash
./build/tools/minic/minic test.mini
./build/tools/minic/minic --target=arm64 test.mini
```

The output contains:

- AIR IR printing
- target assembly printing; `x86_64` is the default, and `arm64` emits macOS arm64 assembly

## 3. Use the AIR API directly

Minimal example:

```cpp
auto module = std::make_unique<aurora::Module>("example");
aurora::SmallVector<aurora::Type*, 8> params = {aurora::Type::getInt32Ty(), aurora::Type::getInt32Ty()};
auto* fnTy = new aurora::FunctionType(aurora::Type::getInt32Ty(), params);
auto* fn = module->createFunction(fnTy, "add");
aurora::AIRBuilder builder(fn->getEntryBlock());
unsigned sum = builder.createAdd(aurora::Type::getInt32Ty(), 0, 1);
builder.createRet(sum);
```

Common entry points:

- `aurora::Module`
- `aurora::FunctionType`
- `aurora::Function`
- `aurora::BasicBlock`
- `aurora::AIRBuilder`
- `aurora::AIRInstruction`

## 4. Emit assembly

```cpp
auto tm = aurora::TargetMachine::createX86_64();
aurora::MachineFunction mf(*fn, *tm);
aurora::PassManager pm;
aurora::CodeGenContext::addStandardPasses(pm, *tm);
pm.run(mf);

aurora::AsmTextStreamer streamer(std::cout);
const auto& ri = static_cast<const aurora::X86RegisterInfo&>(tm->getRegisterInfo());
aurora::X86AsmPrinter printer(streamer, ri);
printer.emitFunction(mf);
```

Use `aurora::TargetMachine::createAArch64_Apple()` with `aurora::AArch64AsmPrinter` to emit macOS arm64 assembly.

## 5. Emit an object file

```cpp
aurora::ObjectWriter writer;
writer.addFunction(mf);
writer.addGlobal(*globalVar);
writer.write("out.o");
```

`ObjectWriter` currently emits an x86-64 ELF relocatable object containing:

- `.text`
- `.data`
- `.symtab`
- `.strtab`
- `.rela.text`

## 6. Tests

```bash
ctest --test-dir build --output-on-failure
```

Current coverage includes:

- `ADT`
- `AIR`
- `Target`
- `CodeGen`
- `MC`
