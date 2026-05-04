#include <gtest/gtest.h>
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/AArch64/AArch64CallingConv.h"
#include "Aurora/Target/AArch64/AArch64InstrInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(AArch64TargetTest, TargetMachineFactory) {
    auto tm = TargetMachine::createAArch64_Apple();
    ASSERT_NE(tm, nullptr);
    EXPECT_STREQ(tm->getTargetTriple(), "arm64-apple-darwin");
    EXPECT_EQ(tm->getDataLayout().getPointerSize(), 64u);
}

TEST(AArch64TargetTest, RegisterInfo) {
    AArch64RegisterInfo ri;
    EXPECT_EQ(ri.getFramePointer().id, AArch64RegisterInfo::FP);
    EXPECT_EQ(ri.getStackPointer().id, AArch64RegisterInfo::SP);
    EXPECT_GT(ri.getAllocOrder(RegClass::GPR64).size(), 0u);
}

TEST(AArch64TargetTest, InstrInfoFlags) {
    AArch64RegisterInfo ri;
    AArch64InstrInfo ii(ri);
    EXPECT_TRUE(ii.get(AArch64::B).isBranch);
    EXPECT_TRUE(ii.get(AArch64::RET).isReturn);
    EXPECT_TRUE(ii.get(AArch64::MOVrr).isMove);
}

TEST(AArch64TargetTest, CallingConvention) {
    AArch64CallingConv cc;
    Type* args[] = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto assigns = cc.analyzeArguments(args, 2);
    ASSERT_EQ(assigns.size(), 2u);
    EXPECT_EQ(assigns[0].regId, AArch64RegisterInfo::X0);
    EXPECT_EQ(assigns[1].regId, AArch64RegisterInfo::X1);
    auto ret = cc.analyzeReturn(Type::getInt64Ty());
    ASSERT_EQ(ret.size(), 1u);
    EXPECT_EQ(ret[0].regId, AArch64RegisterInfo::X0);
}
