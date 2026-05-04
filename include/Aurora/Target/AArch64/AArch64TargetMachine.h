#pragma once

#include <memory>
#include "Aurora/Target/TargetMachine.h"

namespace aurora {

class AArch64RegisterInfo;
class AArch64InstrInfo;
class AArch64TargetLowering;
class AArch64CallingConv;
class AArch64FrameLowering;

class AArch64TargetMachine : public TargetMachine {
public:
    AArch64TargetMachine();
    ~AArch64TargetMachine() override;

    [[nodiscard]] const TargetRegisterInfo&  getRegisterInfo() const override;
    [[nodiscard]] const TargetInstrInfo&     getInstrInfo() const override;
    [[nodiscard]] const TargetLowering&      getLowering() const override;
    [[nodiscard]] const TargetCallingConv&   getCallingConv() const override;
    [[nodiscard]] const TargetFrameLowering& getFrameLowering() const override;

    [[nodiscard]] AsmPrinter* createAsmPrinter(MCStreamer& streamer) const override;
    [[nodiscard]] const DataLayout& getDataLayout() const override;
    [[nodiscard]] const char* getTargetTriple() const override { return "arm64-apple-darwin"; }

private:
    std::unique_ptr<AArch64RegisterInfo> regInfo_;
    std::unique_ptr<AArch64InstrInfo> instrInfo_;
    std::unique_ptr<AArch64TargetLowering> lowering_;
    std::unique_ptr<AArch64CallingConv> callingConv_;
    std::unique_ptr<AArch64FrameLowering> frameLowering_;
    std::unique_ptr<DataLayout> dataLayout_;
};

} // namespace aurora
