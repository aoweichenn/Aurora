#include <gtest/gtest.h>
#include "Aurora/Target/X86/X86CallingConv.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(X86CallingConvTest, AnalyzeReturnInt32) {
    X86CallingConv cc;
    auto assigns = cc.analyzeReturn(Type::getInt32Ty());
    ASSERT_EQ(assigns.size(), 1u);
    EXPECT_EQ(assigns[0].loc, CCValAssign::GPR);
    EXPECT_EQ(assigns[0].regId, X86RegisterInfo::RAX);
}

TEST(X86CallingConvTest, AnalyzeReturnInt64) {
    X86CallingConv cc;
    auto assigns = cc.analyzeReturn(Type::getInt64Ty());
    ASSERT_EQ(assigns.size(), 1u);
    EXPECT_EQ(assigns[0].regId, X86RegisterInfo::RAX);
}

TEST(X86CallingConvTest, AnalyzeReturnVoid) {
    X86CallingConv cc;
    auto assigns = cc.analyzeReturn(Type::getVoidTy());
    EXPECT_TRUE(assigns.empty());
}

TEST(X86CallingConvTest, AnalyzeArgumentsSingleInt) {
    X86CallingConv cc;
    Type* args[] = {Type::getInt32Ty()};
    auto assigns = cc.analyzeArguments(args, 1);
    ASSERT_EQ(assigns.size(), 1u);
    EXPECT_EQ(assigns[0].loc, CCValAssign::GPR);
    EXPECT_EQ(assigns[0].regId, X86RegisterInfo::RDI);
}

TEST(X86CallingConvTest, AnalyzeArgumentsTwoInts) {
    X86CallingConv cc;
    Type* args[] = {Type::getInt32Ty(), Type::getInt64Ty()};
    auto assigns = cc.analyzeArguments(args, 2);
    ASSERT_EQ(assigns.size(), 2u);
    EXPECT_EQ(assigns[0].regId, X86RegisterInfo::RDI);
    EXPECT_EQ(assigns[1].regId, X86RegisterInfo::RSI);
}

TEST(X86CallingConvTest, AnalyzeArgumentsSixInts) {
    X86CallingConv cc;
    Type* args[6] = {Type::getInt32Ty(), Type::getInt32Ty(), Type::getInt32Ty(),
                     Type::getInt32Ty(), Type::getInt32Ty(), Type::getInt32Ty()};
    auto assigns = cc.analyzeArguments(args, 6);
    ASSERT_EQ(assigns.size(), 6u);
    // All 6 should use GPR registers (RDI, RSI, RDX, RCX, R8, R9)
    EXPECT_EQ(assigns[0].regId, X86RegisterInfo::RDI);
    EXPECT_EQ(assigns[1].regId, X86RegisterInfo::RSI);
    EXPECT_EQ(assigns[2].regId, X86RegisterInfo::RDX);
    EXPECT_EQ(assigns[3].regId, X86RegisterInfo::RCX);
    EXPECT_EQ(assigns[4].regId, X86RegisterInfo::R8);
    EXPECT_EQ(assigns[5].regId, X86RegisterInfo::R9);
}

TEST(X86CallingConvTest, AnalyzeArgumentsOverflowToStack) {
    X86CallingConv cc;
    Type* args[8];
    for (int i = 0; i < 8; ++i) args[i] = Type::getInt64Ty();
    auto assigns = cc.analyzeArguments(args, 8);
    ASSERT_EQ(assigns.size(), 8u);
    // Args 6 and 7 should be on stack
    EXPECT_EQ(assigns[6].loc, CCValAssign::Stack);
    EXPECT_EQ(assigns[7].loc, CCValAssign::Stack);
}

TEST(X86CallingConvTest, StackAlignment) {
    X86CallingConv cc;
    EXPECT_EQ(cc.getStackAlignment(), 16u);
}

TEST(X86CallingConvTest, ShadowStoreSize) {
    X86CallingConv cc;
    EXPECT_EQ(cc.getShadowStoreSize(), 0u);
}
