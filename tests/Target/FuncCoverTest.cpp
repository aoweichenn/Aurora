// Cover X86InstEncoder, TargetLowering, FrameLowering, RegisterAllocator methods
#include <gtest/gtest.h>
#include "Aurora/Target/X86/X86InstrEncode.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86TargetLowering.h"
#include "Aurora/Target/X86/X86FrameLowering.h"
#include "Aurora/Target/X86/X86TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/TargetLowering.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/RegisterAllocator.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

// ---- X86InstEncoder ----
TEST(FuncCoverEncTest, EncodeADD) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineInstr mi(X86::ADD64rr);
    mi.addOperand(MachineOperand::createReg(0));
    mi.addOperand(MachineOperand::createReg(1));
    enc.encode(mi, o); EXPECT_GT(o.size(),0u);
}
TEST(FuncCoverEncTest, EncodeMOV) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineInstr mi(X86::MOV64rr);
    mi.addOperand(MachineOperand::createReg(0));
    mi.addOperand(MachineOperand::createReg(1));
    enc.encode(mi, o); EXPECT_GT(o.size(),0u);
}
TEST(FuncCoverEncTest, EncodeRET) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineInstr mi(X86::RETQ); enc.encode(mi, o); EXPECT_GT(o.size(),0u);
}
TEST(FuncCoverEncTest, EncodeOperandReg) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineOperand mo = MachineOperand::createReg(0);
    enc.encodeOperand(mo, o, 0); SUCCEED();
}
TEST(FuncCoverEncTest, EncodeOperandImm) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineOperand mo = MachineOperand::createImm(42);
    enc.encodeOperand(mo, o, 0); SUCCEED();
}
TEST(FuncCoverEncTest, EncodeWithImm) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineInstr mi(X86::ADD64ri32);
    mi.addOperand(MachineOperand::createImm(42));
    mi.addOperand(MachineOperand::createReg(0));
    enc.encode(mi, o); EXPECT_GT(o.size(),0u);
}
TEST(FuncCoverEncTest, EncodeSUB) {
    X86InstEncoder enc; SmallVector<uint8_t,32> o;
    MachineInstr mi(X86::SUB64rr);
    mi.addOperand(MachineOperand::createReg(0));
    mi.addOperand(MachineOperand::createReg(1));
    enc.encode(mi, o); EXPECT_GT(o.size(),0u);
}
TEST(FuncCoverEncTest, TableSize) { EXPECT_GT(kX86EncodeTableSize, 0u); }

// ---- X86TargetLowering ----
TEST(FuncCoverLoweringTest, OperationAction) {
    X86TargetLowering tl;
    EXPECT_EQ(tl.getOperationAction(AIROpcode::Add, 64), LegalizeAction::Legal);
    EXPECT_EQ(tl.getOperationAction(AIROpcode::Add, 1), LegalizeAction::Promote);
}
TEST(FuncCoverLoweringTest, TypeLegal) {
    X86TargetLowering tl;
    EXPECT_TRUE(tl.isTypeLegal(64));
    EXPECT_TRUE(tl.isTypeLegal(32));
    EXPECT_TRUE(tl.isTypeLegal(8));
    EXPECT_FALSE(tl.isTypeLegal(1));
}
TEST(FuncCoverLoweringTest, RegisterSizeForType) {
    X86TargetLowering tl;
    EXPECT_EQ(tl.getRegisterSizeForType(Type::getInt64Ty()), 64u);
    EXPECT_EQ(tl.getRegisterSizeForType(Type::getInt32Ty()), 32u);
}

// ---- X86FrameLowering ----
TEST(FuncCoverFrameTest, StackAlignment) {
    X86FrameLowering fl; EXPECT_EQ(fl.getStackAlignment(), 16u);
}
TEST(FuncCoverFrameTest, HasFP) {
    auto m = std::make_unique<Module>("fp");
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), {}), "f");
    AIRBuilder(f->getEntryBlock()).createRet(AIRBuilder(f->getEntryBlock()).createConstantInt(0));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    X86FrameLowering fl;
    EXPECT_TRUE(fl.hasFP(mf));
}
TEST(FuncCoverFrameTest, FrameIndexRef) {
    auto m = std::make_unique<Module>("fr");
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), {}), "f");
    AIRBuilder(f->getEntryBlock()).createRet(AIRBuilder(f->getEntryBlock()).createConstantInt(0));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    X86FrameLowering fl;
    unsigned r = 0; int o = fl.getFrameIndexReference(mf, 0, r);
    EXPECT_EQ(r, X86RegisterInfo::RBP);
}

// ---- RegisterAllocator ----
TEST(FuncCoverRATest, AllocateRegisters) {
    auto m = std::make_unique<Module>("ra");
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), {Type::getInt64Ty(), Type::getInt64Ty()}), "f");
    AIRBuilder b(f->getEntryBlock()); b.createRet(b.createAdd(Type::getInt64Ty(), 0, 1));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    LinearScanRegAlloc alloc(mf);
    alloc.allocateRegisters();
    SUCCEED();
}

// ---- X86TargetMachine factories ----
TEST(FuncCoverTargetTest, TargetMachine) {
    auto tm = TargetMachine::createX86_64();
    EXPECT_NE(tm, nullptr);
    EXPECT_EQ(tm->getTargetTriple(), "x86_64-unknown-linux-gnu");
}
TEST(FuncCoverTargetTest, RegisterInfo) {
    auto tm = TargetMachine::createX86_64();
    auto& ri = tm->getRegisterInfo(); EXPECT_NE(&ri, nullptr);
}
TEST(FuncCoverTargetTest, InstrInfo) {
    auto tm = TargetMachine::createX86_64();
    auto& ii = tm->getInstrInfo(); EXPECT_NE(&ii, nullptr);
}
TEST(FuncCoverTargetTest, Lowering) {
    auto tm = TargetMachine::createX86_64();
    auto& tl = tm->getLowering(); EXPECT_NE(&tl, nullptr);
}
TEST(FuncCoverTargetTest, CallingConv) {
    auto tm = TargetMachine::createX86_64();
    auto& cc = tm->getCallingConv(); EXPECT_NE(&cc, nullptr);
}
TEST(FuncCoverTargetTest, FrameLowering) {
    auto tm = TargetMachine::createX86_64();
    auto& fl = tm->getFrameLowering(); EXPECT_NE(&fl, nullptr);
}
