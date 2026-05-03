#ifndef AURORA_TARGET_TARGETFRAMELOWERING_H
#define AURORA_TARGET_TARGETFRAMELOWERING_H

namespace aurora {

class MachineFunction;
class MachineBasicBlock;

class TargetFrameLowering {
public:
    virtual ~TargetFrameLowering() = default;

    virtual void emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const = 0;
    virtual void emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const = 0;
    virtual int getFrameIndexReference(const MachineFunction& mf, int frameIdx, unsigned& outReg) const = 0;
    virtual bool hasFP(const MachineFunction& mf) const = 0;
    virtual unsigned getStackAlignment() const = 0;
};

} // namespace aurora

#endif // AURORA_TARGET_TARGETFRAMELOWERING_H
