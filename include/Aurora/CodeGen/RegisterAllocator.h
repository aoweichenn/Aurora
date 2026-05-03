#pragma once

#include <vector>
#include "Aurora/CodeGen/LiveInterval.h"

namespace aurora {

class MachineFunction;
class TargetRegisterInfo;
class TargetInstrInfo;

class LinearScanRegAlloc {
public:
    LinearScanRegAlloc(MachineFunction& mf);

    void allocateRegisters();

private:
    MachineFunction& mf_;
    const TargetRegisterInfo& regInfo_;
    const TargetInstrInfo& instrInfo_;

    std::vector<LiveInterval> intervals_;
    std::vector<unsigned> freeRegs_;

    void computeLiveIntervals();
    void linearScan();
    void expireOldIntervals(LiveInterval& current, unsigned currentStart);
    [[nodiscard]] unsigned tryAllocateFreeReg(LiveInterval& current);
    [[nodiscard]] unsigned selectRegToSpill(LiveInterval& current);
    void spillAt(LiveInterval& li, unsigned slot);
    void assignPhysReg(LiveInterval& li, unsigned reg);
};

} // namespace aurora

