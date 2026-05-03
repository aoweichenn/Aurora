#include <gtest/gtest.h>
#include "Aurora/Air/Constant.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(ConstantIntTest, GetInt1True) {
    auto* ci = ConstantInt::getInt1(true);
    ASSERT_NE(ci, nullptr);
    EXPECT_EQ(ci->getType(), Type::getInt1Ty());
    EXPECT_TRUE(ci->isOne());
    EXPECT_FALSE(ci->isZero());
    EXPECT_EQ(ci->getZExtValue(), 1u);
    EXPECT_EQ(ci->getSExtValue(), 1);
    EXPECT_EQ(ci->getBitWidth(), 1u);
}

TEST(ConstantIntTest, GetInt1False) {
    auto* ci = ConstantInt::getInt1(false);
    EXPECT_EQ(ci->getType(), Type::getInt1Ty());
    EXPECT_TRUE(ci->isZero());
    EXPECT_FALSE(ci->isOne());
    EXPECT_EQ(ci->getZExtValue(), 0u);
}

TEST(ConstantIntTest, GetInt8) {
    auto* ci = ConstantInt::getInt8(-42);
    EXPECT_EQ(ci->getType(), Type::getInt8Ty());
    EXPECT_EQ(ci->getBitWidth(), 8u);
    EXPECT_EQ(ci->getSExtValue(), -42);
}

TEST(ConstantIntTest, GetInt16) {
    auto* ci = ConstantInt::getInt16(32767);
    EXPECT_EQ(ci->getType(), Type::getInt16Ty());
    EXPECT_EQ(ci->getBitWidth(), 16u);
    EXPECT_EQ(static_cast<int16_t>(ci->getSExtValue()), 32767);
}

TEST(ConstantIntTest, GetInt32) {
    auto* ci = ConstantInt::getInt32(42);
    EXPECT_EQ(ci->getType(), Type::getInt32Ty());
    EXPECT_EQ(ci->getBitWidth(), 32u);
    EXPECT_EQ(ci->getSExtValue(), 42);
}

TEST(ConstantIntTest, GetInt64) {
    auto* ci = ConstantInt::getInt64(0x7FFFFFFFFFFFFFFFLL);
    EXPECT_EQ(ci->getType(), Type::getInt64Ty());
    EXPECT_EQ(ci->getBitWidth(), 64u);
    EXPECT_EQ(ci->getSExtValue(), 0x7FFFFFFFFFFFFFFFLL);
}

TEST(ConstantIntTest, GetIntZero) {
    auto* ci = ConstantInt::getInt(Type::getInt32Ty(), 0);
    EXPECT_TRUE(ci->isZero());
    EXPECT_FALSE(ci->isOne());
    EXPECT_EQ(ci->getZExtValue(), 0u);
}

TEST(ConstantIntTest, GetIntLarge) {
    auto* ci = ConstantInt::getInt(Type::getInt64Ty(), UINT64_MAX);
    EXPECT_EQ(ci->getZExtValue(), UINT64_MAX);
    EXPECT_EQ(ci->getSExtValue(), -1);
}

TEST(ConstantIntTest, GetIntNegativeSExt) {
    auto* ci = ConstantInt::getInt(Type::getInt32Ty(), 0xFFFFFFFF);
    EXPECT_EQ(static_cast<int32_t>(ci->getSExtValue()), -1);
}

TEST(ConstantFpTest, GetFloat) {
    auto* cf = ConstantFP::getFloat(3.14f);
    ASSERT_NE(cf, nullptr);
    EXPECT_EQ(cf->getType(), Type::getFloatTy());
    EXPECT_FLOAT_EQ(cf->getFloatValue(), 3.14f);
}

TEST(ConstantFpTest, GetDouble) {
    auto* cf = ConstantFP::getDouble(2.718281828);
    ASSERT_NE(cf, nullptr);
    EXPECT_EQ(cf->getType(), Type::getDoubleTy());
    EXPECT_DOUBLE_EQ(cf->getDoubleValue(), 2.718281828);
}

TEST(ConstantFpTest, FloatZero) {
    auto* cf = ConstantFP::getFloat(0.0f);
    EXPECT_FLOAT_EQ(cf->getFloatValue(), 0.0f);
}

TEST(ConstantFpTest, FloatNegative) {
    auto* cf = ConstantFP::getFloat(-1.5f);
    EXPECT_FLOAT_EQ(cf->getFloatValue(), -1.5f);
}

TEST(GlobalVariableTest, Construction) {
    GlobalVariable gv(Type::getInt32Ty(), "my_global");
    EXPECT_EQ(gv.getType(), Type::getInt32Ty());
    EXPECT_EQ(gv.getName(), "my_global");
}

TEST(GlobalVariableTest, PointerType) {
    GlobalVariable gv(Type::getPointerTy(Type::getInt64Ty()), "ptr");
    EXPECT_EQ(gv.getName(), "ptr");
    EXPECT_TRUE(gv.getType()->isPointer());
}

TEST(GlobalVariableTest, EmptyName) {
    GlobalVariable gv(Type::getInt32Ty(), "");
    EXPECT_EQ(gv.getName(), "");
}
