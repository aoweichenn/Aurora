#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

TEST(ISelCoverageTest, AllBinaryOpsPipeline) {
    auto mod = std::make_unique<Module>("cov");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);

    auto* fn = mod->createFunction(fnTy, "all_binary");
    AIRBuilder b(fn->getEntryBlock());
    unsigned r = b.createConstantInt(42);
    r = b.createAdd(Type::getInt64Ty(), r, 0);
    r = b.createSub(Type::getInt64Ty(), r, 0);
    r = b.createMul(Type::getInt64Ty(), r, 0);
    r = b.createAnd(Type::getInt64Ty(), r, 0);
    r = b.createOr(Type::getInt64Ty(), r, 0);
    r = b.createXor(Type::getInt64Ty(), r, 0);
    r = b.createShl(Type::getInt64Ty(), r, 0);
    b.createRet(r);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

TEST(ISelCoverageTest, ConverOpsPipeline) {
    auto mod = std::make_unique<Module>("conv");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = mod->createFunction(fnTy, "conv_ops");
    AIRBuilder b(fn->getEntryBlock());
    unsigned s = b.createSExt(Type::getInt64Ty(), 0);
    unsigned z = b.createZExt(Type::getInt64Ty(), 0);
    unsigned c = b.createConstantInt(1);
    unsigned add = b.createAdd(Type::getInt64Ty(), s, c);
    b.createRet(add);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

TEST(ISelCoverageTest, ConditionalFlowPipeline) {
    auto mod = std::make_unique<Module>("flow");
    SmallVector<Type*, 8> args = {Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), args);
    auto* fn = mod->createFunction(fnTy, "control");
    auto* entry = fn->getEntryBlock();
    auto* trueBB = fn->createBasicBlock("true");
    auto* falseBB = fn->createBasicBlock("false");
    auto* merge = fn->createBasicBlock("merge");

    AIRBuilder b(entry);
    unsigned c = b.createConstantInt(0);
    unsigned cmp = b.createICmp(ICmpCond::EQ, 0, c);
    b.createCondBr(cmp, trueBB, falseBB);

    b.setInsertPoint(trueBB);
    unsigned one = b.createConstantInt(1);
    b.createBr(merge);

    b.setInsertPoint(falseBB);
    unsigned zero = b.createConstantInt(0);
    b.createBr(merge);

    b.setInsertPoint(merge);
    SmallVector<std::pair<BasicBlock*, unsigned>, 4> incomings = {{trueBB, one}, {falseBB, zero}};
    unsigned phi = b.createPhi(Type::getInt64Ty(), incomings);
    b.createRet(phi);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_EQ(mf.getBlocks().size(), 4u);
}

TEST(ISelCoverageTest, SelectPipeline) {
    auto mod = std::make_unique<Module>("sel");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = mod->createFunction(fnTy, "select");
    AIRBuilder b(fn->getEntryBlock());
    unsigned c = b.createICmp(ICmpCond::SGT, 0, 1);
    unsigned sel = b.createSelect(Type::getInt64Ty(), c, 0, 1);
    b.createRet(sel);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

TEST(ISelCoverageTest, UDivSDivPipeline) {
    auto mod = std::make_unique<Module>("div");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = mod->createFunction(fnTy, "divtest");
    AIRBuilder b(fn->getEntryBlock());
    unsigned d = b.createSDiv(Type::getInt64Ty(), 0, b.createConstantInt(2));
    unsigned u = b.createUDiv(Type::getInt64Ty(), 0, 1);
    b.createRet(d);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

TEST(ISelCoverageTest, AllocaStoreLoad) {
    auto mod = std::make_unique<Module>("mem");
    SmallVector<Type*, 8> params = {Type::getInt64Ty()};
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);
    auto* fn = mod->createFunction(fnTy, "memtest");
    AIRBuilder b(fn->getEntryBlock());
    unsigned a = b.createAlloca(Type::getInt64Ty());
    b.createStore(0, a);
    unsigned l = b.createLoad(Type::getInt64Ty(), a);
    b.createRet(l);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

TEST(ISelCoverageTest, FloatingPointBinOps) {
    auto mod = std::make_unique<Module>("fp");
    SmallVector<Type*, 8> params = {Type::getDoubleTy(), Type::getDoubleTy()};
    auto* fnTy = new FunctionType(Type::getDoubleTy(), params);
    auto* fn = mod->createFunction(fnTy, "fpadd");
    AIRBuilder b(fn->getEntryBlock());
    unsigned r = b.createAdd(Type::getDoubleTy(), 0, 1);
    b.createRet(r);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

