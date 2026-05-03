#ifndef AURORA_X86_X86FRAMELOWERING_H
#define AURORA_X86_X86FRAMELOWERING_H

#include "Aurora/Target/TargetFrameLowering.h"

namespace aurora {

class X86FrameLowering : public TargetFrameLowering {
public:
    void emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const override;
    void emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const override;
    int getFrameIndexReference(const MachineFunction& mf, int frameIdx, unsigned& outReg) const override;
    bool hasFP(const MachineFunction& mf) const override;
    unsigned getStackAlignment() const override { return 16; }

    static constexpr unsigned RED_ZONE_SIZE = 128;
};

} // namespace aurora

#endif // AURORA_X86_X86FRAMELOWERING_H
