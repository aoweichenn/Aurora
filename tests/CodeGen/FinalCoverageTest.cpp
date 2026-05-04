#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

// Helper: run pipeline on a single-function module
static void runPF(Module& m) {
    auto tm = TargetMachine::createX86_64();
    for (auto& f : m.getFunctions()) {
        MachineFunction mf(*f, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
    }
}

TEST(FinalCoverageTest, SDivConst) {
    auto m = std::make_unique<Module>("sd");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    b.createRet(b.createSDiv(Type::getInt64Ty(), 0, b.createConstantInt(3)));
    runPF(*m);
}

TEST(FinalCoverageTest, UDivOps) {
    auto m = std::make_unique<Module>("ud");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    b.createRet(b.createUDiv(Type::getInt64Ty(), 0, 1));
    runPF(*m);
}

TEST(FinalCoverageTest, ShiftsAllTypes) {
    auto m = std::make_unique<Module>("sh");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned r = b.createShl(Type::getInt64Ty(), 0, 1);
    r = b.createLShr(Type::getInt64Ty(), r, 0);
    r = b.createAShr(Type::getInt64Ty(), r, 0);
    b.createRet(r);
    runPF(*m);
}

TEST(FinalCoverageTest, FloatSubMulDiv) {
    auto m = std::make_unique<Module>("fp2");
    SmallVector<Type*,8> p = {Type::getFloatTy(), Type::getFloatTy()};
    auto* f = m->createFunction(new FunctionType(Type::getFloatTy(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned r = b.createSub(Type::getFloatTy(), 0, 1);
    r = b.createMul(Type::getFloatTy(), r, 1);
    r = b.createAdd(Type::getFloatTy(), r, 0);
    b.createRet(r);
    runPF(*m);
}

TEST(FinalCoverageTest, ComplexPhiMerge) {
    auto m = std::make_unique<Module>("phi2");
    SmallVector<Type*,8> a = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), a), "f");
    auto* e = f->getEntryBlock();
    auto* t = f->createBasicBlock("T");
    auto* fl = f->createBasicBlock("F");
    auto* mg = f->createBasicBlock("M");
    AIRBuilder b(e);
    b.createCondBr(b.createICmp(ICmpCond::NE, 0, b.createConstantInt(1)), t, fl);
    b.setInsertPoint(t);
    unsigned v1 = b.createAdd(Type::getInt64Ty(), 0, b.createConstantInt(10));
    b.createBr(mg);
    b.setInsertPoint(fl);
    unsigned v2 = b.createSub(Type::getInt64Ty(), 0, b.createConstantInt(5));
    b.createBr(mg);
    b.setInsertPoint(mg);
    SmallVector<std::pair<BasicBlock*, unsigned>,4> inc = {{t, v1}, {fl, v2}};
    unsigned phi = b.createPhi(Type::getInt64Ty(), inc);
    b.createRet(phi);
    runPF(*m);
}

TEST(FinalCoverageTest, MultiBlockChain) {
    auto m = std::make_unique<Module>("chain");
    SmallVector<Type*,8> a = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), a), "f");
    auto* b1 = f->getEntryBlock();
    auto* b2 = f->createBasicBlock("B2");
    auto* b3 = f->createBasicBlock("B3");
    AIRBuilder b(b1);
    b.createCondBr(b.createICmp(ICmpCond::SGT, 0, b.createConstantInt(10)), b2, b3);
    b.setInsertPoint(b2);
    b.createRet(b.createConstantInt(1));
    b.setInsertPoint(b3);
    b.createRet(b.createConstantInt(0));
    runPF(*m);
}
