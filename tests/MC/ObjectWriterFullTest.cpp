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

TEST(ObjectWriterFullTest, WriteMultipleFunctions) {
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

TEST(ObjectWriterFullTest, WriteWithGlobals) {
    ObjectWriter w;
    auto* gv = new GlobalVariable(Type::getInt64Ty(), "counter", ConstantInt::getInt64(0xDEAD));
    w.addGlobal(*gv);
    w.addExternSymbol("printf");
    EXPECT_TRUE(w.write("/tmp/aurora_gf.o"));
    delete gv;
    std::remove("/tmp/aurora_gf.o");
}

TEST(ObjectWriterFullTest, WriteBranchFunction) {
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
