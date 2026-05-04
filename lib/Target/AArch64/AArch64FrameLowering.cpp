#include "Aurora/Target/AArch64/AArch64FrameLowering.h"
#include "Aurora/Target/AArch64/AArch64InstrInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

static int alignedStackSize(const MachineFunction& mf) {
    int totalStack = 0;
    for (const auto& so : mf.getStackObjects())
        totalStack += static_cast<int>(so.size);
    const int rem = totalStack & 15;
    return rem == 0 ? totalStack : totalStack + (16 - rem);
}

void AArch64FrameLowering::emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const {
    MachineInstr* first = entry.getFirst();

    auto* saveFP = new MachineInstr(AArch64::STPpre);
    if (first) entry.insertBefore(first, saveFP); else entry.pushBack(saveFP);

    auto* setFP = new MachineInstr(AArch64::MOVsp);
    entry.insertAfter(saveFP, setFP);

    const int stackSize = alignedStackSize(mf);
    if (stackSize > 0) {
        auto* subSP = new MachineInstr(AArch64::SUBSPi);
        subSP->addOperand(MachineOperand::createImm(stackSize));
        entry.insertAfter(setFP, subSP);
    }
}

void AArch64FrameLowering::emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const {
    MachineInstr* term = ret.getLast();
    while (term && term->getOpcode() != AArch64::RET)
        term = term->getPrev();
    if (!term) return;

    const int stackSize = alignedStackSize(mf);
    if (stackSize > 0) {
        auto* addSP = new MachineInstr(AArch64::ADDSPi);
        addSP->addOperand(MachineOperand::createImm(stackSize));
        ret.insertBefore(term, addSP);
    }

    auto* restoreFP = new MachineInstr(AArch64::LDPpost);
    ret.insertBefore(term, restoreFP);
}

int AArch64FrameLowering::getFrameIndexReference(const MachineFunction& /*mf*/, const int frameIdx, unsigned& outReg) const {
    outReg = AArch64RegisterInfo::FP;
    return -(frameIdx + 1) * 8;
}

bool AArch64FrameLowering::hasFP(const MachineFunction& /*mf*/) const {
    return true;
}

} // namespace aurora
