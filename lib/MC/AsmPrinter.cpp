#include "Aurora/MC/AsmPrinter.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Constant.h"

namespace aurora {

namespace {

void emitIntegerData(MCStreamer& streamer, int64_t value) {
    streamer.emitRawText("\t.quad " + std::to_string(value));
}

void emitZeroDataForType(MCStreamer& streamer, Type* type) {
    if (type && type->isArray()) {
        for (unsigned index = 0; index < type->getNumElements(); ++index)
            emitZeroDataForType(streamer, type->getElementType());
        return;
    }
    emitIntegerData(streamer, 0);
}

void emitConstantData(MCStreamer& streamer, Constant* init, Type* type) {
    if (auto* array = dynamic_cast<ConstantArray*>(init)) {
        Type* elementType = type && type->isArray() ? type->getElementType() : nullptr;
        const size_t count = type && type->isArray() ? type->getNumElements() : array->getNumElements();
        for (size_t index = 0; index < count; ++index) {
            if (auto* element = array->getElement(index))
                emitConstantData(streamer, element, elementType);
            else
                emitZeroDataForType(streamer, elementType);
        }
        return;
    }
    if (auto* ci = dynamic_cast<ConstantInt*>(init)) {
        emitIntegerData(streamer, ci->getSExtValue());
        return;
    }
    emitZeroDataForType(streamer, type);
}

} // namespace

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
        emitConstantData(streamer_, gv->getInitializer(), gv->getType());
    }
}

} // namespace aurora
