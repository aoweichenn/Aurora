#pragma once

#include <ostream>
#include <string>
#include "Aurora/MC/AsmPrinter.h"

namespace aurora {

class X86RegisterInfo;

class X86AsmPrinter : public AsmPrinter {
public:
    X86AsmPrinter(MCStreamer& streamer, const X86RegisterInfo& /*regInfo*/);

    void emitInstruction(const MachineInstr& mi) override;
    void emitBasicBlock(MachineBasicBlock& mbb) override;

protected:
    void emitFunctionHeader(MachineFunction& mf) override;
    void emitFunctionFooter(MachineFunction& mf) override;

private:
    std::string currentFunctionName_;

    void printOperand(const MachineOperand& mo, std::ostream& os) const;
    void print8BitOperand(const MachineOperand& mo, std::ostream& os) const;
    void printMemOperand(const MachineOperand& base, const MachineOperand& offset, std::ostream& os) const;
    [[nodiscard]] std::string labelName(const std::string& blockName) const;
};

} // namespace aurora
