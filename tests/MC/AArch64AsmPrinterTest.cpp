#include <gtest/gtest.h>
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/MC/AArch64/AArch64AsmPrinter.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/Target/TargetMachine.h"
#include <sstream>

using namespace aurora;

TEST(AArch64AsmPrinterTest, EmitsMiniArithmeticFunction) {
    Module module("a64");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fn = module.createFunction(new FunctionType(Type::getInt64Ty(), params), "add");
    AIRBuilder builder(fn->getEntryBlock());
    builder.createRet(builder.createAdd(Type::getInt64Ty(), 0, 1));

    auto tm = TargetMachine::createAArch64_Apple();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    std::ostringstream out;
    AsmTextStreamer streamer(out);
    const auto& ri = dynamic_cast<const AArch64RegisterInfo&>(tm->getRegisterInfo());
    AArch64AsmPrinter printer(streamer, ri);
    printer.emitFunction(mf);

    const std::string asmText = out.str();
    EXPECT_NE(asmText.find(".globl _add"), std::string::npos);
    EXPECT_NE(asmText.find("add\t"), std::string::npos);
    EXPECT_NE(asmText.find("mov\tx0"), std::string::npos);
    EXPECT_NE(asmText.find("ret"), std::string::npos);
}

TEST(AArch64AsmPrinterTest, EmitsDarwinGlobalSymbols) {
    Module module("a64-globals");
    auto* global = module.createGlobal(Type::getInt64Ty(), "counter");
    global->setInitializer(ConstantInt::getInt64(42));

    auto tm = TargetMachine::createAArch64_Apple();
    std::ostringstream out;
    AsmTextStreamer streamer(out);
    const auto& ri = dynamic_cast<const AArch64RegisterInfo&>(tm->getRegisterInfo());
    AArch64AsmPrinter printer(streamer, ri);
    printer.emitGlobals(module);

    const std::string asmText = out.str();
    EXPECT_NE(asmText.find(".data"), std::string::npos);
    EXPECT_NE(asmText.find(".globl _counter"), std::string::npos);
    EXPECT_NE(asmText.find("_counter:"), std::string::npos);
    EXPECT_NE(asmText.find("\t.quad 42"), std::string::npos);
}

TEST(AArch64AsmPrinterTest, EmitsDarwinArrayGlobalSymbols) {
    Module module("a64-array-globals");
    auto* arrayType = Type::getArrayTy(Type::getInt64Ty(), 3);
    auto* global = module.createGlobal(arrayType, "values");
    global->setInitializer(ConstantArray::get(arrayType, {
        ConstantInt::getInt64(3),
        ConstantInt::getInt64(4),
        ConstantInt::getInt64(0),
    }));

    auto tm = TargetMachine::createAArch64_Apple();
    std::ostringstream out;
    AsmTextStreamer streamer(out);
    const auto& ri = dynamic_cast<const AArch64RegisterInfo&>(tm->getRegisterInfo());
    AArch64AsmPrinter printer(streamer, ri);
    printer.emitGlobals(module);

    const std::string asmText = out.str();
    EXPECT_NE(asmText.find(".globl _values"), std::string::npos);
    EXPECT_NE(asmText.find("\t.quad 3"), std::string::npos);
    EXPECT_NE(asmText.find("\t.quad 4"), std::string::npos);
    EXPECT_NE(asmText.find("\t.quad 0"), std::string::npos);
}

TEST(AArch64AsmPrinterTest, LowersGlobalAddressReferences) {
    Module module("a64-global-load");
    (void)module.createGlobal(Type::getInt64Ty(), "counter");
    SmallVector<Type*, 8> params;
    auto* fn = module.createFunction(new FunctionType(Type::getInt64Ty(), params), "read_counter");
    AIRBuilder builder(fn->getEntryBlock());
    unsigned address = builder.createGlobalAddress(Type::getPointerTy(Type::getInt64Ty()), "counter");
    builder.createRet(builder.createLoad(Type::getInt64Ty(), address));

    auto tm = TargetMachine::createAArch64_Apple();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    std::ostringstream out;
    AsmTextStreamer streamer(out);
    const auto& ri = dynamic_cast<const AArch64RegisterInfo&>(tm->getRegisterInfo());
    AArch64AsmPrinter printer(streamer, ri);
    printer.emitFunction(mf);

    const std::string asmText = out.str();
    EXPECT_NE(asmText.find("_counter"), std::string::npos);
    EXPECT_NE(asmText.find("ldr\t"), std::string::npos);
    EXPECT_EQ(asmText.find("unknown opcode"), std::string::npos);
}
