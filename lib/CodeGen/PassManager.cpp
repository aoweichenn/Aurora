#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/InstructionSelector.h"
#include "Aurora/CodeGen/RegisterAllocator.h"
#include "Aurora/CodeGen/PrologueEpilogueInserter.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Target/TargetMachine.h"

namespace aurora {

void PassManager::addPass(std::unique_ptr<CodeGenPass> pass) {
    passes_.push_back(std::move(pass));
}

void PassManager::run(MachineFunction& mf) {
    for (auto& pass : passes_)
        pass->run(mf);
}

// Standard codegen pass implementations
namespace {
class AIRToMachineIRPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        // Translate AIR basic blocks to MachineBasicBlocks
        auto& airFunc = mf.getAIRFunction();
        for (auto& airBB : airFunc.getBlocks()) {
            auto* mbb = mf.createBasicBlock(airBB->getName());
            // Copy AIR instructions to machine instructions
            AIRInstruction* inst = airBB->getFirst();
            while (inst) {
                uint16_t opcode = static_cast<uint16_t>(inst->getOpcode());
                auto* mi = new MachineInstr(opcode);
                for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
                    unsigned vreg = inst->getOperand(i);
                    if (vreg != ~0U)
                        mi->addOperand(MachineOperand::createVReg(vreg));
                }
                if (inst->hasResult()) {
                    mi->addOperand(MachineOperand::createVReg(inst->getDestVReg()));
                }
                mbb->pushBack(mi);
                inst = inst->getNext();
            }
        }

        // Set up CFG edges
        auto& airBlocks = airFunc.getBlocks();
        auto& mbbList = mf.getBlocks();
        for (size_t i = 0; i < airBlocks.size(); ++i) {
            for (auto* succ : airBlocks[i]->getSuccessors()) {
                // Find corresponding MBB
                for (size_t j = 0; j < airBlocks.size(); ++j) {
                    if (airBlocks[j].get() == succ) {
                        mbbList[i]->addSuccessor(mbbList[j].get());
                        break;
                    }
                }
            }
        }
    }
    const char* getName() const override { return "AIR to MachineIR"; }
};

class PeepholePass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        // Peephole optimizations: mov a,a → remove; xor a,a → a=0; etc.
    }
    const char* getName() const override { return "Peephole"; }
};

class PhiEliminationPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        // Eliminate phi nodes by inserting copies on predecessor edges
    }
    const char* getName() const override { return "Phi Elimination"; }
};

class RegisterCoalescerPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        // Merge copy-connected virtual registers
    }
    const char* getName() const override { return "Register Coalescer"; }
};

class BranchFoldingPass : public CodeGenPass {
public:
    void run(MachineFunction& mf) override {
        // Merge consecutive branches to same target
    }
    const char* getName() const override { return "Branch Folder"; }
};
} // anonymous namespace

void CodeGenContext::addStandardPasses(PassManager& pm, TargetMachine& tm) {
    pm.addPass(std::make_unique<AIRToMachineIRPass>());
    pm.addPass(std::make_unique<PeepholePass>());
    pm.addPass(std::make_unique<PhiEliminationPass>());
    pm.addPass(std::make_unique<RegisterCoalescerPass>());
    pm.addPass(std::make_unique<BranchFoldingPass>());
}

CodeGenContext::CodeGenContext(TargetMachine& tm, Module& module)
    : target_(tm), module_(module) {}

void CodeGenContext::run() {
    PassManager pm;
    addStandardPasses(pm, target_);

    for (auto& fn : module_.getFunctions()) {
        MachineFunction mf(*fn, target_);
        pm.run(mf);
    }
}

} // namespace aurora
