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
        // Find entry block and emit prologue (insert in reverse order before first instruction)
        if (mbb->predecessors().empty()) {
            MachineInstr* first = mbb->getFirst();

            // sub $N, rsp — closest to body
            int totalStack = 0;
            for (auto& so : mf.getStackObjects()) totalStack += (int)so.size;
            if (totalStack > 0) {
                int pushedSize = 8 * 6; // rbp + 5 callee-saved
                totalStack = ((totalStack + pushedSize + 15) & ~15) - pushedSize;
                if (totalStack > 0) {
                    auto* subRSP = new MachineInstr(X86::SUB64ri32);
                    subRSP->addOperand(MachineOperand::createImm(totalStack));
                    subRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
                    if (first) mbb->insertBefore(first, subRSP); else mbb->pushBack(subRSP);
                }
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

            // push callee-saved regs in reverse order (last push = RBX, first push = R15)
            for (unsigned csr : {X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13, X86RegisterInfo::R14, X86RegisterInfo::R15}) {
                auto* pushMI = new MachineInstr(X86::PUSH64r);
                pushMI->addOperand(MachineOperand::createReg(csr));
                first = mbb->getFirst();
                if (first) mbb->insertBefore(first, pushMI); else mbb->pushBack(pushMI);
            }
        }

        // Find return blocks and emit epilogue
        MachineInstr* mi = mbb->getLast();
        while (mi) {
            if (mi->isReturn() || mi->getOpcode() == X86::RETQ) {
                // add $N, rsp (restore stack)
                int totalStack = 0;
                for (auto& so : mf.getStackObjects()) totalStack += (int)so.size;
                if (totalStack > 0) {
                    int pushedSize = 8 * 6; // approximate: rbp + 5 callee-saved
                    totalStack = ((totalStack + pushedSize + 15) & ~15) - pushedSize;
                    if (totalStack > 0) {
                        auto* addRSP = new MachineInstr(X86::ADD64ri32);
                        addRSP->addOperand(MachineOperand::createImm(totalStack));
                        addRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
                        mbb->insertBefore(mi, addRSP);
                    }
                }

                // pop rbp
                auto* popRBP = new MachineInstr(X86::POP64r);
                popRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
                mbb->insertBefore(mi, popRBP);

                // pop callee-saved regs in reverse order
                static const unsigned popOrder[] = {X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13, X86RegisterInfo::R14, X86RegisterInfo::R15};
                for (unsigned csr : popOrder) {
                    auto* popMI = new MachineInstr(X86::POP64r);
                    popMI->addOperand(MachineOperand::createReg(csr));
                    mbb->insertBefore(mi, popMI);
                }
                break;
            }
            mi = mi->getPrev();
        }
    }
}

} // namespace aurora
