# 用户手册

## 1. 构建

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
```

默认会生成：

- `build/tools/minic/minic`
- `build/tools/aurorac/aurorac`
- `build/tools/aurora-obj/aurora-obj`

## 2. 运行 MiniC

MiniC 接受一个 `.mini` 源文件，支持 C-like 整数子集：

- 函数定义：`long name(long a, long b) { ... }`，支持 `void` 返回值和简单指针形参 / 返回值
- 局部变量和可选初始化：标量、指针，以及语法级定长数组声明
- 语句：`return`、块、`if` / `else`、`while`、`do` / `while`、`for`、`switch` / `case` / `default`、`break`、`continue`
- 表达式：赋值、复合赋值、函数调用、三元 `?:`、`sizeof`、指针解引用 / 取地址、指针下标、前后缀 `++` / `--`
- 运算符：`+ - * / %`、比较、逻辑运算、位运算、移位，以及 `&= |= ^= <<= >>=`
- 兼容旧表达式函数：`fn name(a, b) = expr`

示例：

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

运行：

```bash
./build/tools/minic/minic test.mini
./build/tools/minic/minic --target=arm64 test.mini
```

输出包含两部分：

- AIR IR 打印
- 目标汇编打印；`x86_64` 是默认目标，`arm64` 会输出 macOS arm64 汇编

## 3. 直接使用 AIR API

最小用法：

```cpp
auto module = std::make_unique<aurora::Module>("example");
aurora::SmallVector<aurora::Type*, 8> params = {aurora::Type::getInt32Ty(), aurora::Type::getInt32Ty()};
auto* fnTy = new aurora::FunctionType(aurora::Type::getInt32Ty(), params);
auto* fn = module->createFunction(fnTy, "add");
aurora::AIRBuilder builder(fn->getEntryBlock());
unsigned sum = builder.createAdd(aurora::Type::getInt32Ty(), 0, 1);
builder.createRet(sum);
```

常用入口：

- `aurora::Module`
- `aurora::FunctionType`
- `aurora::Function`
- `aurora::BasicBlock`
- `aurora::AIRBuilder`
- `aurora::AIRInstruction`

## 4. 生成汇编

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

如果要输出 macOS arm64 汇编，可使用 `aurora::TargetMachine::createAArch64_Apple()` 搭配 `aurora::AArch64AsmPrinter`。

## 5. 生成对象文件

```cpp
aurora::ObjectWriter writer;
writer.addFunction(mf);
writer.addGlobal(*globalVar);
writer.write("out.o");
```

`ObjectWriter` 当前会写出 x86-64 ELF relocatable object，包括：

- `.text`
- `.data`
- `.symtab`
- `.strtab`
- `.rela.text`

## 6. 测试

```bash
ctest --test-dir build --output-on-failure
```

当前测试覆盖：

- `ADT`
- `AIR`
- `Target`
- `CodeGen`
- `MC`
