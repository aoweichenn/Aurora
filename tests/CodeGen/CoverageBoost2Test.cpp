// Boosting line coverage: ObjectWriter + pipeline tests
#include <gtest/gtest.h>
#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include <cstdio>

using namespace aurora;

// ---- ObjectWriter full coverage ----
TEST(ObjectWriterFullTest, WriteWithFunction) {
    auto m = std::make_unique<Module>("wf");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fn = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "add");
    AIRBuilder b(fn->getEntryBlock());
    b.createRet(b.createAdd(Type::getInt64Ty(), 0, 1));

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    ObjectWriter w;
    w.addFunction(mf);
    EXPECT_TRUE(w.write("/tmp/aurora_func.o"));
    std::remove("/tmp/aurora_func.o");
}

TEST(ObjectWriterFullTest, WriteWithMultipleFuncs) {
    auto m = std::make_unique<Module>("wm");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto tm = TargetMachine::createX86_64();
    ObjectWriter w;

    for (int i = 0; i < 3; i++) {
        auto* fn = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f"+std::to_string(i));
        AIRBuilder(fn->getEntryBlock()).createRet(AIRBuilder(fn->getEntryBlock()).createConstantInt(i));
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
        w.addFunction(mf);
    }

    EXPECT_TRUE(w.write("/tmp/aurora_multi.o"));
    std::remove("/tmp/aurora_multi.o");
}

TEST(ObjectWriterFullTest, WriteWithBranching) {
    auto m = std::make_unique<Module>("wb");
    SmallVector<Type*,8> a = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), a), "f");
    auto* e = f->getEntryBlock();
    auto* t = f->createBasicBlock("true");
    auto* f2 = f->createBasicBlock("false");
    AIRBuilder b(e);
    b.createCondBr(b.createICmp(ICmpCond::SLT, 0, b.createConstantInt(5)), t, f2);
    b.setInsertPoint(t); b.createRet(b.createConstantInt(1));
    b.setInsertPoint(f2); b.createRet(b.createConstantInt(0));

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    ObjectWriter w;
    w.addFunction(mf);
    EXPECT_TRUE(w.write("/tmp/aurora_branch.o"));
    std::remove("/tmp/aurora_branch.o");
}

TEST(ObjectWriterFullTest, AddGlobalThenFunction) {
    ObjectWriter w;
    auto* gv = new GlobalVariable(Type::getInt64Ty(), "counter", ConstantInt::getInt64(0xDEAD));
    w.addGlobal(*gv);
    w.addExternSymbol("printf");
    EXPECT_TRUE(w.write("/tmp/aurora_gf.o"));
    delete gv;
    std::remove("/tmp/aurora_gf.o");
}

// ---- More pipeline tests for Select, Switch ----
TEST(PipelineBoostTest, SelectInstruction) {
    auto m = std::make_unique<Module>("sel");
    SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned cmp = b.createICmp(ICmpCond::SGT, 0, 1);
    unsigned sel = b.createSelect(Type::getInt64Ty(), cmp, 0, 1);
    b.createRet(sel);
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
    EXPECT_GT(mf.getBlocks().size(), 0u);
}

TEST(PipelineBoostTest, FloatDoubleMix) {
    auto m = std::make_unique<Module>("fd");
    SmallVector<Type*,8> p = {Type::getDoubleTy(), Type::getDoubleTy()};
    auto* f = m->createFunction(new FunctionType(Type::getDoubleTy(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned r = b.createAdd(Type::getDoubleTy(), 0, 1);
    r = b.createSub(Type::getDoubleTy(), r, 0);
    r = b.createMul(Type::getDoubleTy(), r, 1);
    b.createRet(r);
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
}

TEST(PipelineBoostTest, ICmpAllConditions) {
    auto m = std::make_unique<Module>("allcmp");
    for (auto cond : {ICmpCond::EQ, ICmpCond::NE, ICmpCond::SGT, ICmpCond::SGE,
                      ICmpCond::SLT, ICmpCond::SLE, ICmpCond::UGT, ICmpCond::UGE,
                      ICmpCond::ULT, ICmpCond::ULE}) {
        SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
        auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
        AIRBuilder b(f->getEntryBlock());
        unsigned c = b.createICmp(cond, 0, 1);
        b.createRet(c);
    }
    auto tm = TargetMachine::createX86_64();
    for (auto& fn : m->getFunctions()) {
        MachineFunction mf(*fn, *tm);
        PassManager pm;
        CodeGenContext::addStandardPasses(pm, *tm);
        pm.run(mf);
    }
}

TEST(PipelineBoostTest, MultipleReturnBlocks) {
    auto m = std::make_unique<Module>("mr");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    auto* e = f->getEntryBlock();
    auto* r1 = f->createBasicBlock("r1");
    auto* r2 = f->createBasicBlock("r2");
    AIRBuilder b(e);
    b.createCondBr(b.createICmp(ICmpCond::SGT, 0, b.createConstantInt(0)), r1, r2);
    b.setInsertPoint(r1); b.createRet(b.createConstantInt(42));
    b.setInsertPoint(r2); b.createRet(b.createConstantInt(0));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
}
