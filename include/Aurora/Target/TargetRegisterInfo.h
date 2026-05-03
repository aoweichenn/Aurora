#ifndef AURORA_TARGET_TARGETREGISTERINFO_H
#define AURORA_TARGET_TARGETREGISTERINFO_H

#include "Aurora/ADT/BitVector.h"
#include <string>
#include <vector>

namespace aurora {

class DataLayout;

enum class RegClass : uint8_t {
    GPR8, GPR16, GPR32, GPR64,
    XMM128, YMM256,
    Flag,
    Unknown
};

class Register {
public:
    unsigned id;
    std::string name;
    unsigned bitWidth;
    RegClass regClass;

    bool isGeneralPurpose() const noexcept;
    bool isFloatingPoint()  const noexcept;
    bool isFlag()           const noexcept;
};

class RegisterClass {
public:
    RegisterClass() = default;
    RegisterClass(std::string name, std::vector<Register> regs);

    const std::string& getName() const noexcept { return name_; }
    const std::vector<Register>& getRegisters() const noexcept { return regs_; }
    bool contains(const Register& reg) const;
    unsigned getSizeInBits() const noexcept;
    unsigned getNumRegs() const noexcept { return static_cast<unsigned>(regs_.size()); }
    const Register& operator[](const unsigned i) const { return regs_[i]; }

private:
    std::string name_;
    std::vector<Register> regs_;
};

class TargetRegisterInfo {
public:
    virtual ~TargetRegisterInfo() = default;

    virtual const RegisterClass& getRegClass(RegClass id) const = 0;
    virtual Register getFramePointer() const = 0;
    virtual Register getStackPointer() const = 0;
    virtual BitVector getCalleeSavedRegs() const = 0;
    virtual BitVector getCallerSavedRegs() const = 0;
    virtual const std::vector<unsigned>& getAllocOrder(RegClass rc) const = 0;
    virtual unsigned getNumRegs() const = 0;
};

} // namespace aurora

#endif // AURORA_TARGET_TARGETREGISTERINFO_H
