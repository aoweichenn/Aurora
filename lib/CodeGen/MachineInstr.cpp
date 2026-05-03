#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

MachineOperand MachineOperand::createReg(const unsigned regId) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_Register;
    mo.regId = regId;
    return mo;
}
MachineOperand MachineOperand::createVReg(const unsigned vreg) {
    MachineOperand mo;
    mo.kind_ = MachineOperandKind::MO_VirtualReg;
    mo.vregId = vreg;
    return mo;
}
MachineOperand MachineOperand::createImm(const int64_t val) {
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
MachineOperand MachineOperand::createFrameIndex(const int idx) {
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

MachineInstr::MachineInstr(const uint16_t opcode)
    : opcode_(opcode), parent_(nullptr), next_(nullptr), prev_(nullptr) {}

MachineInstr::~MachineInstr() = default;

MachineOperand& MachineInstr::getOperand(const unsigned i) { return operands_[i]; }
const MachineOperand& MachineInstr::getOperand(const unsigned i) const { return operands_[i]; }

void MachineInstr::addOperand(const MachineOperand mo) { operands_.push_back(mo); }
void MachineInstr::setOperand(const unsigned i, const MachineOperand mo) { operands_[i] = mo; }

bool MachineInstr::isTerminator() const noexcept { return false; }
bool MachineInstr::isBranch() const noexcept { return false; }
bool MachineInstr::isReturn() const noexcept { return false; }
bool MachineInstr::isCall() const noexcept { return false; }
bool MachineInstr::isMove() const noexcept { return false; }

} // namespace aurora
