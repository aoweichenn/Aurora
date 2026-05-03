#include "Aurora/Target/X86/X86TargetMachine.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86TargetLowering.h"
#include "Aurora/Target/X86/X86CallingConv.h"
#include "Aurora/Target/X86/X86FrameLowering.h"
#include "Aurora/Air/Module.h"

namespace aurora {

X86TargetMachine::X86TargetMachine()
    : regInfo_(std::make_unique<X86RegisterInfo>())
    , instrInfo_(std::make_unique<X86InstrInfo>(*regInfo_))
    , lowering_(std::make_unique<X86TargetLowering>())
    , callingConv_(std::make_unique<X86CallingConv>())
    , frameLowering_(std::make_unique<X86FrameLowering>())
    , dataLayout_(std::make_unique<DataLayout>()) {
    dataLayout_->setLittleEndian(true);
    dataLayout_->setPointerSize(64);
}

X86TargetMachine::~X86TargetMachine() = default;

const TargetRegisterInfo&  X86TargetMachine::getRegisterInfo() const  { return *regInfo_; }
const TargetInstrInfo&     X86TargetMachine::getInstrInfo() const     { return *instrInfo_; }
const TargetLowering&      X86TargetMachine::getLowering() const      { return *lowering_; }
const TargetCallingConv&   X86TargetMachine::getCallingConv() const   { return *callingConv_; }
const TargetFrameLowering& X86TargetMachine::getFrameLowering() const { return *frameLowering_; }

const DataLayout& X86TargetMachine::getDataLayout() const { return *dataLayout_; }

AsmPrinter* X86TargetMachine::createAsmPrinter(MCStreamer& streamer) const {
    // Created externally by MC layer
    return nullptr;
}

std::unique_ptr<TargetMachine> TargetMachine::createX86_64() {
    return std::make_unique<X86TargetMachine>();
}

} // namespace aurora
