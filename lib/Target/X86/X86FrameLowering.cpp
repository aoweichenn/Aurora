#include "Aurora/Target/X86/X86FrameLowering.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"

namespace aurora {

void X86FrameLowering::emitPrologue(MachineFunction& /*mf*/, MachineBasicBlock& /*entry*/) const {
}

void X86FrameLowering::emitEpilogue(MachineFunction& /*mf*/, MachineBasicBlock& /*ret*/) const {
}

int X86FrameLowering::getFrameIndexReference(const MachineFunction& /*mf*/, int frameIdx, unsigned& outReg) const {
    outReg = X86RegisterInfo::RBP;
    return -(frameIdx + 1) * 8;
}

bool X86FrameLowering::hasFP(const MachineFunction& /*mf*/) const {
    return true;
}

} // namespace aurora
