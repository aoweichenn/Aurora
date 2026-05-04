#include "Aurora/Target/X86/X86FrameLowering.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

void X86FrameLowering::emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const {
    MachineInstr* first = entry.getFirst();

    // Calculate stack size (16-byte aligned)
    int totalStack = 0;
    for (auto& so : mf.getStackObjects()) totalStack += (int)so.size;
    int pushedSize = 8 * 6; // rbp + 5 callee-saved (RBX, R12-R15)
    int adjStack = 0;
    if (totalStack > 0) {
        adjStack = ((totalStack + pushedSize + 15) & ~15) - pushedSize;
    }

    if (adjStack > 0) {
        auto* subRSP = new MachineInstr(X86::SUB64ri32);
        subRSP->addOperand(MachineOperand::createImm(adjStack));
        subRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
        if (first) entry.insertBefore(first, subRSP); else entry.pushBack(subRSP);
    }

    // mov rsp, rbp
    auto* movRB = new MachineInstr(X86::MOV64rr);
    movRB->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
    movRB->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
    first = entry.getFirst();
    if (first) entry.insertBefore(first, movRB); else entry.pushBack(movRB);

    // push rbp
    auto* pushRBP = new MachineInstr(X86::PUSH64r);
    pushRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
    first = entry.getFirst();
    if (first) entry.insertBefore(first, pushRBP); else entry.pushBack(pushRBP);

    // push callee-saved regs (RBX, R12-R15)
    for (unsigned csr : {X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13, X86RegisterInfo::R14, X86RegisterInfo::R15}) {
        auto* pushMI = new MachineInstr(X86::PUSH64r);
        pushMI->addOperand(MachineOperand::createReg(csr));
        first = entry.getFirst();
        if (first) entry.insertBefore(first, pushMI); else entry.pushBack(pushMI);
    }
}

void X86FrameLowering::emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const {
    MachineInstr* term = ret.getLast();
    while (term && term->getOpcode() != X86::RETQ)
        term = term->getPrev();
    if (!term) return;

    // Calculate stack size
    int totalStack = 0;
    for (auto& so : mf.getStackObjects()) totalStack += (int)so.size;
    int pushedSize = 8 * 6;
    int adjStack = 0;
    if (totalStack > 0) {
        adjStack = ((totalStack + pushedSize + 15) & ~15) - pushedSize;
    }

    if (adjStack > 0) {
        auto* addRSP = new MachineInstr(X86::ADD64ri32);
        addRSP->addOperand(MachineOperand::createImm(adjStack));
        addRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
        ret.insertBefore(term, addRSP);
    }

    // pop rbp
    auto* popRBP = new MachineInstr(X86::POP64r);
    popRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
    ret.insertBefore(term, popRBP);

    // pop callee-saved regs in reverse order
    for (unsigned csr : {X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13, X86RegisterInfo::R14, X86RegisterInfo::R15}) {
        auto* popMI = new MachineInstr(X86::POP64r);
        popMI->addOperand(MachineOperand::createReg(csr));
        ret.insertBefore(term, popMI);
    }
}

int X86FrameLowering::getFrameIndexReference(const MachineFunction& /*mf*/, const int frameIdx, unsigned& outReg) const {
    outReg = X86RegisterInfo::RBP;
    return -(frameIdx + 1) * 8;
}

bool X86FrameLowering::hasFP(const MachineFunction& /*mf*/) const {
    return true;
}

} // namespace aurora
