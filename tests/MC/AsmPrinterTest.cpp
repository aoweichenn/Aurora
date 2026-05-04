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

TEST(X86AsmPrinterTest, EmitMemoryAndGlobalOperands) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    X86AsmPrinter printer(streamer, ri);

    MachineInstr load(X86::MOV64rm);
    load.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    load.addOperand(MachineOperand::createFrameIndex(0));
    printer.emitInstruction(load);

    MachineInstr movGlobal(X86::MOV64ri32);
    movGlobal.addOperand(MachineOperand::createGlobalSym("global_value"));
    movGlobal.addOperand(MachineOperand::createReg(X86RegisterInfo::RBX));
    printer.emitInstruction(movGlobal);

    MachineInstr callGlobal(X86::CALL64pcrel32);
    callGlobal.addOperand(MachineOperand::createGlobalSym("callee"));
    printer.emitInstruction(callGlobal);

    const std::string out = oss.str();
    EXPECT_NE(out.find("movq\t-8(%rbp), %rax"), std::string::npos);
    EXPECT_NE(out.find("$global_value"), std::string::npos);
    EXPECT_NE(out.find("call\tcallee"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitImmediateAndShiftVariants) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    X86AsmPrinter printer(streamer, ri);

    MachineInstr shlImm(X86::SHL64ri);
    shlImm.addOperand(MachineOperand::createImm(3));
    shlImm.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    printer.emitInstruction(shlImm);

    MachineInstr andImm(X86::AND64ri32);
    andImm.addOperand(MachineOperand::createImm(255));
    andImm.addOperand(MachineOperand::createReg(X86RegisterInfo::RCX));
    printer.emitInstruction(andImm);

    MachineInstr orImm(X86::OR64ri32);
    orImm.addOperand(MachineOperand::createImm(1));
    orImm.addOperand(MachineOperand::createReg(X86RegisterInfo::RDX));
    printer.emitInstruction(orImm);

    MachineInstr xorImm(X86::XOR64ri32);
    xorImm.addOperand(MachineOperand::createImm(7));
    xorImm.addOperand(MachineOperand::createReg(X86RegisterInfo::RBX));
    printer.emitInstruction(xorImm);

    MachineInstr cmpImm(X86::CMP64ri32);
    cmpImm.addOperand(MachineOperand::createImm(0));
    cmpImm.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    printer.emitInstruction(cmpImm);

    MachineInstr sar(X86::SAR64rCL);
    sar.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    printer.emitInstruction(sar);

    MachineInstr shr(X86::SHR64rCL);
    shr.addOperand(MachineOperand::createReg(X86RegisterInfo::RCX));
    printer.emitInstruction(shr);

    const std::string out = oss.str();
    EXPECT_NE(out.find("shlq\t$3, %rax"), std::string::npos);
    EXPECT_NE(out.find("andq\t$255, %rcx"), std::string::npos);
    EXPECT_NE(out.find("orq\t$1, %rdx"), std::string::npos);
    EXPECT_NE(out.find("xorq\t$7, %rbx"), std::string::npos);
    EXPECT_NE(out.find("cmpq\t$0, %rax"), std::string::npos);
    EXPECT_NE(out.find("sarq\t%cl, %rax"), std::string::npos);
    EXPECT_NE(out.find("shrq\t%cl, %rcx"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitConversionAndFloatingPointVariants) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    X86AsmPrinter printer(streamer, ri);

    MachineInstr movsx(X86::MOVSX64rr32);
    movsx.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    movsx.addOperand(MachineOperand::createReg(X86RegisterInfo::RCX));
    printer.emitInstruction(movsx);

    MachineInstr idiv(X86::IDIV64r);
    idiv.addOperand(MachineOperand::createReg(X86RegisterInfo::RBX));
    printer.emitInstruction(idiv);

    for (uint16_t opcode : {X86::ADDSDrr, X86::SUBSDrr, X86::MULSDrr, X86::DIVSDrr,
                            X86::UCOMISDrr, X86::CVTSI2SDrr, X86::CVTTSD2SIrr, X86::XOR32rr}) {
        MachineInstr mi(opcode);
        mi.addOperand(MachineOperand::createReg(X86RegisterInfo::XMM0));
        mi.addOperand(MachineOperand::createReg(X86RegisterInfo::XMM1));
        printer.emitInstruction(mi);
    }

    const std::string out = oss.str();
    EXPECT_NE(out.find("movslq\t%rax, %rcx"), std::string::npos);
    EXPECT_NE(out.find("idivq\t%rbx"), std::string::npos);
    EXPECT_NE(out.find("addsd"), std::string::npos);
    EXPECT_NE(out.find("subsd"), std::string::npos);
    EXPECT_NE(out.find("mulsd"), std::string::npos);
    EXPECT_NE(out.find("divsd"), std::string::npos);
    EXPECT_NE(out.find("ucomisd"), std::string::npos);
    EXPECT_NE(out.find("cvtsi2sd"), std::string::npos);
    EXPECT_NE(out.find("cvttsd2si"), std::string::npos);
    EXPECT_NE(out.find("xorl"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitUnsignedBranchesAndSetccVariants) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    X86AsmPrinter printer(streamer, ri);
    MachineBasicBlock target("Ltarget");

    for (uint16_t opcode : {X86::JA_1, X86::JB_1, X86::JAE_1, X86::JBE_1}) {
        MachineInstr mi(opcode);
        mi.addOperand(MachineOperand::createMBB(&target));
        printer.emitInstruction(mi);
    }

    for (uint16_t opcode : {X86::SETEr, X86::SETNEr, X86::SETLr, X86::SETGr, X86::SETLEr,
                            X86::SETGEr, X86::SETAr, X86::SETBEr, X86::SETAEr}) {
        MachineInstr mi(opcode);
        mi.addOperand(MachineOperand::createReg(X86RegisterInfo::R8));
        printer.emitInstruction(mi);
    }

    MachineInstr setXmm(X86::SETEr);
    setXmm.addOperand(MachineOperand::createReg(X86RegisterInfo::XMM0));
    printer.emitInstruction(setXmm);

    MachineInstr setImm(X86::SETNEr);
    setImm.addOperand(MachineOperand::createImm(1));
    printer.emitInstruction(setImm);

    const std::string out = oss.str();
    EXPECT_NE(out.find("ja\t.Ltarget"), std::string::npos);
    EXPECT_NE(out.find("jb\t.Ltarget"), std::string::npos);
    EXPECT_NE(out.find("jae\t.Ltarget"), std::string::npos);
    EXPECT_NE(out.find("jbe\t.Ltarget"), std::string::npos);
    EXPECT_NE(out.find("sete\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setne\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setl\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setg\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setle\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setge\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("seta\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setbe\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("setae\t%r8b"), std::string::npos);
    EXPECT_NE(out.find("sete\t%xmm0"), std::string::npos);
    EXPECT_NE(out.find("setne\t$1"), std::string::npos);
}

TEST(X86AsmPrinterTest, EmitsDefaultOperandForUnsupportedKind) {
    std::ostringstream oss;
    AsmTextStreamer streamer(oss);
    const X86RegisterInfo ri;
    X86AsmPrinter printer(streamer, ri);

    MachineInstr call(X86::CALL64pcrel32);
    call.addOperand(MachineOperand{});
    printer.emitInstruction(call);

    EXPECT_NE(oss.str().find("call\t?"), std::string::npos);
}
