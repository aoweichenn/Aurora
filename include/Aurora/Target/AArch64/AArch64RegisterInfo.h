#pragma once

#include <vector>
#include "Aurora/ADT/BitVector.h"
#include "Aurora/Target/TargetRegisterInfo.h"

namespace aurora {

class AArch64RegisterInfo : public TargetRegisterInfo {
public:
    AArch64RegisterInfo();

    [[nodiscard]] const RegisterClass& getRegClass(RegClass id) const override;
    [[nodiscard]] Register getFramePointer() const override;
    [[nodiscard]] Register getStackPointer() const override;
    [[nodiscard]] BitVector getCalleeSavedRegs() const override;
    [[nodiscard]] BitVector getCallerSavedRegs() const override;
    [[nodiscard]] const std::vector<unsigned>& getAllocOrder(RegClass rc) const override;
    [[nodiscard]] unsigned getNumRegs() const override;

    [[nodiscard]] static Register getReg(unsigned id);

    enum : unsigned {
        X0, X1, X2, X3, X4, X5, X6, X7,
        X8, X9, X10, X11, X12, X13, X14, X15,
        X16, X17, X18, X19, X20, X21, X22, X23,
        X24, X25, X26, X27, X28, X29, X30, SP,
        FP = X29,
        LR = X30,
        NUM_REGS
    };

private:
    std::vector<Register> regs_;
    RegisterClass gpr64_;
    BitVector calleeSaved_;
    BitVector callerSaved_;
    std::vector<unsigned> allocOrderGPR64_;

    void buildRegisterSet();
};

} // namespace aurora
