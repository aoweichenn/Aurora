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
long global_values[3] = {[2] = 3, [0] = 1};

enum Mode {
    MODE_ZERO,
    MODE_ONE = 4,
};

static_assert(MODE_ONE == 4, "enum constants work");
static_assert(alignof(long) == 8, "alignment constants work");

struct Pair {
    long first;
    alignas(16) long second;
};

static_assert(alignof(struct Pair) == 16, "alignas raises record alignment");

struct Pair global_pair = {.second = 6, .first = 2};
struct Pair global_pairs[2] = {{1, 2}, {.second = 8, .first = 4}};

union Slot {
    char tag;
    long value;
};

struct Outer {
    long values[2];
    struct Pair pair;
    union Slot slot;
};

struct Outer global_outer = {{1, 2}, {.second = 8, .first = 4}, {.value = 9}};
struct Outer global_outers[2] = {[1].pair.second = 12, [0].values[1] = 3};

long use_c23_bits(usize value) {
    bool ok = true;
    long casted = (long)value;
    usize high = value >> 63;
    return ok && nullptr == 0 ? casted + high + MODE_ONE + alignof(usize) : 0;
}

long declared_later(long value) {
    return value + 1;
}

long use_global_counter() {
    global_counter = global_counter + imported_counter;
    return global_counter;
}

long use_global_array() {
    global_values[2] = global_counter;
    return global_values[0] + global_values[1] + global_values[2];
}

long use_global_record() {
    return global_pair.first + global_pair.second;
}

long use_global_record_array() {
    return global_pairs[0].second + global_pairs[1].first;
}

long use_global_nested_record() {
    return global_outer.values[1] + global_outer.pair.second + global_outer.slot.value;
}

long use_global_nested_designators() {
    return global_outers[0].values[1] + global_outers[1].pair.second;
}

long use_struct_fields() {
    struct Pair pair = {.second = 5, .first = 4};
    alignas(16) struct Pair *cursor = &pair;
    cursor->second = pair.first + cursor->second;
    return pair.second + sizeof(struct Pair);
}

long use_nested_records() {
    struct Outer outer = {.values[1] = 2, .pair.second = 5, .pair.first = 4, .slot.value = 7};
    return outer.values[1] + outer.pair.first + outer.slot.value;
}

long use_union_fields() {
    union Slot *slot = &(union Slot){.value = 7};
    slot->value = slot->value + alignof(union Slot);
    return slot->value;
}

long use_compound_literals() {
    long *values = (long[3]){[1] = 5, [0] = 2};
    struct Pair *pairs = (struct Pair[2]){[1].second = 13, [0] = {.first = 3, .second = 11}};
    return values[0] + values[1] + pairs[0].second + pairs[1].second;
}
```

This exercises:

- `Lexer` keyword and operator recognition
- `Parser` AST construction for functions, statements, and expressions
- `tools/minic/CodeGen` emission of AIR local variables, calls, branches, and loops
- function prototypes / external declarations that stay callable without emitting bodies
- scalar, one-dimensional array, named record, and record-array global definitions plus `extern` global declarations
- C23 spelling for constant alignment queries through `alignof(type)` and `_Alignof(type)`
- C23 alignment specifiers through `alignas(n)` and `_Alignas(type)`
- named `struct` / `union` layout, global and local record initialization including nested array and record fields, and `.` / `->` field access
- designated initializer paths for arrays and records, including global integer and record arrays
- compound literals for scalar, array, record-array, `struct`, and `union` temporaries
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
