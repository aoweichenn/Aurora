#include "Aurora/CodeGen/PrologueEpilogueInserter.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Target/TargetFrameLowering.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"

namespace aurora {

void PrologueEpilogueInserter::run(MachineFunction& mf) const {
    for (auto& mbb : mf.getBlocks()) {
        // Find entry block and emit prologue
        if (mbb->predecessors().empty()) {
            MachineInstr* first = mbb->getFirst();

            // sub $N, rsp (allocate stack with 16-byte alignment) — insert first
            int totalStack = 0;
            for (auto& so : mf.getStackObjects()) totalStack += (int)so.size;
            if (totalStack > 0) {
                totalStack = (totalStack + 15) & ~15;
                auto* subRSP = new MachineInstr(X86::SUB64ri32);
                subRSP->addOperand(MachineOperand::createImm(totalStack));
                subRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
                if (first) mbb->insertBefore(first, subRSP); else mbb->pushBack(subRSP);
            }

            // mov rsp, rbp
            auto* movRB = new MachineInstr(X86::MOV64rr);
            movRB->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
            movRB->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
            first = mbb->getFirst();
            if (first) mbb->insertBefore(first, movRB); else mbb->pushBack(movRB);

            // push rbp
            auto* pushRBP = new MachineInstr(X86::PUSH64r);
            pushRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
            first = mbb->getFirst();
            if (first) mbb->insertBefore(first, pushRBP); else mbb->pushBack(pushRBP);
        }

        // Find return blocks and emit epilogue
        MachineInstr* mi = mbb->getLast();
        while (mi) {
            if (mi->isReturn() || mi->getOpcode() == X86::RETQ) {
                // add $N, rsp (restore stack)
                int totalStack = 0;
                for (auto& so : mf.getStackObjects()) totalStack += (int)so.size;
                if (totalStack > 0) {
                    totalStack = (totalStack + 15) & ~15;
                    auto* addRSP = new MachineInstr(X86::ADD64ri32);
                    addRSP->addOperand(MachineOperand::createImm(totalStack));
                    addRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
                    mbb->insertBefore(mi, addRSP);
                }

                // pop rbp
                auto* popRBP = new MachineInstr(X86::POP64r);
                popRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
                mbb->insertBefore(mi, popRBP);
                break;
            }
            mi = mi->getPrev();
        }
    }
}

} // namespace aurora
