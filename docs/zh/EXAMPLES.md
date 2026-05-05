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

这会触发：

- `Lexer` 识别关键字和运算符
- `Parser` 为函数、语句和表达式生成 AST
- `tools/minic/CodeGen` 生成 AIR 局部变量、调用、分支和循环
- 函数原型 / 外部声明保持可调用，但不会生成函数体
- 标量全局变量定义和 `extern` 全局变量声明
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
