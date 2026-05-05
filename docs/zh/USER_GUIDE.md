# 用户手册

## 1. 构建

```bash
cmake -B build -S . -DAURORA_BUILD_TESTS=ON
cmake --build build -j8
ctest --test-dir build
```

MiniC 覆盖率门禁要求行覆盖率 90%、分支覆盖率 80%：

```bash
cmake -B build-coverage -S . -DCMAKE_BUILD_TYPE=Debug -DAURORA_BUILD_TESTS=ON -DAURORA_BUILD_BENCHMARKS=OFF -DAURORA_ENABLE_COVERAGE=ON
cmake --build build-coverage -j8 --target minic_coverage
```

默认会生成：

- `build/tools/minic/minic`
- `build/tools/aurorac/aurorac`
- `build/tools/aurora-obj/aurora-obj`

## 2. 运行 MiniC

MiniC 接受一个 `.mini` 源文件，支持 C-like 整数子集：

- 函数定义和函数原型：`long name(long a, long b) { ... }`、`extern long name(long);`，支持 `void` 返回值、简单指针形参 / 返回值、原型中可省略形参名、会退化为指针的数组形参，以及常见存储类 / 限定符解析
- 顶层全局变量：标量全局如 `long counter = 7;`、`extern long imported;`，以及定长一维数组如 `long values[3] = {1, 2};`，支持函数内读取、写入、下标和取地址
- 局部变量和可选初始化：标量、指针、命名 `struct` / `union` 对象，以及带一维大括号初始化列表和数组 / 记录 designator 的定长数组
- 语句：`return`、块、`if` / `else`、`while`、`do` / `while`、`for`、`switch` / `case` / `default`、`break`、`continue`
- 表达式：赋值、复合赋值、函数调用、三元 `?:`、短路 `&&` / `||`、C 风格 cast、`(struct Pair){.x = 1}` 与 `(long[3]){[2] = 5}` 这类 compound literal、`sizeof`、`alignof` / `_Alignof`、`.` / `->` 记录类型字段访问、指针算术、指针解引用 / 取地址、指针下标、前后缀 `++` / `--`
- 声明和常量：`typedef`、`enum`、带标量 / 指针 / 数组字段的命名 `struct` / `union` 定义、声明和 compound literal 中的 `[2] = 7` 与 `.field = 7` 这类 designated initializer、`static_assert` / `_Static_assert`、`bool` / `_Bool`、`true`、`false`、`nullptr`
- 运算符：`+ - * / %`、有符号和无符号比较 / 除法 / 取余 / 右移、逻辑运算、位运算、移位，以及 `&= |= ^= <<= >>=`
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
