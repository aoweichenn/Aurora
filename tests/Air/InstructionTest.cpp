#include <gtest/gtest.h>
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(InstructionTest, CreateAdd) {
    auto* ty = Type::getInt32Ty();
    const auto* inst = AIRInstruction::createAdd(ty, 0, 1);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Add);
    EXPECT_EQ(inst->getType(), ty);
    EXPECT_TRUE(inst->hasResult());
    EXPECT_EQ(inst->getNumOperands(), 2u);
}

TEST(InstructionTest, CreateSub) {
    const auto* inst = AIRInstruction::createSub(Type::getInt64Ty(), 2, 3);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Sub);
    EXPECT_EQ(inst->getNumOperands(), 2u);
}

TEST(InstructionTest, CreateBinaryOps) {
    auto* ty = Type::getInt32Ty();
    EXPECT_EQ(AIRInstruction::createAdd(ty, 0, 1)->getOpcode(), AIROpcode::Add);
    EXPECT_EQ(AIRInstruction::createSub(ty, 0, 1)->getOpcode(), AIROpcode::Sub);
    EXPECT_EQ(AIRInstruction::createMul(ty, 0, 1)->getOpcode(), AIROpcode::Mul);
    EXPECT_EQ(AIRInstruction::createAnd(ty, 0, 1)->getOpcode(), AIROpcode::And);
    EXPECT_EQ(AIRInstruction::createOr(ty, 0, 1)->getOpcode(),  AIROpcode::Or);
    EXPECT_EQ(AIRInstruction::createXor(ty, 0, 1)->getOpcode(), AIROpcode::Xor);
}

TEST(InstructionTest, CreateRet) {
    const auto* inst = AIRInstruction::createRet(5);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Ret);
    EXPECT_FALSE(inst->hasResult());
    EXPECT_EQ(inst->getNumOperands(), 1u);
}

TEST(InstructionTest, CreateRetVoid) {
    const auto* inst = AIRInstruction::createRet(~0U);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Ret);
    EXPECT_EQ(inst->getNumOperands(), 0u);
}

TEST(InstructionTest, CreateBr) {
    BasicBlock bb("target");
    const auto* inst = AIRInstruction::createBr(&bb);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Br);
    EXPECT_FALSE(inst->hasResult());
    EXPECT_EQ(inst->getOperandBlock(0), &bb);
}

TEST(InstructionTest, CreateCondBr) {
    BasicBlock trueBB("true"), falseBB("false");
    const auto* inst = AIRInstruction::createCondBr(0, &trueBB, &falseBB);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::CondBr);
    EXPECT_EQ(inst->getNumOperands(), 1u);
    EXPECT_EQ(inst->getOperandBlock(0), &trueBB);
    EXPECT_EQ(inst->getOperandBlock(1), &falseBB);
}

TEST(InstructionTest, CreateICmp) {
    const auto* inst = AIRInstruction::createICmp(Type::getInt1Ty(), ICmpCond::EQ, 0, 1);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::ICmp);
    EXPECT_EQ(inst->getICmpCondition(), ICmpCond::EQ);
}

TEST(InstructionTest, IcmpConditions) {
    for (auto c : {ICmpCond::EQ, ICmpCond::NE, ICmpCond::UGT, ICmpCond::SGT, ICmpCond::SLE}) {
        const auto* inst = AIRInstruction::createICmp(Type::getInt1Ty(), c, 0, 0);
        EXPECT_EQ(inst->getICmpCondition(), c);
    }
}

TEST(InstructionTest, CreateAlloca) {
    const auto* inst = AIRInstruction::createAlloca(Type::getInt32Ty());
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Alloca);
    EXPECT_TRUE(inst->hasResult());
}

TEST(InstructionTest, CreateLoad) {
    const auto* inst = AIRInstruction::createLoad(Type::getInt32Ty(), 1);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Load);
    EXPECT_TRUE(inst->hasResult());
    EXPECT_EQ(inst->getNumOperands(), 1u);
}

TEST(InstructionTest, CreateStore) {
    const auto* inst = AIRInstruction::createStore(0, 1);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Store);
    EXPECT_FALSE(inst->hasResult());
}

TEST(InstructionTest, CreatePhi) {
    BasicBlock bb1("bb1"), bb2("bb2");
    SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings;
    incomings.push_back({&bb1, 0});
    incomings.push_back({&bb2, 1});
    const auto* inst = AIRInstruction::createPhi(Type::getInt32Ty(), incomings);
    EXPECT_EQ(inst->getOpcode(), AIROpcode::Phi);
    EXPECT_EQ(inst->getPhiIncomings().size(), 2u);
}

TEST(InstructionTest, DestVReg) {
    auto* inst = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    inst->setDestVReg(42);
    EXPECT_EQ(inst->getDestVReg(), 42u);
}

TEST(InstructionTest, ReplaceUse) {
    auto* inst = AIRInstruction::createAdd(Type::getInt32Ty(), 5, 6);
    inst->replaceUse(5, 10);
    EXPECT_EQ(inst->getOperand(0), 10u);
    EXPECT_EQ(inst->getOperand(1), 6u);
}

TEST(InstructionTest, OpcodeNames) {
    EXPECT_STREQ(opcodeName(AIROpcode::Add), "add");
    EXPECT_STREQ(opcodeName(AIROpcode::Sub), "sub");
    EXPECT_STREQ(opcodeName(AIROpcode::ICmp), "icmp");
    EXPECT_STREQ(opcodeName(AIROpcode::Load), "load");
    EXPECT_STREQ(opcodeName(AIROpcode::Store), "store");
    EXPECT_STREQ(opcodeName(AIROpcode::Ret), "ret");
    EXPECT_STREQ(opcodeName(AIROpcode::Br), "br");
    EXPECT_STREQ(opcodeName(AIROpcode::CondBr), "condbr");
    EXPECT_STREQ(opcodeName(AIROpcode::Phi), "phi");
    EXPECT_STREQ(opcodeName(AIROpcode::Call), "call");
}

TEST(InstructionTest, HasResult) {
    EXPECT_TRUE(AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1)->hasResult());
    EXPECT_FALSE(AIRInstruction::createRet(0)->hasResult());
    EXPECT_FALSE(AIRInstruction::createBr(nullptr)->hasResult());
    EXPECT_FALSE(AIRInstruction::createStore(0, 1)->hasResult());
}
