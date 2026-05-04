#include <gtest/gtest.h>
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

// Test ALL factory methods
TEST(InstructionFactoryTest, AllTerminators) {
    auto* r = AIRInstruction::createRet(5);
    EXPECT_EQ(r->getOpcode(), AIROpcode::Ret);
    EXPECT_EQ(r->getNumOperands(), 1u);
    EXPECT_FALSE(r->hasResult());

    auto* rv = AIRInstruction::createRet(~0U);
    EXPECT_EQ(rv->getNumOperands(), 0u);

    BasicBlock bb("t");
    auto* br = AIRInstruction::createBr(&bb);
    EXPECT_EQ(br->getOpcode(), AIROpcode::Br);
    EXPECT_EQ(br->getNumOperands(), 0u);

    BasicBlock t("tt"), f("ff");
    auto* cbr = AIRInstruction::createCondBr(0, &t, &f);
    EXPECT_EQ(cbr->getOpcode(), AIROpcode::CondBr);
    EXPECT_EQ(cbr->getNumOperands(), 1u);

    auto* u = AIRInstruction::createUnreachable();
    EXPECT_EQ(u->getOpcode(), AIROpcode::Unreachable);
}

TEST(InstructionFactoryTest, AllBinaryArithmetic) {
    auto* ty = Type::getInt64Ty();
    EXPECT_EQ(AIRInstruction::createAdd(ty, 0, 1)->getOpcode(), AIROpcode::Add);
    EXPECT_EQ(AIRInstruction::createSub(ty, 0, 1)->getOpcode(), AIROpcode::Sub);
    EXPECT_EQ(AIRInstruction::createMul(ty, 0, 1)->getOpcode(), AIROpcode::Mul);
    EXPECT_EQ(AIRInstruction::createUDiv(ty, 0, 1)->getOpcode(), AIROpcode::UDiv);
    EXPECT_EQ(AIRInstruction::createSDiv(ty, 0, 1)->getOpcode(), AIROpcode::SDiv);
    EXPECT_EQ(AIRInstruction::createURem(ty, 0, 1)->getOpcode(), AIROpcode::URem);
    EXPECT_EQ(AIRInstruction::createSRem(ty, 0, 1)->getOpcode(), AIROpcode::SRem);
}

TEST(InstructionFactoryTest, AllBitwiseAndShift) {
    auto* ty = Type::getInt64Ty();
    EXPECT_EQ(AIRInstruction::createAnd(ty, 0, 1)->getOpcode(), AIROpcode::And);
    EXPECT_EQ(AIRInstruction::createOr(ty, 0, 1)->getOpcode(), AIROpcode::Or);
    EXPECT_EQ(AIRInstruction::createXor(ty, 0, 1)->getOpcode(), AIROpcode::Xor);
    EXPECT_EQ(AIRInstruction::createShl(ty, 0, 1)->getOpcode(), AIROpcode::Shl);
    EXPECT_EQ(AIRInstruction::createLShr(ty, 0, 1)->getOpcode(), AIROpcode::LShr);
    EXPECT_EQ(AIRInstruction::createAShr(ty, 0, 1)->getOpcode(), AIROpcode::AShr);
}

TEST(InstructionFactoryTest, AllComparisons) {
    auto* ty = Type::getInt1Ty();
    for (auto c : {ICmpCond::EQ, ICmpCond::NE, ICmpCond::SLT, ICmpCond::SLE,
                   ICmpCond::SGT, ICmpCond::SGE, ICmpCond::ULT, ICmpCond::ULE,
                   ICmpCond::UGT, ICmpCond::UGE}) {
        auto* i = AIRInstruction::createICmp(ty, c, 0, 1);
        EXPECT_EQ(i->getOpcode(), AIROpcode::ICmp);
        EXPECT_EQ(i->getICmpCondition(), c);
    }
}

TEST(InstructionFactoryTest, AllMemory) {
    auto* i32 = Type::getInt32Ty();
    EXPECT_EQ(AIRInstruction::createAlloca(i32)->getOpcode(), AIROpcode::Alloca);
    EXPECT_EQ(AIRInstruction::createLoad(i32, 42)->getOpcode(), AIROpcode::Load);
    EXPECT_EQ(AIRInstruction::createStore(0, 1)->getOpcode(), AIROpcode::Store);

    SmallVector<unsigned,4> idx = {0u, 1u};
    auto* gep = AIRInstruction::createGEP(Type::getPointerTy(i32), 42, idx);
    EXPECT_EQ(gep->getOpcode(), AIROpcode::GetElementPtr);
    EXPECT_EQ(gep->getIndices().size(), 2u);
}

