// Complete ObjectWriter method coverage
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
#include <cstdio>

using namespace aurora;

TEST(ObjectWriterFuncTest, Construction) { ObjectWriter w; SUCCEED(); }
TEST(ObjectWriterFuncTest, AddGlobal) { ObjectWriter w; auto* g = new GlobalVariable(Type::getInt64Ty(), "x", ConstantInt::getInt64(1)); w.addGlobal(*g); delete g; SUCCEED(); }
TEST(ObjectWriterFuncTest, AddExtern) { ObjectWriter w; w.addExternSymbol("printf"); w.addExternSymbol("malloc"); SUCCEED(); }

TEST(ObjectWriterFuncTest, WriteEmpty) { ObjectWriter w; EXPECT_TRUE(w.write("/tmp/oe.o")); std::remove("/tmp/oe.o"); }

TEST(ObjectWriterFuncTest, WriteWithDataOnly) {
    ObjectWriter w;
    auto* g = new GlobalVariable(Type::getInt64Ty(), "v", ConstantInt::getInt64(42));
    w.addGlobal(*g); w.addExternSymbol("ext");
    EXPECT_TRUE(w.write("/tmp/od.o")); delete g; std::remove("/tmp/od.o");
}

TEST(ObjectWriterFuncTest, WriteWithFunctionSimple) {
    auto m = std::make_unique<Module>("wf");
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), {}), "f");
    AIRBuilder(f->getEntryBlock()).createRet(AIRBuilder(f->getEntryBlock()).createConstantInt(0));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm; CodeGenContext::addStandardPasses(pm, *tm); pm.run(mf);
    ObjectWriter w; w.addFunction(mf);
    EXPECT_TRUE(w.write("/tmp/of.o")); std::remove("/tmp/of.o");
}

TEST(ObjectWriterFuncTest, WriteWithCall) {
    auto m = std::make_unique<Module>("wc");
    auto* callee = m->createFunction(new FunctionType(Type::getInt64Ty(), {Type::getInt64Ty(), Type::getInt64Ty()}), "helper");
    AIRBuilder(callee->getEntryBlock()).createRet(0);
    auto* caller = m->createFunction(new FunctionType(Type::getInt64Ty(), {}), "caller");
    AIRBuilder b(caller->getEntryBlock());
    b.createRet(b.createCall(callee, {b.createConstantInt(1), b.createConstantInt(2)}));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*caller, *tm);
    PassManager pm; CodeGenContext::addStandardPasses(pm, *tm); pm.run(mf);
    ObjectWriter w; w.addFunction(mf);
    EXPECT_TRUE(w.write("/tmp/oc.o")); std::remove("/tmp/oc.o");
}

TEST(ObjectWriterFuncTest, WriteWithBranches) {
    auto m = std::make_unique<Module>("wbr");
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), {Type::getInt64Ty()}), "f");
    auto* e = f->getEntryBlock(); auto* t = f->createBasicBlock("T"); auto* fl = f->createBasicBlock("F");
    AIRBuilder b(e); b.createCondBr(b.createICmp(ICmpCond::SLT, 0, b.createConstantInt(5)), t, fl);
    b.setInsertPoint(t); b.createRet(b.createConstantInt(1));
    b.setInsertPoint(fl); b.createRet(b.createConstantInt(0));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm; CodeGenContext::addStandardPasses(pm, *tm); pm.run(mf);
    ObjectWriter w; w.addFunction(mf);
    EXPECT_TRUE(w.write("/tmp/ob.o")); std::remove("/tmp/ob.o");
}

TEST(ObjectWriterFuncTest, RelocationConstants) { EXPECT_EQ(R_X86_64_PC32, 2u); EXPECT_EQ(R_X86_64_PLT32, 4u); EXPECT_EQ(R_X86_64_32S, 11u); }
