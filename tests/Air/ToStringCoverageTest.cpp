#include <gtest/gtest.h>
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Module.h"

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

TEST(ToStringCoverageTest, TypeToStringAll) {
    EXPECT_EQ(Type::getVoidTy()->toString(), "void");
    EXPECT_EQ(Type::getInt8Ty()->toString(), "i8");
    EXPECT_EQ(Type::getInt16Ty()->toString(), "i16");
    EXPECT_EQ(Type::getInt32Ty()->toString(), "i32");
    EXPECT_EQ(Type::getInt64Ty()->toString(), "i64");
    EXPECT_EQ(Type::getFloatTy()->toString(), "float");
    EXPECT_EQ(Type::getDoubleTy()->toString(), "double");
    EXPECT_NE(Type::getPointerTy(Type::getInt32Ty())->toString(), "");
    EXPECT_NE(Type::getArrayTy(Type::getInt32Ty(), 10)->toString(), "");
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
