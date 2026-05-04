#pragma once

#include <ostream>
#include <string>
#include "Aurora/MC/AsmPrinter.h"

namespace aurora {

class AArch64RegisterInfo;

class AArch64AsmPrinter : public AsmPrinter {
public:
    AArch64AsmPrinter(MCStreamer& streamer, const AArch64RegisterInfo& regInfo);

    void emitBasicBlock(MachineBasicBlock& mbb) override;
    void emitInstruction(const MachineInstr& mi) override;

protected:
    void emitFunctionHeader(MachineFunction& mf) override;
    void emitFunctionFooter(MachineFunction& mf) override;

private:
    std::string currentFunctionName_;

    void printOperand(const MachineOperand& mo, std::ostream& os) const;
    void printLabelOperand(const MachineOperand& mo, std::ostream& os) const;
    [[nodiscard]] std::string labelName(const std::string& blockName) const;
    [[nodiscard]] std::string symbolName(const std::string& name) const;
};

} // namespace aurora
