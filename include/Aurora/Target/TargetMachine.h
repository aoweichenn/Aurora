#pragma once

#include <memory>

namespace aurora {

class TargetRegisterInfo;
class TargetInstrInfo;
class TargetLowering;
class TargetCallingConv;
class TargetFrameLowering;
class AsmPrinter;
class MCStreamer;
class Module;
class DataLayout;

class TargetMachine {
public:
    virtual ~TargetMachine() = default;

    [[nodiscard]] virtual const TargetRegisterInfo&  getRegisterInfo() const = 0;
    [[nodiscard]] virtual const TargetInstrInfo&     getInstrInfo() const = 0;
    [[nodiscard]] virtual const TargetLowering&      getLowering() const = 0;
    [[nodiscard]] virtual const TargetCallingConv&   getCallingConv() const = 0;
    [[nodiscard]] virtual const TargetFrameLowering& getFrameLowering() const = 0;

    [[nodiscard]] virtual AsmPrinter* createAsmPrinter(MCStreamer& streamer) const = 0;
    [[nodiscard]] virtual const DataLayout& getDataLayout() const = 0;

    [[nodiscard]] virtual const char* getTargetTriple() const = 0;

    [[nodiscard]] static std::unique_ptr<TargetMachine> createX86_64();
};

} // namespace aurora

