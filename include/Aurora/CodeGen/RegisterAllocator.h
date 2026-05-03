#ifndef AURORA_CODEGEN_REGISTERALLOCATOR_H
#define AURORA_CODEGEN_REGISTERALLOCATOR_H

#include "Aurora/CodeGen/LiveInterval.h"
#include <vector>

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
    unsigned tryAllocateFreeReg(LiveInterval& current);
    unsigned selectRegToSpill(LiveInterval& current);
    void spillAt(LiveInterval& li, unsigned slot);
    void assignPhysReg(LiveInterval& li, unsigned reg);
};

} // namespace aurora

#endif // AURORA_CODEGEN_REGISTERALLOCATOR_H
