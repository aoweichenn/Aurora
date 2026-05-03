#include "Aurora/CodeGen/PrologueEpilogueInserter.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/TargetFrameLowering.h"

namespace aurora {

void PrologueEpilogueInserter::run(MachineFunction& mf) {
    const auto& frameLowering = mf.getTarget().getFrameLowering();

    for (auto& mbb : mf.getBlocks()) {
        // Find entry block and emit prologue
        if (mbb->predecessors().empty()) {
            frameLowering.emitPrologue(mf, *mbb);
        }
        // Find return blocks and emit epilogue
        const MachineInstr* mi = mbb->getLast();
        while (mi) {
            if (mi->isReturn()) {
                frameLowering.emitEpilogue(mf, *mbb);
                break;
            }
            mi = mi->getPrev();
        }
    }
}

} // namespace aurora
