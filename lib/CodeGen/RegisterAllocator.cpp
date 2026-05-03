#include "Aurora/CodeGen/RegisterAllocator.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/TargetRegisterInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include <algorithm>
#include <map>

namespace aurora {

LinearScanRegAlloc::LinearScanRegAlloc(MachineFunction& mf)
    : mf_(mf),
      regInfo_(mf.getTarget().getRegisterInfo()),
      instrInfo_(mf.getTarget().getInstrInfo()) {
}

void LinearScanRegAlloc::allocateRegisters() {
    computeLiveIntervals();
    linearScan();
}

void LinearScanRegAlloc::computeLiveIntervals() {
    intervals_.clear();
    std::map<unsigned, size_t> vregToIdx;

    for (unsigned vreg = 0; vreg < mf_.getNumVRegs(); ++vreg) {
        Type* ty = mf_.getVRegType(vreg);
        if (ty) {
            vregToIdx[vreg] = intervals_.size();
            intervals_.emplace_back(vreg, ty);
        }
    }

    // Dataflow analysis to compute live ranges
    // For each MachineBasicBlock, compute def/use positions
    unsigned slot = 0;
    std::map<unsigned, unsigned> lastDef;
    std::map<unsigned, unsigned> firstUse;

    for (const auto& mbb : mf_.getBlocks()) {
        MachineInstr* mi = mbb->getFirst();
        while (mi) {
            for (unsigned i = 0; i < mi->getNumOperands(); ++i) {
                MachineOperand& mo = mi->getOperand(i);
                if (mo.isVReg()) {
                    unsigned vreg = mo.getVirtualReg();
                    firstUse[vreg] = std::min(firstUse.count(vreg) ? firstUse[vreg] : slot, slot);
                }
            }
            // For defs, check if it's a destination
            if (mi->getNumOperands() > 0 && mi->getOperand(0).isVReg()) {
                unsigned vreg = mi->getOperand(0).getVirtualReg();
                lastDef[vreg] = slot;
            }
            mi = mi->getNext();
            ++slot;
        }
    }

    // Build intervals from def/use pairs
    for (auto& [vreg, def] : lastDef) {
        auto it = vregToIdx.find(vreg);
        if (it != vregToIdx.end() && firstUse.count(vreg)) {
            intervals_[it->second].addRange(def, firstUse[vreg]);
        }
    }
}

void LinearScanRegAlloc::linearScan() {
    // Sort intervals by start position
    std::sort(intervals_.begin(), intervals_.end(),
              [](const LiveInterval& a, const LiveInterval& b) {
                  return a.start() < b.start();
              });

    for (auto& li : intervals_) {
        expireOldIntervals(li, li.start());

        const unsigned reg = tryAllocateFreeReg(li);
        if (reg == ~0U) {
            // Need to spill
            const unsigned spillReg = selectRegToSpill(li);
            if (spillReg != ~0U) {
                // Spill the existing interval
                for (auto& other : intervals_) {
                    if (other.hasAssignment() && other.getAssignedReg() == spillReg &&
                        other.overlaps(li)) {
                        spillAt(other, li.start());
                    }
                }
                assignPhysReg(li, spillReg);
            }
        } else {
            assignPhysReg(li, reg);
        }
    }
}

void LinearScanRegAlloc::expireOldIntervals(LiveInterval& /*current*/, unsigned /*currentStart*/) {
    // Remove intervals that end before currentStart from active set
    // (handled via sorted processing)
}

unsigned LinearScanRegAlloc::tryAllocateFreeReg(LiveInterval& current) {
    const auto& order = regInfo_.getAllocOrder(RegClass::GPR64);
    for (const unsigned reg : order) {
        if (reg == X86RegisterInfo::RSP || reg == X86RegisterInfo::RBP)
            continue; // Skip reserved registers
        bool conflict = false;
        for (auto& li : intervals_) {
            if (li.hasAssignment() && li.getAssignedReg() == reg && li.overlaps(current)) {
                conflict = true;
                break;
            }
        }
        if (!conflict) return reg;
    }
    return ~0U;
}

unsigned LinearScanRegAlloc::selectRegToSpill(LiveInterval& /*current*/) {
    // Select interval with furthest next use position to spill
    // Simplified: return first callee-saved register
    return X86RegisterInfo::RBX;
}

void LinearScanRegAlloc::spillAt(LiveInterval& li, unsigned /*slot*/) {
    const int spillSlot = mf_.createStackSlot(8, 8);
    li.setSpillSlot(spillSlot);
}

void LinearScanRegAlloc::assignPhysReg(LiveInterval& li, const unsigned reg) {
    li.setAssignedReg(reg);
}

} // namespace aurora
