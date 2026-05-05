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

long first(long items[]) {
    return items[0];
}

long array_sum() {
    long values[3] = {3, 4};
    long *cursor = values;
    cursor += 2;
    *cursor = 5;
    return values[0] + values[1] + values[2];
}

typedef unsigned long usize;

extern long imported(long);
extern long imported_counter;

long declared_later(long);
long global_counter = 7;

enum Mode {
    MODE_ZERO,
    MODE_ONE = 4,
};

static_assert(MODE_ONE == 4, "enum constants work");

long use_c23_bits(usize value) {
    bool ok = true;
    long casted = (long)value;
    usize high = value >> 63;
    return ok && nullptr == 0 ? casted + high + MODE_ONE : 0;
}

long declared_later(long value) {
    return value + 1;
}

long use_global_counter() {
    global_counter = global_counter + imported_counter;
    return global_counter;
}
```

This exercises:

- `Lexer` keyword and operator recognition
- `Parser` AST construction for functions, statements, and expressions
- `tools/minic/CodeGen` emission of AIR local variables, calls, branches, and loops
- function prototypes / external declarations that stay callable without emitting bodies
- scalar global definitions and `extern` global declarations
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
