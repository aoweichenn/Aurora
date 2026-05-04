# User Guide

## 1. Build

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
```

The build produces:

- `build/tools/minic/minic`
- `build/tools/aurorac/aurorac`
- `build/tools/aurora-obj/aurora-obj`

## 2. Run MiniC

MiniC accepts a `.mini` source file and supports the current-stage syntax:

- function definitions: `fn name(a, b) = expr`
- integer literals and variables
- `+ - * /`
- comparisons: `== != < <= > >=`
- `if cond then expr else expr`
- function calls

Example:

```mini
fn add(a, b) = a + b
fn abs(x) = if x < 0 then 0 - x else x
fn max(a, b) = if a > b then a else b
```

Run it with:

```bash
./build/tools/minic/minic test.mini
```

The output contains:

- AIR IR printing
- x86-64 AT&T assembly printing

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

## 5. Emit an object file

```cpp
aurora::ObjectWriter writer;
writer.addFunction(mf);
writer.addGlobal(*globalVar);
writer.write("out.o");
```

`ObjectWriter` emits an ELF relocatable object containing:

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
