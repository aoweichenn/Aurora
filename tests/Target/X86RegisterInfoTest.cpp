#include <gtest/gtest.h>
#include "Aurora/Target/X86/X86RegisterInfo.h"

using namespace aurora;

TEST(X86RegisterInfoTest, NumRegs) {
    X86RegisterInfo ri;
    EXPECT_EQ(ri.getNumRegs(), X86RegisterInfo::NUM_REGS);
}

TEST(X86RegisterInfoTest, FramePointer) {
    X86RegisterInfo ri;
    auto fp = ri.getFramePointer();
    EXPECT_EQ(fp.name, "rbp");
    EXPECT_EQ(fp.bitWidth, 64u);
}

TEST(X86RegisterInfoTest, StackPointer) {
    X86RegisterInfo ri;
    auto sp = ri.getStackPointer();
    EXPECT_EQ(sp.name, "rsp");
    EXPECT_EQ(sp.bitWidth, 64u);
}

TEST(X86RegisterInfoTest, CalleeSavedRegs) {
    X86RegisterInfo ri;
    auto cs = ri.getCalleeSavedRegs();
    // RBX, RBP, R12-R15 should be callee-saved
    EXPECT_TRUE(cs.test(X86RegisterInfo::RBX));
    EXPECT_TRUE(cs.test(X86RegisterInfo::RBP));
    EXPECT_TRUE(cs.test(X86RegisterInfo::R12));
    EXPECT_TRUE(cs.test(X86RegisterInfo::R13));
    EXPECT_TRUE(cs.test(X86RegisterInfo::R14));
    EXPECT_TRUE(cs.test(X86RegisterInfo::R15));

    // RAX should NOT be callee-saved
    EXPECT_FALSE(cs.test(X86RegisterInfo::RAX));
}

TEST(X86RegisterInfoTest, CallerSavedRegs) {
    X86RegisterInfo ri;
    auto cs = ri.getCallerSavedRegs();
    EXPECT_TRUE(cs.test(X86RegisterInfo::RAX));
    EXPECT_TRUE(cs.test(X86RegisterInfo::RCX));
    EXPECT_TRUE(cs.test(X86RegisterInfo::RDX));
    EXPECT_TRUE(cs.test(X86RegisterInfo::RSI));
    EXPECT_TRUE(cs.test(X86RegisterInfo::RDI));
    EXPECT_TRUE(cs.test(X86RegisterInfo::R8));
}

TEST(X86RegisterInfoTest, RegisterClasses) {
    X86RegisterInfo ri;
    const auto& gpr64 = ri.getRegClass(RegClass::GPR64);
    EXPECT_EQ(gpr64.getNumRegs(), 16u);
    EXPECT_EQ(gpr64.getName(), "GPR64");

    const auto& gpr32 = ri.getRegClass(RegClass::GPR32);
    EXPECT_EQ(gpr32.getNumRegs(), 16u);

    const auto& xmm = ri.getRegClass(RegClass::XMM128);
    EXPECT_EQ(xmm.getNumRegs(), 16u);
}

TEST(X86RegisterInfoTest, AllocationOrder) {
    X86RegisterInfo ri;
    auto order = ri.getAllocOrder(RegClass::GPR64);
    EXPECT_FALSE(order.empty());
    // RSP should not be early in allocation order
}

TEST(X86RegisterInfoTest, GetRegById) {
    auto reg = X86RegisterInfo::getReg(X86RegisterInfo::RAX);
    EXPECT_EQ(reg.name, "rax");
    EXPECT_TRUE(reg.isGeneralPurpose());
}

TEST(X86RegisterInfoTest, GetXMMById) {
    auto reg = X86RegisterInfo::getReg(X86RegisterInfo::XMM0);
    EXPECT_EQ(reg.name, "xmm0");
    EXPECT_TRUE(reg.isFloatingPoint());
}
