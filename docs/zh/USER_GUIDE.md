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

MiniC 接受一个 `.mini` 源文件，支持当前阶段的语法：

- 函数定义：`fn name(a, b) = expr`
- 整数字面量和变量引用
- `+ - * /`
- 比较：`== != < <= > >=`
- `if cond then expr else expr`
- 函数调用

示例：

```mini
fn add(a, b) = a + b
fn abs(x) = if x < 0 then 0 - x else x
fn max(a, b) = if a > b then a else b
```

运行：

```bash
./build/tools/minic/minic test.mini
```

输出包含两部分：

- AIR IR 打印
- x86-64 AT&T 汇编打印

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

## 5. 生成对象文件

```cpp
aurora::ObjectWriter writer;
writer.addFunction(mf);
writer.addGlobal(*globalVar);
writer.write("out.o");
```

`ObjectWriter` 会写出 ELF relocatable object，包括：

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
