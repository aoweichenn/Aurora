#pragma once

#include "Aurora/Target/TargetFrameLowering.h"

namespace aurora {

class AArch64FrameLowering : public TargetFrameLowering {
public:
    void emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const override;
    void emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const override;
    int getFrameIndexReference(const MachineFunction& mf, int frameIdx, unsigned& outReg) const override;
    [[nodiscard]] bool hasFP(const MachineFunction& mf) const override;
    [[nodiscard]] unsigned getStackAlignment() const override { return 16; }
};

} // namespace aurora
