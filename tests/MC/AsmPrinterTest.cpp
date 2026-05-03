#include <gtest/gtest.h>
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86AsmPrinter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include <sstream>

using namespace aurora;

TEST(MCStreamerTest, EmitLabel) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    streamer.emitLabel("test_label");
    EXPECT_EQ(oss.str(), "test_label:\n");
}

TEST(MCStreamerTest, EmitComment) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    streamer.emitComment("this is a comment");
    EXPECT_EQ(oss.str(), "# this is a comment\n");
}

TEST(MCStreamerTest, EmitGlobalSymbol) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    streamer.emitGlobalSymbol("main");
    EXPECT_EQ(oss.str(), ".globl main\n");
}

TEST(MCStreamerTest, EmitAlignment) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    streamer.emitAlignment(16);
    EXPECT_EQ(oss.str(), ".align 16\n");
}

TEST(MCStreamerTest, EmitRawText) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    streamer.emitRawText("\tpushq %rbp");
    EXPECT_EQ(oss.str(), "\tpushq %rbp\n");
}

TEST(X86AsmPrinterTest, Construction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    X86AsmPrinter printer(streamer, ri);
    SUCCEED();
}

TEST(X86AsmPrinterTest, EmitMoveInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;

    MachineInstr mi(X86::MOV64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));

    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);

    std::string out = oss.str();
    EXPECT_NE(out.find("movq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitAddInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;

    MachineInstr mi(X86::ADD64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));

    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);

    std::string out = oss.str();
    EXPECT_NE(out.find("addq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitReturnInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;

    const MachineInstr mi(X86::RETQ);
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);

    std::string out = oss.str();
    EXPECT_NE(out.find("ret"), std::string::npos);
}
