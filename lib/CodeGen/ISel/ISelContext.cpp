#include "Aurora/CodeGen/ISel/ISelContext.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Instruction.h"

namespace aurora {

ISelContext::ISelContext(MachineFunction& mf) : mf_(mf) {
    // Initial scan to build initial store map (simple mem2reg info)
    const auto& airFunc = mf.getAIRFunction();
    for (auto& bb : airFunc.getBlocks()) {
        const AIRInstruction* inst = bb->getFirst();
        while (inst) {
            if (inst->getOpcode() == AIROpcode::Store && inst->getNumOperands() >= 2) {
                recordStore(inst->getOperand(1), inst->getOperand(0));
            }
            inst = inst->getNext();
        }
    }
}

void ISelContext::recordConstant(unsigned vreg, int64_t value) {
    constantMap_[vreg] = value;
}

bool ISelContext::getConstant(unsigned vreg, int64_t& outValue) const {
    auto it = constantMap_.find(vreg);
    if (it != constantMap_.end()) {
        outValue = it->second;
        return true;
    }
    return false;
}

void ISelContext::recordFrameIndex(unsigned vreg, int fi) {
    frameIndexMap_[vreg] = fi;
}

bool ISelContext::getFrameIndex(unsigned vreg, int& outFI) const {
    auto it = frameIndexMap_.find(vreg);
    if (it != frameIndexMap_.end()) {
        outFI = it->second;
        return true;
    }
    return false;
}

void ISelContext::recordStore(unsigned ptrVreg, unsigned valVreg) {
    storeMap_[ptrVreg] = valVreg;
}

bool ISelContext::getStoredValue(unsigned ptrVreg, unsigned& outValVreg) const {
    auto it = storeMap_.find(ptrVreg);
    if (it != storeMap_.end()) {
        outValVreg = it->second;
        return true;
    }
    return false;
}

Type* ISelContext::getVRegType(unsigned vreg) const {
    // Try MachineFunction first
    if (Type* ty = mf_.getVRegType(vreg)) return ty;
    
    // Fallback to AIR function scanning (from old code)
    const auto& airFunc = mf_.getAIRFunction();
    for (auto& airBB : airFunc.getBlocks()) {
        const AIRInstruction* airInst = airBB->getFirst();
        while (airInst) {
            if (airInst->hasResult() && airInst->getDestVReg() == vreg)
                return airInst->getType();
            airInst = airInst->getNext();
        }
    }
    return nullptr;
}

} // namespace aurora
