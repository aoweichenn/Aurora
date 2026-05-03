#pragma once

#include <cstdint>

namespace aurora {

class MachineInstr;
class MachineBasicBlock;
class Register;

struct MachineOpcodeDesc {
    uint16_t opcode;
    const char* asmString;
    unsigned numOperands;
    bool isTerminator : 1;
    bool isBranch : 1;
    bool isCall : 1;
    bool isReturn : 1;
    bool isMove : 1;
    bool isCompare : 1;
    bool hasSideEffects : 1;
    unsigned reserved : 9;
};

class TargetInstrInfo {
public:
    virtual ~TargetInstrInfo() = default;

    [[nodiscard]] virtual const MachineOpcodeDesc& get(unsigned opcode) const = 0;
    [[nodiscard]] virtual unsigned getNumOpcodes() const = 0;

    [[nodiscard]] virtual bool isMoveImmediate(const MachineInstr& mi, unsigned& dstReg, int64_t& val) const = 0;
    virtual void copyPhysReg(MachineBasicBlock& mbb, MachineInstr* pos,
                             const Register& dst, const Register& src) const = 0;
    virtual void storeRegToStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                                     const Register& src, int frameIdx) const = 0;
    virtual void loadRegFromStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                                      const Register& dst, int frameIdx) const = 0;
};

} // namespace aurora

