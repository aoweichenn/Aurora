#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

static void runModule(Module& mod) {
    auto tm = TargetMachine::createX86_64();
    for (auto& fn : mod.getFunctions()) {
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        ASSERT_GT(mf.getBlocks().size(), 0u);
        for (auto& b : mf.getBlocks()) ASSERT_FALSE(b->empty());
    }
}

TEST(ISelPipeline2Test, SimpleAddSubMul) {
    auto m = std::make_unique<Module>("t");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f1");
    AIRBuilder b(f->getEntryBlock());
    b.createRet(b.createAdd(Type::getInt64Ty(), 0, 1));
    f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f2");
    AIRBuilder(f->getEntryBlock()).createRet(AIRBuilder(f->getEntryBlock()).createSub(Type::getInt64Ty(), 0, 1));
    f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f3");
    AIRBuilder(f->getEntryBlock()).createRet(AIRBuilder(f->getEntryBlock()).createMul(Type::getInt64Ty(), 0, 1));
    runModule(*m);
}

TEST(ISelPipeline2Test, BitwiseOps) {
    auto m = std::make_unique<Module>("bw");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* f = m->createFunction(ft, "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned r = b.createAnd(Type::getInt64Ty(), 0, 1);
    r = b.createOr(Type::getInt64Ty(), r, 0);
    r = b.createXor(Type::getInt64Ty(), r, 0);
    r = b.createShl(Type::getInt64Ty(), r, 0);
    r = b.createLShr(Type::getInt64Ty(), r, 0);
    r = b.createAShr(Type::getInt64Ty(), r, 0);
    b.createRet(r);
    runModule(*m);
}

TEST(ISelPipeline2Test, ConstantAndConv) {
    auto m = std::make_unique<Module>("cc");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned c = b.createConstantInt(42);
    unsigned s = b.createSExt(Type::getInt64Ty(), 0);
    unsigned z = b.createZExt(Type::getInt64Ty(), 0);
    b.createRet(b.createAdd(Type::getInt64Ty(), b.createAdd(Type::getInt64Ty(), c, s), z));
    runModule(*m);
}

TEST(ISelPipeline2Test, DivisionOps) {
    auto m = std::make_unique<Module>("div");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned two = b.createConstantInt(2);
    b.createRet(b.createSDiv(Type::getInt64Ty(), 0, two));
    runModule(*m);
}

TEST(ISelPipeline2Test, ICmpOps) {
    auto m = std::make_unique<Module>("cmp");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    b.createRet(b.createICmp(ICmpCond::SLT, 0, 1));
    runModule(*m);
}

TEST(ISelPipeline2Test, LoadStoreAlloca) {
    auto m = std::make_unique<Module>("mem");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned a = b.createAlloca(Type::getInt64Ty());
    b.createStore(0, a);
    b.createRet(b.createLoad(Type::getInt64Ty(), a));
    runModule(*m);
}

TEST(ISelPipeline2Test, RetVoidFunc) {
    auto m = std::make_unique<Module>("rv");
    SmallVector<Type*,8> p;
    auto* f = m->createFunction(new FunctionType(Type::getVoidTy(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    b.createRetVoid();
    runModule(*m);
}

TEST(ISelPipeline2Test, PhiMerge) {
    auto m = std::make_unique<Module>("ph");
    SmallVector<Type*,8> a = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), a), "f");
    auto* e = f->getEntryBlock();
    auto* t = f->createBasicBlock("t");
    auto* f2 = f->createBasicBlock("f");
    auto* m2 = f->createBasicBlock("m");
    AIRBuilder b(e);
    b.createCondBr(b.createICmp(ICmpCond::NE, 0, b.createConstantInt(0)), t, f2);
    b.setInsertPoint(t); unsigned one = b.createConstantInt(1); b.createBr(m2);
    b.setInsertPoint(f2); unsigned zero = b.createConstantInt(0); b.createBr(m2);
    b.setInsertPoint(m2);
    SmallVector<std::pair<BasicBlock*, unsigned>,4> inc = {{t, one}, {f2, zero}};
    b.createPhi(Type::getInt64Ty(), inc);
    b.createRet(one);
    runModule(*m);
}

TEST(ISelPipeline2Test, FloatAdd) {
    auto m = std::make_unique<Module>("fp");
    SmallVector<Type*,8> p = {Type::getFloatTy(), Type::getFloatTy()};
    auto* f = m->createFunction(new FunctionType(Type::getFloatTy(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    b.createRet(b.createAdd(Type::getFloatTy(), 0, 1));
    runModule(*m);
}
