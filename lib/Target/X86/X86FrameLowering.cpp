#include "Aurora/Target/X86/X86FrameLowering.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"

namespace aurora {

void X86FrameLowering::emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const {
    // push rbp
    // mov rbp, rsp
    // sub rsp, frameSize
}

void X86FrameLowering::emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const {
    // mov rsp, rbp (or add rsp, frameSize)
    // pop rbp
    // ret
}

int X86FrameLowering::getFrameIndexReference(const MachineFunction& mf, int frameIdx, unsigned& outReg) const {
    // Return offset from either RSP or RBP
    outReg = X86RegisterInfo::RBP;
    return -(frameIdx + 1) * 8; // simplified
}

bool X86FrameLowering::hasFP(const MachineFunction& mf) const {
    return true; // Always use frame pointer for now
}

} // namespace aurora