TEST(InstructionFactoryTest, AllConversions) {
    EXPECT_EQ(AIRInstruction::createSExt(Type::getInt64Ty(), 0)->getOpcode(), AIROpcode::SExt);
    EXPECT_EQ(AIRInstruction::createZExt(Type::getInt64Ty(), 0)->getOpcode(), AIROpcode::ZExt);
    EXPECT_EQ(AIRInstruction::createTrunc(Type::getInt32Ty(), 0)->getOpcode(), AIROpcode::Trunc);
    // FpToSi/SiToFp/BitCast tested via create instructions
}

TEST(InstructionFactoryTest, PhiSelectCall) {
    BasicBlock b1("b1"), b2("b2");
    SmallVector<std::pair<BasicBlock*, unsigned>,4> inc = {{&b1, 0}, {&b2, 1}};
    auto* phi = AIRInstruction::createPhi(Type::getInt32Ty(), inc);
    EXPECT_EQ(phi->getOpcode(), AIROpcode::Phi);
    EXPECT_EQ(phi->getPhiIncomings().size(), 2u);

    auto* sel = AIRInstruction::createSelect(Type::getInt64Ty(), 0, 1, 2);
    EXPECT_EQ(sel->getOpcode(), AIROpcode::Select);

    SmallVector<unsigned,8> args;
    auto* call = AIRInstruction::createCall(Type::getInt32Ty(), nullptr, args);
    EXPECT_EQ(call->getOpcode(), AIROpcode::Call);
}

TEST(InstructionFactoryTest, ConstantIntOp) {
    auto* c = AIRInstruction::createConstantInt(Type::getInt64Ty(), 42);
    EXPECT_EQ(c->getOpcode(), AIROpcode::ConstantInt);
    EXPECT_EQ(c->getConstantValue(), 42);
    EXPECT_EQ(c->getType(), Type::getInt64Ty());
}

TEST(InstructionFactoryTest, DestVRegAndReplaceUse) {
    auto* i = AIRInstruction::createAdd(Type::getInt32Ty(), 5, 6);
    i->setDestVReg(42);
    EXPECT_EQ(i->getDestVReg(), 42u);
    i->replaceUse(5, 100);
    EXPECT_EQ(i->getOperand(0), 100u);
    EXPECT_EQ(i->getOperand(1), 6u);
}

TEST(InstructionFactoryTest, HasResult) {
    EXPECT_TRUE(AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1)->hasResult());
    EXPECT_TRUE(AIRInstruction::createSub(Type::getInt32Ty(), 0, 1)->hasResult());
    EXPECT_TRUE(AIRInstruction::createAlloca(Type::getInt32Ty())->hasResult());
    EXPECT_TRUE(AIRInstruction::createLoad(Type::getInt32Ty(), 0)->hasResult());
    EXPECT_TRUE(AIRInstruction::createICmp(Type::getInt1Ty(), ICmpCond::EQ, 0, 1)->hasResult());
    EXPECT_TRUE(AIRInstruction::createConstantInt(Type::getInt64Ty(), 0)->hasResult());
    EXPECT_FALSE(AIRInstruction::createRet(0)->hasResult());
    EXPECT_FALSE(AIRInstruction::createBr(nullptr)->hasResult());
    EXPECT_FALSE(AIRInstruction::createStore(0, 1)->hasResult());
    EXPECT_FALSE(AIRInstruction::createUnreachable()->hasResult());
    EXPECT_FALSE(AIRInstruction::createCondBr(0, nullptr, nullptr)->hasResult());
}

TEST(InstructionFactoryTest, ParentLinkage) {
    auto* i = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    BasicBlock bb("test");
    i->setParent(&bb);
    EXPECT_EQ(i->getParent(), &bb);
}

TEST(InstructionFactoryTest, NextPrevLinks) {
    auto* i1 = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    auto* i2 = AIRInstruction::createSub(Type::getInt32Ty(), 0, 1);
    i1->setNext(i2);
    i2->setPrev(i1);
    EXPECT_EQ(i1->getNext(), i2);
    EXPECT_EQ(i2->getPrev(), i1);
}

TEST(InstructionFactoryTest, ToString) {
    auto* i = AIRInstruction::createAdd(Type::getInt32Ty(), 0, 1);
    i->setDestVReg(2);
    std::string s = i->toString();
    EXPECT_NE(s.find("add"), std::string::npos);
    EXPECT_NE(s.find("i32"), std::string::npos);
}
