#pragma once

#include <cstdint>
#include "Aurora/Target/TargetInstrInfo.h"

namespace aurora {

namespace AArch64 {
enum Opcode : uint16_t {
    MOVrr,
    MOVri,
    ADDrr,
    ADDri,
    SUBrr,
    SUBri,
    MULrr,
    SDIVrr,
    UDIVrr,
    ANDrr,
    ORRrr,
    EORrr,
    LSLrr,
    LSRrr,
    ASRrr,
    CMPrr,
    CMPri,
    CSETEQ,
    CSETNE,
    CSETLT,
    CSETLE,
    CSETGT,
    CSETGE,
    CSETLO,
    CSETLS,
    CSETHI,
    CSETHS,
    B,
    BEQ,
    BNE,
    BLT,
    BLE,
    BGT,
    BGE,
    BLO,
    BLS,
    BHI,
    BHS,
    BL,
    RET,
    LDRfi,
    STRfi,
    STPpre,
    LDPpost,
    MOVsp,
    SUBSPi,
    ADDSPi,
    BRK,
    NUM_OPS
};
} // namespace AArch64

class AArch64RegisterInfo;

class AArch64InstrInfo : public TargetInstrInfo {
public:
    explicit AArch64InstrInfo(const AArch64RegisterInfo& regInfo);

    [[nodiscard]] const MachineOpcodeDesc& get(unsigned opcode) const override;
    [[nodiscard]] unsigned getNumOpcodes() const override;

    [[nodiscard]] bool isMoveImmediate(const MachineInstr& mi, unsigned& dstReg, int64_t& val) const override;
    void copyPhysReg(MachineBasicBlock& mbb, MachineInstr* pos,
                     const Register& dst, const Register& src) const override;
    void storeRegToStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                             const Register& src, int frameIdx) const override;
    void loadRegFromStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                              const Register& dst, int frameIdx) const override;

private:
    void buildOpcodeTable();

    MachineOpcodeDesc opcodeTable_[AArch64::NUM_OPS];
};

} // namespace aurora
