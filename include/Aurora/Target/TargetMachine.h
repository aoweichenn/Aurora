#ifndef AURORA_TARGET_TARGETMACHINE_H
#define AURORA_TARGET_TARGETMACHINE_H

#include <memory>
#include <string>

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

    virtual const TargetRegisterInfo&  getRegisterInfo() const = 0;
    virtual const TargetInstrInfo&     getInstrInfo() const = 0;
    virtual const TargetLowering&      getLowering() const = 0;
    virtual const TargetCallingConv&   getCallingConv() const = 0;
    virtual const TargetFrameLowering& getFrameLowering() const = 0;

    virtual AsmPrinter* createAsmPrinter(MCStreamer& streamer) const = 0;
    virtual const DataLayout& getDataLayout() const = 0;

    virtual const char* getTargetTriple() const = 0;

    static std::unique_ptr<TargetMachine> createX86_64();
};

} // namespace aurora

#endif // AURORA_TARGET_TARGETMACHINE_H
