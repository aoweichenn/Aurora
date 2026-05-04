#include <gtest/gtest.h>
#include "Aurora/Air/Builder.h"
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
    EXPECT_NE(asmText.find("add\tx0, x0, x1"), std::string::npos);
    EXPECT_NE(asmText.find("ret"), std::string::npos);
}
