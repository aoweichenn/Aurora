#include <gtest/gtest.h>
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/MC/X86AsmPrinter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
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

TEST(X86AsmPrinterTest, EmitSubInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::SUB64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("subq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitMulInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::IMUL64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("imulq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitAndInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::AND64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("andq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitOrInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::OR64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("orq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitXorInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::XOR64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("xorq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitCmpInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::CMP64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("cmpq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitTestInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::TEST64rr);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("testq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitCqoInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    const MachineInstr mi(X86::CQO);
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("cqo"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitShlInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::SHL64rCL);
    mi.addOperand(MachineOperand::createVReg(0));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("shlq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitPushInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::PUSH64r);
    mi.addOperand(MachineOperand::createVReg(0));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("pushq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitPopInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::POP64r);
    mi.addOperand(MachineOperand::createVReg(0));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("popq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJeInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Ltrue");
    MachineInstr mi(X86::JE_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("je"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJmpInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Lend");
    MachineInstr mi(X86::JMP_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("jmp"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJgInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Ltrue");
    MachineInstr mi(X86::JG_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("jg"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJgeInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Ltrue");
    MachineInstr mi(X86::JGE_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("jge"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitCallInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock funcLabel(".func");
    MachineInstr mi(X86::CALL64pcrel32);
    mi.addOperand(MachineOperand::createMBB(&funcLabel));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("call"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitMoveImmInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::MOV64ri32);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("movq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitAddImmInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::ADD64ri32);
    mi.addOperand(MachineOperand::createVReg(0));
    mi.addOperand(MachineOperand::createVReg(1));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("addq"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJneInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Lfalse");
    MachineInstr mi(X86::JNE_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("jne"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJlInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Lneg");
    MachineInstr mi(X86::JL_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("jl"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitJleInstruction) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineBasicBlock target(".Ltrue");
    MachineInstr mi(X86::JLE_1);
    mi.addOperand(MachineOperand::createMBB(&target));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("jle"), std::string::npos);
}

TEST(X86AsmPrinterTest, PrintPhysicalRegister) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::MOV64rr);
    mi.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    mi.addOperand(MachineOperand::createReg(X86RegisterInfo::RBX));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    std::string out = oss.str();
    EXPECT_NE(out.find("%rax"), std::string::npos);
    EXPECT_NE(out.find("%rbx"), std::string::npos);
}

TEST(X86AsmPrinterTest, PrintImmediateOperand) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(X86::ADD64ri32);
    mi.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    mi.addOperand(MachineOperand::createImm(42));
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("$42"), std::string::npos);
}

TEST(X86AsmPrinterTest, UnknownOpcode) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    MachineInstr mi(9999);
    X86AsmPrinter printer(streamer, ri);
    printer.emitInstruction(mi);
    EXPECT_NE(oss.str().find("unknown opcode"), std::string::npos);
}
