#pragma once

#include <vector>
#include "Aurora/ADT/BitVector.h"
#include "Aurora/Target/TargetRegisterInfo.h"

namespace aurora {

class X86RegisterInfo : public TargetRegisterInfo {
public:
    X86RegisterInfo();

    [[nodiscard]] const RegisterClass& getRegClass(RegClass id) const override;
    [[nodiscard]] Register getFramePointer() const override;
    [[nodiscard]] Register getStackPointer() const override;
    [[nodiscard]] BitVector getCalleeSavedRegs() const override;
    [[nodiscard]] BitVector getCallerSavedRegs() const override;
    [[nodiscard]] const std::vector<unsigned>& getAllocOrder(RegClass rc) const override;
    [[nodiscard]] unsigned getNumRegs() const override;

    [[nodiscard]] static Register getReg(unsigned id);

    // x86 register IDs
    enum : unsigned {
        RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
        R8,  R9,  R10, R11, R12, R13, R14, R15,
        NUM_GPRS,
        XMM0 = NUM_GPRS, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
        XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
        NUM_REGS = XMM15 + 1
    };

    // Sub-register mappings
    [[nodiscard]] static unsigned get32Reg(unsigned reg64);

private:
    std::vector<Register> regs_;
    RegisterClass gpr8_, gpr16_, gpr32_, gpr64_, xmm128_;
    BitVector calleeSaved_, callerSaved_;
    std::vector<unsigned> allocOrderGPR64_;
    void buildRegisterSet();
};

} // namespace aurora

