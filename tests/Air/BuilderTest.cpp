#include <gtest/gtest.h>
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"

using namespace aurora;

TEST(BuilderTest, CreateAdd) {
    const SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    Module mod("test");
    auto* fn = mod.createFunction(fnTy, "add");
    BasicBlock* bb = fn->createBasicBlock("entry");
    AIRBuilder builder(bb);

    const unsigned result = builder.createAdd(Type::getInt32Ty(), 0, 1);
    EXPECT_EQ(result, 2u); // First 2 vregs are params, so result is #2
}

TEST(BuilderTest, BuildSimpleFunction) {
    const SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    Module mod("test");
    const auto* fn = mod.createFunction(fnTy, "square");

    AIRBuilder builder(fn->getEntryBlock());
    // %2 = mul i32 %0, %0  (square the first argument)
    const unsigned result = builder.createMul(Type::getInt32Ty(), 0, 0);
    builder.createRet(result);

    EXPECT_EQ(fn->getNumVRegs(), 3u);
}

TEST(BuilderTest, BuildFunctionWithControlFlow) {
    const SmallVector<Type*, 8> params = {Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    Module mod("test");
    auto* fn = mod.createFunction(fnTy, "abs");

    auto* entry = fn->getEntryBlock();
    auto* ltZero = fn->createBasicBlock("lt_zero");
    auto* merge  = fn->createBasicBlock("merge");

    AIRBuilder builder(entry);
    const unsigned constZero = fn->nextVReg();
    // This test demonstrates basic IR building
    const unsigned cmp = builder.createICmp(ICmpCond::SLT, 0, constZero);
    builder.createCondBr(cmp, ltZero, merge);

    builder.setInsertPoint(ltZero);
    unsigned neg = builder.createSub(Type::getInt32Ty(), constZero, 0);
    builder.createBr(merge);

    builder.setInsertPoint(merge);
    SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings;
    incomings.push_back({entry, 0});
    incomings.push_back({ltZero, neg});
    const unsigned phi = builder.createPhi(Type::getInt32Ty(), incomings);
    builder.createRet(phi);

    EXPECT_EQ(fn->getBlocks().size(), 3u);
}

TEST(BuilderTest, CreateLoadAndStore) {
    const SmallVector<Type*, 8> params;
    auto* fnTy = new FunctionType(Type::getVoidTy(), params);
    Module mod("test");
    const auto* fn = mod.createFunction(fnTy, "memtest");

    AIRBuilder builder(fn->getEntryBlock());
    const unsigned alloca = builder.createAlloca(Type::getInt32Ty());
    builder.createStore(0, alloca);
    const unsigned loaded = builder.createLoad(Type::getInt32Ty(), alloca);
    EXPECT_NE(loaded, alloca);

    const unsigned add = builder.createAdd(Type::getInt32Ty(), loaded, loaded);
    builder.createRet(add);
}

TEST(BuilderTest, CreateCall) {
    const SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
    auto* calleeTy = new FunctionType(Type::getInt32Ty(), params);

    Module mod("test");
    auto* callee = mod.createFunction(calleeTy, "helper");

    const SmallVector<Type*, 8> callerParams;
    auto* callerTy = new FunctionType(Type::getInt32Ty(), callerParams);
    const auto* fn = mod.createFunction(callerTy, "caller");

    AIRBuilder builder(fn->getEntryBlock());
    const SmallVector<unsigned, 8> args = {0, 1};
    const unsigned result = builder.createCall(callee, args);
    builder.createRet(result);

    EXPECT_GT(fn->getNumVRegs(), 0u);
}

TEST(BuilderTest, CreateShiftOps) {
    const SmallVector<Type*, 8> params = {Type::getInt32Ty(), Type::getInt32Ty()};
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    Module mod("test");
    const auto* fn = mod.createFunction(fnTy, "shifts");

    AIRBuilder builder(fn->getEntryBlock());
    const unsigned shl = builder.createShl(Type::getInt32Ty(), 0, 1);
    const unsigned lshr = builder.createLShr(Type::getInt32Ty(), shl, 0);
    builder.createRet(lshr);

    EXPECT_GT(fn->getNumVRegs(), 2u);
}
