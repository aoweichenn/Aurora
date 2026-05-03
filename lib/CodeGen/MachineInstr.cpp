#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

MachineOperand MachineOperand::createReg(unsigned regId) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_Register;
    mo.regId = regId;
    return mo;
}
MachineOperand MachineOperand::createVReg(unsigned vreg) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_VirtualReg;
    mo.vregId = vreg;
    return mo;
}
MachineOperand MachineOperand::createImm(int64_t val) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_Immediate;
    mo.immVal = val;
    return mo;
}
MachineOperand MachineOperand::createMBB(MachineBasicBlock* mbb) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_MBB;
    mo.mbb = mbb;
    return mo;
}
MachineOperand MachineOperand::createFrameIndex(int idx) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_FrameIndex;
    mo.frameIdx = idx;
    return mo;
}

unsigned MachineOperand::getReg() const noexcept { return regId; }
unsigned MachineOperand::getVirtualReg() const noexcept { return vregId; }
int64_t MachineOperand::getImm() const noexcept { return immVal; }
MachineBasicBlock* MachineOperand::getMBB() const noexcept { return mbb; }
int MachineOperand::getFrameIndex() const noexcept { return frameIdx; }
bool MachineOperand::isReg() const noexcept { return kind_ == MachineOperandKind::MO_Register; }
bool MachineOperand::isVReg() const noexcept { return kind_ == MachineOperandKind::MO_VirtualReg; }
bool MachineOperand::isImm() const noexcept { return kind_ == MachineOperandKind::MO_Immediate; }

MachineInstr::MachineInstr(uint16_t opcode)
    : opcode_(opcode), parent_(nullptr), next_(nullptr), prev_(nullptr) {}

MachineInstr::~MachineInstr() = default;

MachineOperand& MachineInstr::getOperand(unsigned i) { return operands_[i]; }
const MachineOperand& MachineInstr::getOperand(unsigned i) const { return operands_[i]; }

void MachineInstr::addOperand(MachineOperand mo) { operands_.push_back(mo); }
void MachineInstr::setOperand(unsigned i, MachineOperand mo) { operands_[i] = mo; }

bool MachineInstr::isTerminator() const noexcept { return false; }
bool MachineInstr::isBranch() const noexcept { return false; }
bool MachineInstr::isReturn() const noexcept { return false; }
bool MachineInstr::isCall() const noexcept { return false; }
bool MachineInstr::isMove() const noexcept { return false; }

} // namespace aurora
