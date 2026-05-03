#pragma once

#include <cstdint>
#include "Aurora/Target/TargetInstrInfo.h"

namespace aurora {

class X86RegisterInfo;

namespace X86 {
enum Opcode : uint16_t {
    // Moves
    MOV32rr, MOV32ri, MOV32rm, MOV32mr,
    MOV64rr, MOV64ri32, MOV64rm, MOV64mr,
    MOVSX32rr8, MOVZX32rr8,
    MOVSX64rr32,
    MOVSDrm, MOVSDmr, MOVAPDrm, MOVAPDmr,

    // Arithmetic (reg, reg)
    ADD32rr, ADD32ri, ADD32rm, ADD32mr,
    ADD64rr, ADD64ri8, ADD64ri32, ADD64rm, ADD64mr,
    SUB32rr, SUB32ri, SUB64rr, SUB64ri8, SUB64ri32,
    IMUL32rr, IMUL64rr,
    IDIV32r, IDIV64r,
    INC32r, INC64r, DEC32r, DEC64r,
    NEG32r, NEG64r,
    CQO,

    // Bitwise
    AND32rr, AND32ri, AND64rr, AND64ri8, AND64ri32,
    OR32rr,  OR32ri,  OR64rr,  OR64ri8,  OR64ri32,
    XOR32rr, XOR32ri, XOR64rr, XOR64ri8, XOR64ri32,
    NOT32r, NOT64r,
    SHL32rCL, SHR32rCL, SAR32rCL,
    SHL64rCL, SHR64rCL, SAR64rCL,
    SHL32ri, SHL64ri,
    SHR32ri, SHR64ri,
    SAR32ri, SAR64ri,

    // Comparison
    CMP32rr, CMP32ri, CMP64rr, CMP64ri8, CMP64ri32,
    TEST32rr, TEST64rr,

    // SetCC
    SETEr, SETNEr, SETLr, SETGr, SETLEr, SETGEr, SETAr, SETBEr,
    SETAEr,

    // Branches
    JMP_1, JMP_4,
    JE_1, JE_4, JNE_1, JNE_4,
    JL_1, JL_4, JG_1, JG_4,
    JLE_1, JLE_4, JGE_1, JGE_4,
    JAE_1, JAE_4, JBE_1, JBE_4,

    // Call / Return
    CALL64pcrel32, RETQ, RETIQ,

    // Stack
    PUSH64r, POP64r,
    LEA64r,

    // Extension
    MOVSX32rr8_op,
    MOVZX32rr8_op,

    // FP
    ADDSDrr, SUBSDrr, MULSDrr, DIVSDrr,
    UCOMISDrr,
    CVTSI2SDrr, CVTTSD2SIrr,

    // Misc
    NOP,
    NUM_OPS
};
} // namespace X86

class X86InstrInfo : public TargetInstrInfo {
public:
    explicit X86InstrInfo(const X86RegisterInfo& regInfo);

    [[nodiscard]] const MachineOpcodeDesc& get(unsigned opcode) const override;
    [[nodiscard]] unsigned getNumOpcodes() const override;

    [[nodiscard]] bool isMoveImmediate(const MachineInstr& mi, unsigned& dstReg, int64_t& val) const override;
    void copyPhysReg(MachineBasicBlock& mbb, MachineInstr* pos,
                     const Register& dst, const Register& src) const override;
    void storeRegToStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                             const Register& src, int frameIdx) const override;
    void loadRegFromStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                              const Register& dst, int frameIdx) const override;

    [[nodiscard]] unsigned getMoveOpcode(unsigned srcSize, unsigned dstSize) const;
    [[nodiscard]] unsigned getArithOpcode(unsigned opType, unsigned size, bool isImm) const;

private:
    void buildOpcodeTable();
    MachineOpcodeDesc opcodeTable_[X86::NUM_OPS];
};

} // namespace aurora

