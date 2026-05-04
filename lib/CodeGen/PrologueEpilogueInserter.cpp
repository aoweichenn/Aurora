#include "Aurora/CodeGen/PrologueEpilogueInserter.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/TargetFrameLowering.h"

namespace aurora {

void PrologueEpilogueInserter::run(MachineFunction& mf) const {
    const auto& frameLowering = mf.getTarget().getFrameLowering();
    bool isEntryBlock = true;

    for (auto& mbb : mf.getBlocks()) {
        // Emit prologue only for the first (entry) basic block
        if (isEntryBlock) {
            frameLowering.emitPrologue(mf, *mbb);
            isEntryBlock = false;
        }

        // Find return blocks and emit epilogue
        MachineInstr* mi = mbb->getLast();
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
