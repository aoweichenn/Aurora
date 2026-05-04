#include <gtest/gtest.h>
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"

using namespace aurora;

TEST(ToStringCoverageTest, AllOpcodes) {
    for (int i = 0; i <= 31; i++) {
        auto op = static_cast<AIROpcode>(i);
        const char* n = opcodeName(op);
        EXPECT_STRNE(n, "");
    }
}

TEST(ToStringCoverageTest, AllICmpNames) {
    for (int i = 0; i <= 9; i++) {
        auto c = static_cast<ICmpCond>(i);
        const char* n = icmpCondName(c);
        EXPECT_STRNE(n, "");
    }
}

TEST(ToStringCoverageTest, InstructionToString) {
    auto* i32 = Type::getInt32Ty();
    auto* inst = AIRInstruction::createAdd(i32, 0, 1);
    inst->setDestVReg(2);
    std::string s = inst->toString();
    EXPECT_NE(s.find("add"), std::string::npos);

    auto* ret = AIRInstruction::createRet(0);
    EXPECT_NE(ret->toString().find("ret"), std::string::npos);

    auto* store = AIRInstruction::createStore(0, 1);
    EXPECT_NE(store->toString().find("store"), std::string::npos);

    BasicBlock bb("t");
    auto* br = AIRInstruction::createBr(&bb);
    EXPECT_NE(br->toString().find("br"), std::string::npos);
}

TEST(ToStringCoverageTest, InstructionToStringAllOperandForms) {
    auto* i64 = Type::getInt64Ty();
    BasicBlock trueBB("true");
    BasicBlock falseBB("false");
    BasicBlock mergeBB("merge");
    SmallVector<Type*, 8> params = {i64};
    Function callee(new FunctionType(i64, params), "callee");

    auto* condBr = AIRInstruction::createCondBr(3, &trueBB, &falseBB);
    EXPECT_NE(condBr->toString().find("condbr %3, &true, &false"), std::string::npos);

    auto* icmp = AIRInstruction::createICmp(i64, ICmpCond::UGE, 1, 2);
    icmp->setDestVReg(4);
    EXPECT_NE(icmp->toString().find("icmp uge %1, %2"), std::string::npos);

    auto* alloca = AIRInstruction::createAlloca(i64);
    alloca->setDestVReg(5);
    EXPECT_NE(alloca->toString().find("alloca i64*"), std::string::npos);

    auto* load = AIRInstruction::createLoad(i64, 5);
    load->setDestVReg(6);
    EXPECT_NE(load->toString().find("load i64 %5"), std::string::npos);

    SmallVector<unsigned, 4> indices = {0, 2};
    auto* gep = AIRInstruction::createGEP(Type::getPointerTy(i64), 6, indices);
    gep->setDestVReg(7);
    EXPECT_NE(gep->toString().find("gep %6, %0, %2"), std::string::npos);

    SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings = {{&trueBB, 8}, {&falseBB, 9}};
    auto* phi = AIRInstruction::createPhi(i64, incomings);
    phi->setDestVReg(10);
    EXPECT_NE(phi->toString().find("[&true, %8]"), std::string::npos);
    EXPECT_EQ(phi->getOperandBlock(0), &trueBB);
    phi->replaceUse(8, 11);
    EXPECT_NE(phi->toString().find("[&true, %11]"), std::string::npos);

    SmallVector<unsigned, 8> args = {1, 2};
    auto* call = AIRInstruction::createCall(i64, &callee, args);
    call->setDestVReg(12);
    EXPECT_NE(call->toString().find("call callee %1 %2"), std::string::npos);

    auto* indirectCall = AIRInstruction::createCall(i64, nullptr, args);
    indirectCall->setDestVReg(13);
    EXPECT_NE(indirectCall->toString().find("call ? %1 %2"), std::string::npos);

    auto* global = AIRInstruction::createGlobalAddress(Type::getPointerTy(i64), "g");
    global->setDestVReg(14);
    EXPECT_NE(global->toString().find("globaladdr @g"), std::string::npos);

    auto* unnamedGlobal = AIRInstruction::createGlobalAddress(Type::getPointerTy(i64), nullptr);
    unnamedGlobal->setDestVReg(15);
    EXPECT_NE(unnamedGlobal->toString().find("globaladdr @?"), std::string::npos);

    SmallVector<std::pair<int64_t, BasicBlock*>, 8> cases = {{1, &trueBB}};
    auto* switchInst = AIRInstruction::createSwitch(Type::getVoidTy(), 3, &falseBB, cases);
    switchInst->setDestVReg(16);
    EXPECT_NE(switchInst->toString().find("switch"), std::string::npos);

    EXPECT_EQ(AIRInstruction::createAdd(i64, 1, 2)->getOperandBlock(0), nullptr);
}

