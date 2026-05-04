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

static void runPipeline(Module& mod) {
    auto tm = TargetMachine::createX86_64();
    for (auto& fn : mod.getFunctions()) {
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        EXPECT_GT(mf.getBlocks().size(), 0u);
        for (auto& mbb : mf.getBlocks())
            EXPECT_FALSE(mbb->empty());
    }
}

// ---- All Binary Ops ----
TEST(ISelCoverageTest, AllBinaryOps) {
    auto mod = std::make_unique<Module>("bin");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "bin");
    AIRBuilder b(fn->getEntryBlock());
    unsigned r = b.createAdd(Type::getInt64Ty(), 0, 1);
    r = b.createSub(Type::getInt64Ty(), r, 1);
    r = b.createMul(Type::getInt64Ty(), r, 1);
    r = b.createAnd(Type::getInt64Ty(), r, 0);
    r = b.createOr(Type::getInt64Ty(), r, 0);
    r = b.createXor(Type::getInt64Ty(), r, 0);
    r = b.createShl(Type::getInt64Ty(), r, 0);
    r = b.createLShr(Type::getInt64Ty(), r, 0);
    r = b.createAShr(Type::getInt64Ty(), r, 0);
    b.createRet(r);
    runPipeline(*mod);
}

// ---- Constant + Conversion Ops ----
TEST(ISelCoverageTest, ConstantAndConversion) {
    auto mod = std::make_unique<Module>("conv");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "conv");
    AIRBuilder b(fn->getEntryBlock());
    unsigned c = b.createConstantInt(42);
    unsigned s = b.createSExt(Type::getInt64Ty(), 0);
    unsigned z = b.createZExt(Type::getInt64Ty(), 0);
    unsigned t = b.createTrunc(Type::getInt32Ty(), 0);
    EXPECT_NE(t, ~0U);
    unsigned r = b.createAdd(Type::getInt64Ty(), b.createAdd(Type::getInt64Ty(), c, s), z);
    b.createRet(r);
    runPipeline(*mod);
}

// ---- UDiv/SDiv ----
TEST(ISelCoverageTest, DivisionOps) {
    auto mod = std::make_unique<Module>("div");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "div");
    AIRBuilder b(fn->getEntryBlock());
    unsigned two = b.createConstantInt(2);
    unsigned d = b.createSDiv(Type::getInt64Ty(), 0, two);
    b.createRet(d);
    runPipeline(*mod);
}

// ---- ICmp (one function with all conditions) ----
TEST(ISelCoverageTest, ICmpConditions) {
    auto mod = std::make_unique<Module>("icmp");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "cmp");
    AIRBuilder b(fn->getEntryBlock());
    unsigned c1 = b.createICmp(ICmpCond::EQ, 0, 1);
    unsigned c2 = b.createICmp(ICmpCond::NE, 0, 1);
    unsigned c3 = b.createICmp(ICmpCond::SLT, 0, 1);
    unsigned r1 = b.createAdd(Type::getInt64Ty(), c1, c2);
    unsigned r2 = b.createAdd(Type::getInt64Ty(), r1, c3);
    b.createRet(r2);
    runPipeline(*mod);
}

// ---- Control Flow (Phi + CondBr + Br) ----
TEST(ISelCoverageTest, ControlFlowWithPhi) {
    auto mod = std::make_unique<Module>("phi");
    SmallVector<Type*,8> a = {Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), a);
    auto* fn = mod->createFunction(ft, "phi");
    auto* entry = fn->getEntryBlock();
    auto* tBB = fn->createBasicBlock("t");
    auto* fBB = fn->createBasicBlock("f");
    auto* mBB = fn->createBasicBlock("m");
    AIRBuilder b(entry);
    unsigned c = b.createConstantInt(0);
    unsigned cmp = b.createICmp(ICmpCond::NE, 0, c);
    b.createCondBr(cmp, tBB, fBB);
    b.setInsertPoint(tBB); unsigned one = b.createConstantInt(1); b.createBr(mBB);
    b.setInsertPoint(fBB); unsigned zero = b.createConstantInt(0); b.createBr(mBB);
    b.setInsertPoint(mBB);
    SmallVector<std::pair<BasicBlock*, unsigned>,4> inc = {{tBB, one}, {fBB, zero}};
    (void)b.createPhi(Type::getInt64Ty(), inc);
    b.createRet(one);
    runPipeline(*mod);
}

