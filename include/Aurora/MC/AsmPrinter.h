#pragma once

namespace aurora {

class Module;
class MachineFunction;
class MachineBasicBlock;
class MachineInstr;
class MachineOperand;
class MCStreamer;

class AsmPrinter {
public:
    explicit AsmPrinter(MCStreamer& streamer);
    virtual ~AsmPrinter() = default;

    virtual void emitFunction(MachineFunction& mf);
    virtual void emitBasicBlock(MachineBasicBlock& mbb);
    virtual void emitInstruction(const MachineInstr& mi) = 0;
    virtual void emitGlobals(Module& mod);

protected:
    [[nodiscard]] MCStreamer& getStreamer() const noexcept { return streamer_; }
    virtual void emitFunctionHeader(MachineFunction& mf);
    virtual void emitFunctionFooter(MachineFunction& mf);

private:
    MCStreamer& streamer_;
};

} // namespace aurora

