#include <gtest/gtest.h>
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

static FunctionType* makeTestFnType() {
    SmallVector<Type*, 8> params;
    params.push_back(Type::getInt32Ty());
    params.push_back(Type::getInt32Ty());
    return new FunctionType(Type::getInt32Ty(), params);
}

TEST(FunctionTest, Construction) {
    auto* fnTy = makeTestFnType();
    Function fn(fnTy, "test_func");
    EXPECT_EQ(fn.getName(), "test_func");
    EXPECT_EQ(fn.getNumArgs(), 2u);
}

TEST(FunctionTest, CreateBasicBlock) {
    auto* fnTy = makeTestFnType();
    Function fn(fnTy, "test");
    auto* bb = fn.createBasicBlock("entry");
    EXPECT_NE(bb, nullptr);
    EXPECT_EQ(bb->getName(), "entry");
    EXPECT_GE(fn.getBlocks().size(), 1u);
    EXPECT_NE(fn.getEntryBlock(), nullptr);
}

TEST(FunctionTest, MultipleBlocks) {
    auto* fnTy = makeTestFnType();
    Function fn(fnTy, "test");
    fn.createBasicBlock("entry");
    fn.createBasicBlock("loop");
    fn.createBasicBlock("exit");
    EXPECT_GE(fn.getBlocks().size(), 3u);
}

TEST(FunctionTest, NextVReg) {
    auto* fnTy = makeTestFnType();
    Function fn(fnTy, "test");
    // First vregs (0, 1) are params
    fn.nextVReg(); // alloc vreg 2
    fn.nextVReg(); // alloc vreg 3
    EXPECT_EQ(fn.getNumVRegs(), 4u);
}

TEST(FunctionTest, FunctionTypeAccess) {
    auto* fnTy = makeTestFnType();
    Function fn(fnTy, "test");
    EXPECT_EQ(fn.getFunctionType(), fnTy);
    EXPECT_EQ(fn.getFunctionType()->getReturnType(), Type::getInt32Ty());
}
