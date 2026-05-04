#include <gtest/gtest.h>
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

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
    for (auto cond : {ICmpCond::EQ, ICmpCond::NE, ICmpCond::SGT, ICmpCond::SGE,
                      ICmpCond::SLT, ICmpCond::SLE, ICmpCond::UGT, ICmpCond::UGE,
                      ICmpCond::ULT, ICmpCond::ULE}) {
        auto m = std::make_unique<Module>("cmp");
        SmallVector<Type*,8> p = {Type::getInt64Ty(), Type::getInt64Ty()};
        auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
        AIRBuilder b(f->getEntryBlock());
        b.createRet(b.createICmp(cond, 0, 1));
        auto tm = TargetMachine::createX86_64();
        MachineFunction mf(*f, *tm);
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