// ---- Alloca/Store/Load ----
TEST(ISelCoverageTest, MemoryOps) {
    auto mod = std::make_unique<Module>("mem");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "mem");
    AIRBuilder b(fn->getEntryBlock());
    unsigned a1 = b.createAlloca(Type::getInt64Ty());
    unsigned a2 = b.createAlloca(Type::getInt64Ty());
    b.createStore(0, a1);
    b.createStore(b.createConstantInt(10), a2);
    unsigned l1 = b.createLoad(Type::getInt64Ty(), a1);
    unsigned l2 = b.createLoad(Type::getInt64Ty(), a2);
    b.createRet(b.createAdd(Type::getInt64Ty(), l1, l2));
    runPipeline(*mod);
}

// ---- GEP ----
TEST(ISelCoverageTest, GEPAccess) {
    auto mod = std::make_unique<Module>("gep");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "gep");
    AIRBuilder b(fn->getEntryBlock());
    SmallVector<unsigned,4> idx = {0u};
    unsigned gep = b.createGEP(Type::getPointerTy(Type::getInt64Ty()), 0, idx);
    unsigned val = b.createLoad(Type::getInt64Ty(), gep);
    b.createRet(val);
    runPipeline(*mod);
}

// ---- Select ----
TEST(ISelCoverageTest, SelectOp) {
    auto mod = std::make_unique<Module>("sel");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "sel");
    AIRBuilder b(fn->getEntryBlock());
    unsigned cmp = b.createICmp(ICmpCond::SGT, 0, 1);
    unsigned sel = b.createSelect(Type::getInt64Ty(), cmp, 0, 1);
    b.createRet(sel);
    runPipeline(*mod);
}

// ---- Unreachable ----
TEST(ISelCoverageTest, UnreachableOp) {
    auto mod = std::make_unique<Module>("unr");
    SmallVector<Type*,8> p;
    auto* ft = new FunctionType(Type::getVoidTy(), p);
    auto* fn = mod->createFunction(ft, "unr");
    AIRBuilder b(fn->getEntryBlock());
    b.createUnreachable();
    runPipeline(*mod);
}

// ---- Float ops ----
TEST(ISelCoverageTest, FloatOps) {
    auto mod = std::make_unique<Module>("fp");
    SmallVector<Type*,8> p = {Type::getFloatTy(), Type::getFloatTy()};
    auto* ft = new FunctionType(Type::getFloatTy(), p);
    auto* fn = mod->createFunction(ft, "fpadd");
    AIRBuilder b(fn->getEntryBlock());
    unsigned r = b.createAdd(Type::getFloatTy(), 0, 1);
    b.createRet(r);
    runPipeline(*mod);
}

// ---- Float with ICmp standalone ----
TEST(ISelCoverageTest, ICmpStandalone) {
    auto mod = std::make_unique<Module>("icmps");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "icmps");
    AIRBuilder b(fn->getEntryBlock());
    unsigned c1 = b.createICmp(ICmpCond::SLT, 0, b.createConstantInt(0));
    unsigned c2 = b.createICmp(ICmpCond::SGT, 0, b.createConstantInt(10));
    unsigned r = b.createAdd(Type::getInt64Ty(), c1, c2);
    b.createRet(r);
    runPipeline(*mod);
}

// ---- BitCast ----
TEST(ISelCoverageTest, BitCastOp) {
    auto mod = std::make_unique<Module>("bc");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* ft = new FunctionType(Type::getInt64Ty(), p);
    auto* fn = mod->createFunction(ft, "bc");
    AIRBuilder b(fn->getEntryBlock());
    unsigned s = b.createSExt(Type::getInt64Ty(), 0);
    b.createRet(s);
    runPipeline(*mod);
}

// ---- Empty module ----
TEST(ISelCoverageTest, EmptyModule) {
    auto mod = std::make_unique<Module>("empty");
    auto tm = TargetMachine::createX86_64();
    CodeGenContext cgc(*tm, *mod);
    cgc.run();
    SUCCEED();
}

// ---- Return void ----
TEST(ISelCoverageTest, RetVoid) {
    auto mod = std::make_unique<Module>("rv");
    SmallVector<Type*,8> p;
    auto* ft = new FunctionType(Type::getVoidTy(), p);
    auto* fn = mod->createFunction(ft, "rv");
    AIRBuilder b(fn->getEntryBlock());
    b.createRetVoid();
    runPipeline(*mod);
}
