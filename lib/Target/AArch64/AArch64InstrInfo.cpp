#include "Aurora/Target/AArch64/AArch64InstrInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include <cstring>

namespace aurora {

AArch64InstrInfo::AArch64InstrInfo(const AArch64RegisterInfo& /*regInfo*/) {
    buildOpcodeTable();
}

const MachineOpcodeDesc& AArch64InstrInfo::get(const unsigned opcode) const {
    static const MachineOpcodeDesc invalid = {};
    if (opcode >= AArch64::NUM_OPS) return invalid;
    return opcodeTable_[opcode];
}

unsigned AArch64InstrInfo::getNumOpcodes() const { return AArch64::NUM_OPS; }

bool AArch64InstrInfo::isMoveImmediate(const MachineInstr& mi, unsigned& dstReg, int64_t& val) const {
    if (mi.getOpcode() != AArch64::MOVri || mi.getNumOperands() < 2)
        return false;
    if (!mi.getOperand(0).isImm() || !mi.getOperand(1).isReg())
        return false;
    val = mi.getOperand(0).getImm();
    dstReg = mi.getOperand(1).getReg();
    return true;
}

void AArch64InstrInfo::copyPhysReg(MachineBasicBlock& mbb, MachineInstr* pos,
                                   const Register& dst, const Register& src) const {
    auto* mi = new MachineInstr(AArch64::MOVrr);
    mi->addOperand(MachineOperand::createReg(src.id));
    mi->addOperand(MachineOperand::createReg(dst.id));
    if (pos) mbb.insertBefore(pos, mi); else mbb.pushBack(mi);
}

void AArch64InstrInfo::storeRegToStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                                           const Register& src, const int frameIdx) const {
    auto* mi = new MachineInstr(AArch64::STRfi);
    mi->addOperand(MachineOperand::createReg(src.id));
    mi->addOperand(MachineOperand::createFrameIndex(frameIdx));
    if (pos) mbb.insertBefore(pos, mi); else mbb.pushBack(mi);
}

void AArch64InstrInfo::loadRegFromStackSlot(MachineBasicBlock& mbb, MachineInstr* pos,
                                            const Register& dst, const int frameIdx) const {
    auto* mi = new MachineInstr(AArch64::LDRfi);
    mi->addOperand(MachineOperand::createFrameIndex(frameIdx));
    mi->addOperand(MachineOperand::createReg(dst.id));
    if (pos) mbb.insertBefore(pos, mi); else mbb.pushBack(mi);
}

void AArch64InstrInfo::buildOpcodeTable() {
    memset(opcodeTable_, 0, sizeof(opcodeTable_));

    auto setDesc = [this](const AArch64::Opcode op, const char* asmStr, const unsigned numOps,
                          const bool term, const bool branch, const bool call, const bool ret, const bool move, const bool cmp, const bool side) {
        auto& desc = opcodeTable_[op];
        desc.opcode = op;
        desc.asmString = asmStr;
        desc.numOperands = numOps;
        desc.isTerminator = term;
        desc.isBranch = branch;
        desc.isCall = call;
        desc.isReturn = ret;
        desc.isMove = move;
        desc.isCompare = cmp;
        desc.hasSideEffects = side;
    };

    setDesc(AArch64::MOVrr, "mov\t$dst, $src", 2, false, false, false, false, true, false, false);
    setDesc(AArch64::MOVri, "mov\t$dst, #$imm", 2, false, false, false, false, true, false, false);
    setDesc(AArch64::ADDrr, "add\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::ADDri, "add\t$dst, $lhs, #$imm", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::SUBrr, "sub\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::SUBri, "sub\t$dst, $lhs, #$imm", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::MULrr, "mul\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::SDIVrr, "sdiv\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::UDIVrr, "udiv\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::ANDrr, "and\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::ORRrr, "orr\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::EORrr, "eor\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::LSLrr, "lsl\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::LSRrr, "lsr\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::ASRrr, "asr\t$dst, $lhs, $rhs", 3, false, false, false, false, false, false, false);
    setDesc(AArch64::CMPrr, "cmp\t$lhs, $rhs", 2, false, false, false, false, false, true, false);
    setDesc(AArch64::CMPri, "cmp\t$lhs, #$imm", 2, false, false, false, false, false, true, false);
    setDesc(AArch64::CSETEQ, "cset\t$dst, eq", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETNE, "cset\t$dst, ne", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETLT, "cset\t$dst, lt", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETLE, "cset\t$dst, le", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETGT, "cset\t$dst, gt", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETGE, "cset\t$dst, ge", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETLO, "cset\t$dst, lo", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETLS, "cset\t$dst, ls", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETHI, "cset\t$dst, hi", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::CSETHS, "cset\t$dst, hs", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::B, "b\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BEQ, "b.eq\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BNE, "b.ne\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BLT, "b.lt\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BLE, "b.le\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BGT, "b.gt\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BGE, "b.ge\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BLO, "b.lo\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BLS, "b.ls\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BHI, "b.hi\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BHS, "b.hs\t$dst", 1, true, true, false, false, false, false, true);
    setDesc(AArch64::BL, "bl\t$dst", 1, false, false, true, false, false, false, true);
    setDesc(AArch64::RET, "ret", 0, true, false, false, true, false, false, true);
    setDesc(AArch64::LDRfi, "ldr\t$dst, [$base]", 2, false, false, false, false, true, false, false);
    setDesc(AArch64::STRfi, "str\t$src, [$base]", 2, false, false, false, false, false, false, true);
    setDesc(AArch64::STPpre, "stp\tx29, x30, [sp, #-16]!", 0, false, false, false, false, false, false, true);
    setDesc(AArch64::LDPpost, "ldp\tx29, x30, [sp], #16", 0, false, false, false, false, false, false, true);
    setDesc(AArch64::MOVsp, "mov\tx29, sp", 0, false, false, false, false, true, false, false);
    setDesc(AArch64::SUBSPi, "sub\tsp, sp, #$imm", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::ADDSPi, "add\tsp, sp, #$imm", 1, false, false, false, false, false, false, false);
    setDesc(AArch64::BRK, "brk\t#0", 0, true, false, false, false, false, false, true);
}

} // namespace aurora
