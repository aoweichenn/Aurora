#ifndef AURORA_X86_X86TARGETMACHINE_H
#define AURORA_X86_X86TARGETMACHINE_H

#include "Aurora/Target/TargetMachine.h"
#include <memory>

namespace aurora {

class X86RegisterInfo;
class X86InstrInfo;
class X86TargetLowering;
class X86CallingConv;
class X86FrameLowering;

class X86TargetMachine : public TargetMachine {
public:
    X86TargetMachine();
    ~X86TargetMachine() override;

    [[nodiscard]] const TargetRegisterInfo&  getRegisterInfo() const override;
    [[nodiscard]] const TargetInstrInfo&     getInstrInfo() const override;
    [[nodiscard]] const TargetLowering&      getLowering() const override;
    [[nodiscard]] const TargetCallingConv&   getCallingConv() const override;
    [[nodiscard]] const TargetFrameLowering& getFrameLowering() const override;

    AsmPrinter* createAsmPrinter(MCStreamer& streamer) const override;
    [[nodiscard]] const DataLayout& getDataLayout() const override;
    [[nodiscard]] const char* getTargetTriple() const override { return "x86_64-unknown-linux-gnu"; }

private:
    std::unique_ptr<X86RegisterInfo> regInfo_;
    std::unique_ptr<X86InstrInfo> instrInfo_;
    std::unique_ptr<X86TargetLowering> lowering_;
    std::unique_ptr<X86CallingConv> callingConv_;
    std::unique_ptr<X86FrameLowering> frameLowering_;
    std::unique_ptr<DataLayout> dataLayout_;
};

} // namespace aurora

#endif // AURORA_X86_X86TARGETMACHINE_H
