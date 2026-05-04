#include "Aurora/Target/X86/X86FrameLowering.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

void X86FrameLowering::emitPrologue(MachineFunction& mf, MachineBasicBlock& entry) const {
    // Standard x86-64 prologue (inserted in reverse with insertBefore, so each new
    // instruction goes to the top of the block. Final execution order = reverse of code order):
    //   push rbp
    //   mov rsp, rbp
    //   push rbx, r12, r13, r14, r15
    //   sub $N, rsp

    // Calculate stack adjustment (16-byte aligned)
    int totalStack = 0;
    for (auto& so : mf.getStackObjects()) totalStack += static_cast<int>(so.size);
    int adjStack = (totalStack + 15) & ~15;

    MachineInstr* first = entry.getFirst();

    // Insert in REVERSE order (each insertBefore goes before previous → final order reversed)
    // Last in code = sub $N, rsp (executes last)
    if (adjStack > 0) {
        auto* subRSP = new MachineInstr(X86::SUB64ri32);
        subRSP->addOperand(MachineOperand::createImm(adjStack));
        subRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
        if (first) entry.insertBefore(first, subRSP); else entry.pushBack(subRSP);
        first = entry.getFirst();
    }

    // Push callee-saved regs in REVERSE execution order, so they pop correctly
    for (unsigned csr : {X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13, X86RegisterInfo::R14, X86RegisterInfo::R15}) {
        auto* pushMI = new MachineInstr(X86::PUSH64r);
        pushMI->addOperand(MachineOperand::createReg(csr));
        first = entry.getFirst();
        if (first) entry.insertBefore(first, pushMI); else entry.pushBack(pushMI);
    }

    // mov rsp, rbp
    auto* movRB = new MachineInstr(X86::MOV64rr);
    movRB->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
    movRB->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
    first = entry.getFirst();
    if (first) entry.insertBefore(first, movRB); else entry.pushBack(movRB);

    // push rbp (first instruction in prologue)
    auto* pushRBP = new MachineInstr(X86::PUSH64r);
    pushRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
    first = entry.getFirst();
    if (first) entry.insertBefore(first, pushRBP); else entry.pushBack(pushRBP);
}

void X86FrameLowering::emitEpilogue(MachineFunction& mf, MachineBasicBlock& ret) const {
    MachineInstr* term = ret.getLast();
    while (term && term->getOpcode() != X86::RETQ)
        term = term->getPrev();
    if (!term) return;

    // Calculate stack adjustment
    int totalStack = 0;
    for (auto& so : mf.getStackObjects()) totalStack += static_cast<int>(so.size);
    int adjStack = (totalStack + 15) & ~15;

    // Epilogue (before return), in execution order:
    // add $N, rsp
    // pop rbx, r12, r13, r14, r15  (reverse of push order)
    // pop rbp

    if (adjStack > 0) {
        auto* addRSP = new MachineInstr(X86::ADD64ri32);
        addRSP->addOperand(MachineOperand::createImm(adjStack));
        addRSP->addOperand(MachineOperand::createReg(X86RegisterInfo::RSP));
        ret.insertBefore(term, addRSP);
    }

    // Pop callee-saved in push-order (LIFO): RBX, R12, R13, R14, R15
    for (unsigned csr : {X86RegisterInfo::RBX, X86RegisterInfo::R12, X86RegisterInfo::R13, X86RegisterInfo::R14, X86RegisterInfo::R15}) {
        auto* popMI = new MachineInstr(X86::POP64r);
        popMI->addOperand(MachineOperand::createReg(csr));
        ret.insertBefore(term, popMI);
    }

    // Pop RBP
    auto* popRBP = new MachineInstr(X86::POP64r);
    popRBP->addOperand(MachineOperand::createReg(X86RegisterInfo::RBP));
    ret.insertBefore(term, popRBP);
}

int X86FrameLowering::getFrameIndexReference(const MachineFunction& /*mf*/, const int frameIdx, unsigned& outReg) const {
    outReg = X86RegisterInfo::RBP;
    // Frame index offset: RBP-relative with pushed regs accounted for
    // RBP points between saved RBP and pushed regs
    // Pushed regs: RBX, R12-R15 (5 regs) → offset = -(6 + frameIdx) * 8
    return -(6 + frameIdx) * 8;
}

bool X86FrameLowering::hasFP(const MachineFunction& /*mf*/) const {
    return true;
}

} // namespace aurora
