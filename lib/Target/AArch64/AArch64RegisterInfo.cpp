#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include <utility>

namespace aurora {

static const char* GPR64Names[] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp",
};

AArch64RegisterInfo::AArch64RegisterInfo() {
    buildRegisterSet();
}

const RegisterClass& AArch64RegisterInfo::getRegClass(const RegClass id) const {
    switch (id) {
    case RegClass::GPR64:
        return gpr64_;
    default:
        return gpr64_;
    }
}

Register AArch64RegisterInfo::getFramePointer() const { return regs_[FP]; }

Register AArch64RegisterInfo::getStackPointer() const { return regs_[SP]; }

BitVector AArch64RegisterInfo::getCalleeSavedRegs() const { return calleeSaved_; }

BitVector AArch64RegisterInfo::getCallerSavedRegs() const { return callerSaved_; }

const std::vector<unsigned>& AArch64RegisterInfo::getAllocOrder(const RegClass rc) const {
    static std::vector<unsigned> defaultOrder;
    if (rc == RegClass::GPR64) return allocOrderGPR64_;
    return defaultOrder;
}

unsigned AArch64RegisterInfo::getNumRegs() const { return NUM_REGS; }

Register AArch64RegisterInfo::getReg(const unsigned id) {
    static AArch64RegisterInfo ri;
    if (id >= NUM_REGS) return ri.regs_[X0];
    return ri.regs_[id];
}

void AArch64RegisterInfo::buildRegisterSet() {
    regs_.reserve(NUM_REGS);
    std::vector<Register> gpr64;
    for (unsigned i = 0; i < NUM_REGS; ++i) {
        const Register reg{i, GPR64Names[i], 64, RegClass::GPR64};
        regs_.push_back(reg);
        gpr64.push_back(reg);
    }

    gpr64_ = RegisterClass("GPR64", std::move(gpr64));

    calleeSaved_ = BitVector(NUM_REGS);
    for (const unsigned reg : {X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, FP, LR})
        calleeSaved_.set(reg);

    callerSaved_ = BitVector(NUM_REGS);
    for (const unsigned reg : {X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15, X16, X17, X18})
        callerSaved_.set(reg);

    allocOrderGPR64_ = {X9, X10, X11, X12, X13, X14, X15, X16, X17, X8, X0, X1, X2, X3, X4, X5, X6, X7};
}

} // namespace aurora
