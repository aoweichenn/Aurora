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

## 2. MiniC Control Flow

```mini
long gcd(long a, long b) {
    while (b != 0) {
        long next = a - (a / b) * b;
        a = b;
        b = next;
    }
    return a;
}

long sum_to(long n) {
    long sum = 0;
    for (long i = 0; i <= n; i++) {
        sum += i;
    }
    return sum;
}

long classify(long value) {
    switch (value) {
    case 0:
        return sizeof(long);
    case 1:
        return 'a' - 'a' + 1;
    default:
        return 2;
    }
}

long first(long *items) {
    return items[0];
}

long array_sum() {
    long values[2];
    long *cursor = values;
    cursor[0] = 3;
    cursor += 1;
    *cursor = 4;
    return values[0] + values[1];
}
```

This exercises:

- `Lexer` keyword and operator recognition
- `Parser` AST construction for functions, statements, and expressions
- `tools/minic/CodeGen` emission of AIR local variables, calls, branches, and loops
- backend lowering to x86-64 or macOS arm64 assembly

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
