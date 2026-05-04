#include "Aurora/MC/AsmPrinter.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Constant.h"

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

    const MachineInstr* mi = mbb.getFirst();
    while (mi) {
        emitInstruction(*mi);
        mi = mi->getNext();
    }
}

void AsmPrinter::emitFunctionHeader(MachineFunction& mf) {
    const auto& fn = mf.getAIRFunction();
    streamer_.emitGlobalSymbol(fn.getName());
    streamer_.emitLabel(fn.getName());
    streamer_.emitRawText("\t.type " + fn.getName() + ", @function");
}

void AsmPrinter::emitFunctionFooter(MachineFunction& mf) {
    const auto& fn = mf.getAIRFunction();
    streamer_.emitRawText("\t.size " + fn.getName() + ", .-" + fn.getName());
}

void AsmPrinter::emitGlobals(Module& mod) {
    bool hasData = false;
    for (auto& gv : mod.getGlobals()) {
        if (!hasData) { streamer_.emitRawText(".data"); hasData = true; }
        streamer_.emitGlobalSymbol(gv->getName());
        streamer_.emitLabel(gv->getName());
        if (auto* init = gv->getInitializer()) {
            if (auto* ci = dynamic_cast<ConstantInt*>(init))
                streamer_.emitRawText("\t.quad " + std::to_string(ci->getSExtValue()));
            else
                streamer_.emitRawText("\t.quad 0");
        } else {
            streamer_.emitRawText("\t.quad 0");
        }
    }
}

} // namespace aurora
