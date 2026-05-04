#include "Aurora/Target/AArch64/AArch64TargetMachine.h"
#include "Aurora/Target/AArch64/AArch64CallingConv.h"
#include "Aurora/Target/AArch64/AArch64FrameLowering.h"
#include "Aurora/Target/AArch64/AArch64InstrInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/Target/AArch64/AArch64TargetLowering.h"
#include "Aurora/Air/Module.h"

namespace aurora {

AArch64TargetMachine::AArch64TargetMachine()
    : regInfo_(std::make_unique<AArch64RegisterInfo>()),
      instrInfo_(std::make_unique<AArch64InstrInfo>(*regInfo_)),
      lowering_(std::make_unique<AArch64TargetLowering>()),
      callingConv_(std::make_unique<AArch64CallingConv>()),
      frameLowering_(std::make_unique<AArch64FrameLowering>()),
      dataLayout_(std::make_unique<DataLayout>()) {
    dataLayout_->setLittleEndian(true);
    dataLayout_->setPointerSize(64);
}

AArch64TargetMachine::~AArch64TargetMachine() = default;

const TargetRegisterInfo&  AArch64TargetMachine::getRegisterInfo() const  { return *regInfo_; }
const TargetInstrInfo&     AArch64TargetMachine::getInstrInfo() const     { return *instrInfo_; }
const TargetLowering&      AArch64TargetMachine::getLowering() const      { return *lowering_; }
const TargetCallingConv&   AArch64TargetMachine::getCallingConv() const   { return *callingConv_; }
const TargetFrameLowering& AArch64TargetMachine::getFrameLowering() const { return *frameLowering_; }

AsmPrinter* AArch64TargetMachine::createAsmPrinter(MCStreamer& /*streamer*/) const {
    return nullptr;
}

const DataLayout& AArch64TargetMachine::getDataLayout() const { return *dataLayout_; }

std::unique_ptr<TargetMachine> TargetMachine::createAArch64_Apple() {
    return std::make_unique<AArch64TargetMachine>();
}

} // namespace aurora
