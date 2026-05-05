# 示例

## 1. 最小 AIR 生成

```cpp
auto module = std::make_unique<aurora::Module>("example");
aurora::SmallVector<aurora::Type*, 8> params = {aurora::Type::getInt32Ty(), aurora::Type::getInt32Ty()};
auto* fn = module->createFunction(new aurora::FunctionType(aurora::Type::getInt32Ty(), params), "add");
aurora::AIRBuilder builder(fn->getEntryBlock());
unsigned sum = builder.createAdd(aurora::Type::getInt32Ty(), 0, 1);
builder.createRet(sum);
```

这个例子会生成一个简单的 `add` 函数，对应 AIR 中的 `add` 和 `ret`。

## 2. MiniC 控制流

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

long use_struct_fields() {
    struct Pair pair = {.second = 5, .first = 4};
    alignas(16) struct Pair *cursor = &pair;
    cursor->second = pair.first + cursor->second;
    return pair.second + sizeof(struct Pair);
}

long use_nested_records() {
    struct Outer outer = {{1, 2}, {.second = 5, .first = 4}, {.value = 7}};
    return outer.values[1] + outer.pair.first + outer.slot.value;
}

long use_union_fields() {
    union Slot *slot = &(union Slot){.value = 7};
    slot->value = slot->value + alignof(union Slot);
    return slot->value;
}

long use_compound_literals() {
    long *values = (long[3]){[1] = 5, [0] = 2};
    return values[0] + values[1] + ((struct Pair){.second = 11, .first = 3}).second;
}
```

这会触发：

- `Lexer` 识别关键字和运算符
- `Parser` 为函数、语句和表达式生成 AST
- `tools/minic/CodeGen` 生成 AIR 局部变量、调用、分支和循环
- 函数原型 / 外部声明保持可调用，但不会生成函数体
- 标量、一维数组、命名 record 和 record 数组全局变量定义，以及 `extern` 全局变量声明
- 通过 `alignof(type)` 和 `_Alignof(type)` 查询常量对齐值的 C23 写法
- 通过 `alignas(n)` 和 `_Alignas(type)` 描述对齐要求的 C23 写法
- 命名 `struct` / `union` 布局、包括嵌套数组和 record 字段在内的全局 / 局部记录类型初始化，以及 `.` / `->` 字段访问
- 数组和记录类型 designated initializer，包括全局整数数组
- 标量、数组、`struct` 和 `union` 临时对象的 compound literal
- 后端生成 x86-64 或 macOS arm64 汇编

## 3. 生成对象文件

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

适合验证：

- 符号表是否正确
- 重定位是否生成
- `.text` / `.data` 是否布局正确

## 4. 观察后端流水线

```cpp
aurora::AsmTextStreamer streamer(std::cout);
const auto& ri = static_cast<const aurora::X86RegisterInfo&>(tm->getRegisterInfo());
aurora::X86AsmPrinter printer(streamer, ri);
printer.emitFunction(mf);
```

适合用来检查：

- 指令选择结果
- 寄存器分配结果
- 序言/尾声
- 跳转和 PHI lowering
