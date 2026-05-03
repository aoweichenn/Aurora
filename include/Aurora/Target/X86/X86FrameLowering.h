#pragma once

#include "Aurora/Target/TargetFrameLowering.h"

namespace aurora {

class X86FrameLowering : public TargetFrameLowering {
public:
    void emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const override;
    void emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const override;
    int getFrameIndexReference(const MachineFunction& mf, int frameIdx, unsigned& outReg) const override;
    [[nodiscard]] bool hasFP(const MachineFunction& mf) const override;
    [[nodiscard]] unsigned getStackAlignment() const override { return 16; }

    static constexpr unsigned RED_ZONE_SIZE = 128;
};

} // namespace aurora

