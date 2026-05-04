#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/CodeGen/SelectionDAG.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/InstructionSelector.h"

using namespace aurora;

// Exercise SelectionDAG methods
TEST(FuncCoverTest, SelectionDAGMethods) {
    SelectionDAG dag;
    SmallVector<SDValue,4> ops;
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    dag.dagCombine();
    dag.legalize();
    auto* mbb = new MachineBasicBlock("t");
    dag.select(mbb);
    dag.schedule(mbb);
    delete mbb;
    SUCCEED();
}

// Exercise InstructionSelector
TEST(FuncCoverTest, InstructionSelectorRun) {
    auto tm = TargetMachine::createX86_64();
    SelectionDAG dag;
    SmallVector<SDValue,4> ops;
    dag.createNode(AIROpcode::Add, Type::getInt32Ty(), ops);
    MachineBasicBlock mbb("t");
    InstructionSelector isel(*tm);
    isel.run(dag, mbb);
    SUCCEED();
}

// Pipeline with double type
TEST(FuncCoverTest, DoubleTypePipeline) {
    auto m = std::make_unique<Module>("db");
    SmallVector<Type*,8> p = {Type::getDoubleTy(), Type::getDoubleTy()};
    auto* f = m->createFunction(new FunctionType(Type::getDoubleTy(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    b.createRet(b.createAdd(Type::getDoubleTy(), 0, 1));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
}

// Test with multiple blocks
TEST(FuncCoverTest, MultiBlockCFG) {
    auto m = std::make_unique<Module>("cfg");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    auto* e = f->getEntryBlock();
    auto* b1 = f->createBasicBlock("L1");
    auto* b2 = f->createBasicBlock("L2");
    AIRBuilder b(e);
    b.createCondBr(b.createICmp(ICmpCond::SLT, 0, b.createConstantInt(5)), b1, b2);
    b.setInsertPoint(b1); b.createRet(0);
    b.setInsertPoint(b2); b.createRet(b.createConstantInt(1));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
}

// Collector: explicit copy coalescing
TEST(FuncCoverTest, CopyCoalescingPipeline) {
    auto m = std::make_unique<Module>("cc2");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    unsigned r = b.createConstantInt(10);
    r = b.createAdd(Type::getInt64Ty(), r, 0);
    b.createRet(r);
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
}

// GEP simple test (pipeline)
TEST(FuncCoverTest, GEPSimple) {
    auto m = std::make_unique<Module>("g2");
    SmallVector<Type*,8> p = {Type::getInt64Ty()};
    auto* f = m->createFunction(new FunctionType(Type::getInt64Ty(), p), "f");
    AIRBuilder b(f->getEntryBlock());
    SmallVector<unsigned,4> idx = {0u};
    unsigned gep = b.createGEP(Type::getPointerTy(Type::getInt64Ty()), 0, idx);
    b.createRet(b.createLoad(Type::getInt64Ty(), gep));
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*f, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);
}
