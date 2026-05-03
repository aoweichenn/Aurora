#include <gtest/gtest.h>
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(TypeTest, VoidType) {
    auto* t = Type::getVoidTy();
    EXPECT_EQ(t->getKind(), TypeKind::Void);
    EXPECT_EQ(t->getSizeInBits(), 0u);
    EXPECT_EQ(t->toString(), "void");
}

TEST(TypeTest, IntegerTypes) {
    EXPECT_EQ(Type::getInt1Ty()->getSizeInBits(), 1u);
    EXPECT_EQ(Type::getInt8Ty()->getSizeInBits(), 8u);
    EXPECT_EQ(Type::getInt32Ty()->getSizeInBits(), 32u);
    EXPECT_EQ(Type::getInt64Ty()->getSizeInBits(), 64u);
}

TEST(TypeTest, IntegerTypeToString) {
    EXPECT_EQ(Type::getInt1Ty()->toString(), "i1");
    EXPECT_EQ(Type::getInt8Ty()->toString(), "i8");
    EXPECT_EQ(Type::getInt32Ty()->toString(), "i32");
    EXPECT_EQ(Type::getInt64Ty()->toString(), "i64");
}

TEST(TypeTest, FloatTypes) {
    EXPECT_EQ(Type::getFloatTy()->getKind(), TypeKind::Float);
    EXPECT_EQ(Type::getFloatTy()->getSizeInBits(), 32u);
    EXPECT_EQ(Type::getDoubleTy()->getSizeInBits(), 64u);
}

TEST(TypeTest, PointerType) {
    auto* int32 = Type::getInt32Ty();
    auto* ptrTy = Type::getPointerTy(int32);
    EXPECT_EQ(ptrTy->getKind(), TypeKind::Pointer);
    EXPECT_EQ(ptrTy->getElementType(), int32);
    EXPECT_EQ(ptrTy->getSizeInBits(), 64u); // x86-64 pointer
}

TEST(TypeTest, PointerUniqueness) {
    auto* i32 = Type::getInt32Ty();
    auto* p1 = Type::getPointerTy(i32);
    auto* p2 = Type::getPointerTy(i32);
    EXPECT_EQ(p1, p2); // Same type should return same object
}

TEST(TypeTest, ArrayType) {
    auto* i32 = Type::getInt32Ty();
    auto* arr = Type::getArrayTy(i32, 10);
    EXPECT_EQ(arr->getKind(), TypeKind::Array);
    EXPECT_EQ(arr->getElementType(), i32);
    EXPECT_EQ(arr->getNumElements(), 10u);
    EXPECT_EQ(arr->getSizeInBits(), 320u);
}

TEST(TypeTest, FunctionType) {
    SmallVector<Type*, 8> params;
    params.push_back(Type::getInt32Ty());
    params.push_back(Type::getFloatTy());
    auto* fnTy = Type::getFunctionTy(Type::getInt64Ty(), std::move(params));
    EXPECT_EQ(fnTy->getKind(), TypeKind::Function);
    EXPECT_EQ(fnTy->getReturnType(), Type::getInt64Ty());
    EXPECT_EQ(fnTy->getParamTypes().size(), 2u);
}

TEST(TypeTest, TypeKindChecks) {
    EXPECT_TRUE(Type::getInt32Ty()->isInteger());
    EXPECT_TRUE(Type::getFloatTy()->isFloat());
    EXPECT_TRUE(Type::getVoidTy()->isVoid());
    EXPECT_TRUE(Type::getPointerTy(Type::getInt32Ty())->isPointer());
}
