#include <gtest/gtest.h>
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86CallingConv.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86AsmPrinter.h"
#include "Aurora/MC/AsmPrinter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include <sstream>

using namespace aurora;

// ---- CallingConv edge cases ----
TEST(CoverageBoostTest, CallingConvReturnVoid) {
    auto tm = TargetMachine::createX86_64();
    auto& cc = tm->getCallingConv();
    auto ret = cc.analyzeReturn(Type::getVoidTy());
    EXPECT_EQ(ret.size(), 0u);
}

TEST(CoverageBoostTest, CallingConvReturnInt64) {
    auto tm = TargetMachine::createX86_64();
    auto& cc = tm->getCallingConv();
    auto ret = cc.analyzeReturn(Type::getInt64Ty());
    EXPECT_EQ(ret.size(), 1u);
}

// ---- Emit Global Data ----
TEST(CoverageBoostTest, AsmPrinterEmitGlobals) {
    auto mod = std::make_unique<Module>("em");
    auto* gv = mod->createGlobal(Type::getInt64Ty(), "counter");
    gv->setInitializer(ConstantInt::getInt64(99));

    auto tm = TargetMachine::createX86_64();
    std::ostringstream oss;
    AsmTextStreamer s(oss);
    X86AsmPrinter printer(s, static_cast<const X86RegisterInfo&>(tm->getRegisterInfo()));
    printer.emitGlobals(*mod);

    std::string out = oss.str();
    EXPECT_NE(out.find(".data"), std::string::npos);
    EXPECT_NE(out.find("counter"), std::string::npos);
}

// ---- MCStreamer edge cases ----
TEST(CoverageBoostTest, MCStreamerEmitAll) {
    std::ostringstream oss;
    AsmTextStreamer s(oss);
    s.emitLabel("test");
    s.emitComment("hello");
    s.emitGlobalSymbol("func");
    s.emitAlignment(8);
    s.emitRawText("\tnop");
    std::string out = oss.str();
    EXPECT_NE(out.find("test:"), std::string::npos);
    EXPECT_NE(out.find("# hello"), std::string::npos);
    EXPECT_NE(out.find(".globl func"), std::string::npos);
}

// ---- MachineOperand edge cases ----
TEST(CoverageBoostTest, MachineOperandDefault) {
    MachineOperand mo;
    EXPECT_EQ(mo.getKind(), MachineOperandKind::MO_None);
}
