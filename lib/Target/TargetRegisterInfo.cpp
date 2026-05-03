#include "Aurora/Target/TargetRegisterInfo.h"

namespace aurora {

bool Register::isGeneralPurpose() const noexcept {
    return regClass == RegClass::GPR8 || regClass == RegClass::GPR16 ||
           regClass == RegClass::GPR32 || regClass == RegClass::GPR64;
}
bool Register::isFloatingPoint() const noexcept {
    return regClass == RegClass::XMM128 || regClass == RegClass::YMM256;
}
bool Register::isFlag() const noexcept { return regClass == RegClass::Flag; }

RegisterClass::RegisterClass(std::string name, std::vector<Register> regs)
    : name_(std::move(name)), regs_(std::move(regs)) {}

bool RegisterClass::contains(const Register& reg) const {
    return reg.regClass == regs_[0].regClass;
}

unsigned RegisterClass::getSizeInBits() const noexcept {
    if (regs_.empty()) return 0;
    return regs_[0].bitWidth;
}

} // namespace aurora
