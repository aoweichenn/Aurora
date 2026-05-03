#include "Aurora/MC/AsmPrinter.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/Function.h"

namespace aurora {

AsmPrinter::AsmPrinter(MCStreamer& streamer) : streamer_(streamer) {}

void AsmPrinter::emitFunction(MachineFunction& mf) {
    emitFunctionHeader(mf);

    for (auto& mbb : mf.getBlocks()) {
        emitBasicBlock(*mbb);
    }

    emitFunctionFooter(mf);
}

void AsmPrinter::emitBasicBlock(MachineBasicBlock& mbb) {
    streamer_.emitLabel("." + mbb.getName());

    MachineInstr* mi = mbb.getFirst();
    while (mi) {
        emitInstruction(*mi);
        mi = mi->getNext();
    }
}

void AsmPrinter::emitFunctionHeader(MachineFunction& mf) {
    auto& fn = mf.getAIRFunction();
    streamer_.emitGlobalSymbol(fn.getName());
    streamer_.emitLabel(fn.getName());
    streamer_.emitRawText("\t.type " + fn.getName() + ", @function");
}

void AsmPrinter::emitFunctionFooter(MachineFunction& mf) {
    auto& fn = mf.getAIRFunction();
    streamer_.emitRawText("\t.size " + fn.getName() + ", .-" + fn.getName());
}

} // namespace aurora
