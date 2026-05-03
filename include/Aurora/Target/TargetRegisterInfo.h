#pragma once

#include <string>
#include <vector>
#include "Aurora/ADT/BitVector.h"

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

    [[nodiscard]] bool isGeneralPurpose() const noexcept;
    [[nodiscard]] bool isFloatingPoint()  const noexcept;
    [[nodiscard]] bool isFlag()           const noexcept;
};

class RegisterClass {
public:
    RegisterClass() = default;
    RegisterClass(std::string name, std::vector<Register> regs);

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] const std::vector<Register>& getRegisters() const noexcept { return regs_; }
    [[nodiscard]] bool contains(const Register& reg) const;
    [[nodiscard]] unsigned getSizeInBits() const noexcept;
    [[nodiscard]] unsigned getNumRegs() const noexcept { return static_cast<unsigned>(regs_.size()); }
    [[nodiscard]] const Register& operator[](const unsigned i) const { return regs_[i]; }

private:
    std::string name_;
    std::vector<Register> regs_;
};

class TargetRegisterInfo {
public:
    virtual ~TargetRegisterInfo() = default;

    [[nodiscard]] virtual const RegisterClass& getRegClass(RegClass id) const = 0;
    [[nodiscard]] virtual Register getFramePointer() const = 0;
    [[nodiscard]] virtual Register getStackPointer() const = 0;
    [[nodiscard]] virtual BitVector getCalleeSavedRegs() const = 0;
    [[nodiscard]] virtual BitVector getCallerSavedRegs() const = 0;
    [[nodiscard]] virtual const std::vector<unsigned>& getAllocOrder(RegClass rc) const = 0;
    [[nodiscard]] virtual unsigned getNumRegs() const = 0;
};

} // namespace aurora

