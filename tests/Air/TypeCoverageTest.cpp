#include <gtest/gtest.h>
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(TypeCoverageTest, AllIntegerSizes) {
    EXPECT_EQ(Type::getInt1Ty()->getSizeInBits(), 1u);
    EXPECT_EQ(Type::getInt8Ty()->getSizeInBits(), 8u);
    EXPECT_EQ(Type::getInt16Ty()->getSizeInBits(), 16u);
    EXPECT_EQ(Type::getInt32Ty()->getSizeInBits(), 32u);
    EXPECT_EQ(Type::getInt64Ty()->getSizeInBits(), 64u);
}

TEST(TypeCoverageTest, FloatSizes) {
    EXPECT_EQ(Type::getFloatTy()->getSizeInBits(), 32u);
    EXPECT_EQ(Type::getDoubleTy()->getSizeInBits(), 64u);
}

TEST(TypeCoverageTest, ArrayType) {
    auto* arr = Type::getArrayTy(Type::getInt32Ty(), 100);
    EXPECT_EQ(arr->getNumElements(), 100u);
    EXPECT_EQ(arr->getElementType(), Type::getInt32Ty());
    EXPECT_TRUE(arr->isArray());
}

TEST(TypeCoverageTest, PointerTypeUnique) {
    auto* p1 = Type::getPointerTy(Type::getInt32Ty());
    auto* p2 = Type::getPointerTy(Type::getInt32Ty());
    EXPECT_EQ(p1, p2);
}

TEST(TypeCoverageTest, StructMembers) {
    SmallVector<Type*, 8> members = {Type::getInt32Ty(), Type::getInt64Ty()};
    auto* st = Type::getStructTy(members);
    EXPECT_TRUE(st->isStruct());
    EXPECT_EQ(st->getStructMembers().size(), 2u);
    EXPECT_GE(st->getMemberOffset(0), 0u);
}

TEST(TypeCoverageTest, StructAlignment) {
    SmallVector<Type*, 8> members = {Type::getInt8Ty(), Type::getInt64Ty()};
    auto* st = Type::getStructTy(members);
    // offsets are in bits: i8 at 0, i64 at 64 (8 bytes aligned)
    EXPECT_EQ(st->getMemberOffset(0), 0u);
    EXPECT_EQ(st->getMemberOffset(1), 64u); // 64 bits = 8 bytes
}

TEST(TypeCoverageTest, FunctionTypeVarArg) {
    SmallVector<Type*, 8> params = {Type::getInt32Ty()};
    auto* ft = Type::getFunctionTy(Type::getInt32Ty(), params, true);
    EXPECT_TRUE(ft->isVarArg());
    EXPECT_EQ(ft->getReturnType(), Type::getInt32Ty());
    EXPECT_EQ(ft->getParamTypes().size(), 1u);
}

TEST(TypeCoverageTest, VoidType) {
    EXPECT_TRUE(Type::getVoidTy()->isVoid());
    EXPECT_EQ(Type::getVoidTy()->getSizeInBits(), 0u);
}

TEST(TypeCoverageTest, SizeInBits) {
    EXPECT_EQ(Type::getInt32Ty()->getSizeInBits(), 32u);
    EXPECT_EQ(Type::getInt64Ty()->getSizeInBits(), 64u);
    EXPECT_EQ(Type::getPointerTy(Type::getInt32Ty())->getSizeInBits(), 64u);
}

TEST(TypeCoverageTest, ToString) {
    EXPECT_EQ(Type::getInt32Ty()->toString(), "i32");
    EXPECT_EQ(Type::getFloatTy()->toString(), "float");
    EXPECT_EQ(Type::getDoubleTy()->toString(), "double");
    EXPECT_EQ(Type::getVoidTy()->toString(), "void");
}
