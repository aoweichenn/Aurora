# Examples

## 1. Minimal AIR Generation

```cpp
auto module = std::make_unique<aurora::Module>("example");
aurora::SmallVector<aurora::Type*, 8> params = {aurora::Type::getInt32Ty(), aurora::Type::getInt32Ty()};
auto* fn = module->createFunction(new aurora::FunctionType(aurora::Type::getInt32Ty(), params), "add");
aurora::AIRBuilder builder(fn->getEntryBlock());
unsigned sum = builder.createAdd(aurora::Type::getInt32Ty(), 0, 1);
builder.createRet(sum);
```

This produces a trivial `add` function backed by AIR `add` and `ret` instructions.

## 2. Mini Language Control Flow

```mini
fn abs(x) = if x < 0 then 0 - x else x
fn max(a, b) = if a > b then a else b
```

This exercises:

- `Lexer` keyword and operator recognition
- `Parser` AST construction
- `tools/minic/CodeGen` emission of AIR `icmp`, `condbr`, and `phi`
- backend lowering to x86 assembly

## 3. Emit an Object File

```cpp
auto tm = aurora::TargetMachine::createX86_64();
aurora::MachineFunction mf(*fn, *tm);
aurora::PassManager pm;
aurora::CodeGenContext::addStandardPasses(pm, *tm);
pm.run(mf);

aurora::ObjectWriter writer;
writer.addFunction(mf);
writer.write("out.o");
```

Useful for checking:

- symbol-table correctness
- relocation emission
- `.text` / `.data` layout

## 4. Inspect the Backend Pipeline

```cpp
aurora::AsmTextStreamer streamer(std::cout);
const auto& ri = static_cast<const aurora::X86RegisterInfo&>(tm->getRegisterInfo());
aurora::X86AsmPrinter printer(streamer, ri);
printer.emitFunction(mf);
```

Useful for checking:

- instruction selection results
- register allocation results
- prologue/epilogue insertion
- jump and PHI lowering
