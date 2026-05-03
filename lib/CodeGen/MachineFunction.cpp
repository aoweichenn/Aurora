#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Air/Function.h"

namespace aurora {

MachineFunction::MachineFunction(Function& airFunc, const TargetMachine& TM)
    : airFunc_(airFunc), target_(TM), nextVReg_(0), nextFrameIndex_(-1) {
    // Create virtual registers for function parameters
    const auto fnTy = airFunc.getFunctionType();
    for (unsigned i = 0; i < fnTy->getNumParams(); ++i) {
        createVirtualRegister(fnTy->getParamTypes()[i]);
    }
}

MachineFunction::~MachineFunction() = default;

MachineBasicBlock* MachineFunction::createBasicBlock(const std::string& name) {
    auto mbb = std::make_unique<MachineBasicBlock>(name);
    MachineBasicBlock* ptr = mbb.get();
    mbb->setParent(this);
    blocks_.push_back(std::move(mbb));
    return ptr;
}

unsigned MachineFunction::createVirtualRegister(Type* ty) {
    const unsigned vreg = nextVReg_++;
    vregTypes_.resize(nextVReg_, nullptr);
    vregTypes_[vreg] = ty;
    return vreg;
}

Type* MachineFunction::getVRegType(const unsigned vreg) const noexcept {
    if (vreg >= vregTypes_.size()) return nullptr;
    return vregTypes_[vreg];
}

int MachineFunction::createStackSlot(const unsigned size, const unsigned alignment) {
    const int idx = ++nextFrameIndex_;
    stackObjects_.push_back({size, alignment, idx});
    return idx;
}

} // namespace aurora
