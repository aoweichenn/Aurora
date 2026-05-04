#include "Aurora/CodeGen/PrologueEpilogueInserter.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/TargetFrameLowering.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"

namespace aurora {

void PrologueEpilogueInserter::run(MachineFunction& mf) const {
    const auto& frameLowering = mf.getTarget().getFrameLowering();

    for (auto& mbb : mf.getBlocks()) {
        if (mbb->predecessors().empty()) {
            frameLowering.emitPrologue(mf, *mbb);
        }

        // Find return blocks and emit epilogue
        MachineInstr* mi = mbb->getLast();
        while (mi) {
            if (mi->getOpcode() == X86::RETQ) {
                frameLowering.emitEpilogue(mf, *mbb);
                break;
            }
            mi = mi->getPrev();
        }
    }
}

} // namespace aurora
