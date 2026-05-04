#include <gtest/gtest.h>
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/Passes/PassFactories.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/X86/X86InstrInfo.h"

using namespace aurora;

namespace {

unsigned appendResult(Function& fn, BasicBlock& bb, AIRInstruction* inst) {
    const unsigned vreg = fn.nextVReg();
    inst->setDestVReg(vreg);
    bb.pushBack(inst);
    return vreg;
}

void runAIRToISel(Function& fn) {
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(fn, *tm);
    PassManager pm;
    pm.addPass(createAIRToMachineIRPass());
    pm.addPass(createInstructionSelectionPass());
    pm.run(mf);
    ASSERT_GT(mf.getBlocks().size(), 0u);
}

} // namespace

TEST(PassManagerDeepCoverageTest, LowersAdvancedIROps) {
    Module module("deep-isel");
    SmallVector<Type*, 8> params = {Type::getInt64Ty(), Type::getInt64Ty()};
    auto* fn = module.createFunction(new FunctionType(Type::getInt64Ty(), params), "advanced");
    auto* entry = fn->getEntryBlock();
    auto* caseOne = fn->createBasicBlock("case.one");
    auto* caseTwo = fn->createBasicBlock("case.two");
    auto* def = fn->createBasicBlock("default");

    const unsigned c8 = appendResult(*fn, *entry, AIRInstruction::createConstantInt(Type::getInt8Ty(), 7));
    const unsigned c16 = appendResult(*fn, *entry, AIRInstruction::createConstantInt(Type::getInt16Ty(), 9));
    const unsigned sx8 = appendResult(*fn, *entry, AIRInstruction::createSExt(Type::getInt64Ty(), c8));
    const unsigned sx16 = appendResult(*fn, *entry, AIRInstruction::createSExt(Type::getInt64Ty(), c16));
    const unsigned srem = appendResult(*fn, *entry, AIRInstruction::createSRem(Type::getInt64Ty(), sx8, sx16));
    const unsigned urem = appendResult(*fn, *entry, AIRInstruction::createURem(Type::getInt64Ty(), sx16, sx8));

    SmallVector<unsigned, 4> structIndices = {0u};
    const unsigned extracted = appendResult(*fn, *entry, AIRInstruction::createExtractValue(Type::getInt64Ty(), srem, structIndices));
    const unsigned inserted = appendResult(*fn, *entry, AIRInstruction::createInsertValue(Type::getInt64Ty(), extracted, urem, structIndices));
    const unsigned global = appendResult(*fn, *entry, AIRInstruction::createGlobalAddress(Type::getPointerTy(Type::getInt64Ty()), "global_value"));
    const unsigned folded = appendResult(*fn, *entry, AIRInstruction::createAdd(Type::getInt64Ty(), inserted, global));

    SmallVector<std::pair<int64_t, BasicBlock*>, 8> cases = {{1, caseOne}, {2, caseTwo}};
    entry->pushBack(AIRInstruction::createSwitch(Type::getVoidTy(), folded, def, cases));
    entry->addSuccessor(caseOne);
    entry->addSuccessor(caseTwo);
    entry->addSuccessor(def);

    caseOne->pushBack(AIRInstruction::createRet(srem));
    caseTwo->pushBack(AIRInstruction::createRet(urem));
    def->pushBack(AIRInstruction::createRet(folded));

    runAIRToISel(*fn);
}

TEST(PassManagerDeepCoverageTest, LowersVarArgCallWithStackArguments) {
    Module module("calls");
    SmallVector<Type*, 8> calleeParams = {Type::getInt64Ty()};
    auto* callee = module.createFunction(new FunctionType(Type::getInt64Ty(), calleeParams, true), "vararg_callee");
    callee->getEntryBlock()->pushBack(AIRInstruction::createRet(0));

    SmallVector<Type*, 8> callerParams = {Type::getInt64Ty(), Type::getInt64Ty(), Type::getInt64Ty(), Type::getInt64Ty(),
                                          Type::getInt64Ty(), Type::getInt64Ty(), Type::getInt64Ty(), Type::getInt64Ty()};
    auto* caller = module.createFunction(new FunctionType(Type::getInt64Ty(), callerParams), "caller");
    auto* entry = caller->getEntryBlock();
    SmallVector<unsigned, 8> args = {0, 1, 2, 3, 4, 5, 6, 7};
    const unsigned result = appendResult(*caller, *entry, AIRInstruction::createCall(Type::getInt64Ty(), callee, args));
    entry->pushBack(AIRInstruction::createRet(result));

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*caller, *tm);
    PassManager pm;
    pm.addPass(createAIRToMachineIRPass());
    pm.addPass(createInstructionSelectionPass());
    pm.run(mf);

    bool sawPush = false;
    bool sawVarArgZero = false;
    bool sawStackCleanup = false;
    for (auto& mbb : mf.getBlocks()) {
        for (MachineInstr* mi = mbb->getFirst(); mi; mi = mi->getNext()) {
            sawPush = sawPush || mi->getOpcode() == X86::PUSH64r;
            sawVarArgZero = sawVarArgZero || mi->getOpcode() == X86::XOR32rr;
            sawStackCleanup = sawStackCleanup || mi->getOpcode() == X86::ADD64ri32;
        }
    }
    EXPECT_TRUE(sawPush);
    EXPECT_TRUE(sawVarArgZero);
    EXPECT_TRUE(sawStackCleanup);
}

TEST(PassManagerDeepCoverageTest, BranchFolderThreadsAndRemovesFallthrough) {
    Module module("branch-fold");
    SmallVector<Type*, 8> params;
    auto* fn = module.createFunction(new FunctionType(Type::getVoidTy(), params), "fold");
    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    auto* first = mf.createBasicBlock("first");
    auto* last = mf.createBasicBlock("last");
    auto* middle = mf.createBasicBlock("middle");

    auto* firstJump = new MachineInstr(X86::JE_1);
    firstJump->addOperand(MachineOperand::createMBB(middle));
    firstJump->setFlags(MachineInstr::MIF_BRANCH | MachineInstr::MIF_TERMINATOR);
    first->pushBack(firstJump);

    auto* middleJump = new MachineInstr(X86::JMP_1);
    middleJump->addOperand(MachineOperand::createMBB(last));
    middleJump->setFlags(MachineInstr::MIF_BRANCH | MachineInstr::MIF_TERMINATOR);
    middle->pushBack(middleJump);

    auto branchFold = createBranchFoldingPass();
    branchFold->run(mf);
    ASSERT_NE(first->getLast(), nullptr);
    EXPECT_EQ(first->getLast()->getOperand(0).getMBB(), last);

    auto* fallthrough = new MachineInstr(X86::JMP_1);
    fallthrough->addOperand(MachineOperand::createMBB(last));
    fallthrough->setFlags(MachineInstr::MIF_BRANCH | MachineInstr::MIF_TERMINATOR);
    first->pushBack(fallthrough);
    branchFold->run(mf);
    EXPECT_NE(first->getLast(), fallthrough);
}