TEST(ToStringCoverageTest, ExplicitOpcodeAndConditionFallbacks) {
    EXPECT_STREQ(opcodeName(AIROpcode::ConstantInt), "const");
    EXPECT_STREQ(opcodeName(AIROpcode::Switch), "switch");
    EXPECT_STREQ(opcodeName(AIROpcode::GlobalAddress), "globaladdr");
    EXPECT_STREQ(opcodeName(AIROpcode::ExtractValue), "extractvalue");
    EXPECT_STREQ(opcodeName(AIROpcode::InsertValue), "insertvalue");
    EXPECT_STREQ(opcodeName(static_cast<AIROpcode>(999)), "unknown");
    EXPECT_STREQ(icmpCondName(static_cast<ICmpCond>(99)), "?");
}

TEST(ToStringCoverageTest, TypeToStringAll) {
    EXPECT_EQ(Type::getVoidTy()->toString(), "void");
    EXPECT_EQ(Type::getInt1Ty()->toString(), "i1");
    EXPECT_EQ(Type::getInt8Ty()->toString(), "i8");
    EXPECT_EQ(Type::getInt16Ty()->toString(), "i16");
    EXPECT_EQ(Type::getInt32Ty()->toString(), "i32");
    EXPECT_EQ(Type::getInt64Ty()->toString(), "i64");
    EXPECT_EQ(Type::getFloatTy()->toString(), "float");
    EXPECT_EQ(Type::getDoubleTy()->toString(), "double");
    EXPECT_NE(Type::getPointerTy(Type::getInt32Ty())->toString(), "");
    EXPECT_NE(Type::getArrayTy(Type::getInt32Ty(), 10)->toString(), "");
}

TEST(ToStringCoverageTest, TypeToStringDerivedAndFallbacks) {
    SmallVector<Type*, 8> members = {Type::getInt8Ty(), Type::getInt64Ty()};
    Type* structTy = Type::getStructTy(members);
    EXPECT_EQ(structTy->toString(), "{i8, i64}");
    EXPECT_EQ(structTy->getStructMembers().size(), 2u);
    EXPECT_GT(structTy->getMemberOffset(1), 0u);
    EXPECT_TRUE(Type::getInt32Ty()->getStructMembers().empty());
    EXPECT_EQ(Type::getInt32Ty()->getMemberOffset(0), 0u);

    SmallVector<Type*, 8> fnParams = {Type::getInt32Ty(), Type::getDoubleTy()};
    Type* fnTy = Type::getFunctionTy(Type::getVoidTy(), fnParams, true);
    EXPECT_EQ(fnTy->toString(), "void (i32, double)");
    EXPECT_EQ(fnTy->getParamTypes().size(), 2u);
    EXPECT_TRUE(fnTy->isVarArg());
    EXPECT_TRUE(Type::getInt64Ty()->getParamTypes().empty());

    Type opaquePointer(TypeKind::Pointer, nullptr, 0, 0);
    EXPECT_EQ(opaquePointer.toString(), "ptr");
    Type oddInteger(TypeKind::Integer, 3, 3);
    EXPECT_EQ(oddInteger.toString(), "i3");
    Type unknown(static_cast<TypeKind>(99), 0, 0);
    EXPECT_EQ(unknown.toString(), "unknown");
}

TEST(ToStringCoverageTest, DataLayoutAll) {
    DataLayout dl;
    EXPECT_TRUE(dl.isLittleEndian());
    dl.setLittleEndian(false);
    EXPECT_FALSE(dl.isLittleEndian());
    dl.setLittleEndian(true);
    EXPECT_TRUE(dl.isLittleEndian());
    EXPECT_EQ(dl.getPointerSize(), 64u);
    dl.setPointerSize(32);
    EXPECT_EQ(dl.getPointerSize(), 32u);
}
