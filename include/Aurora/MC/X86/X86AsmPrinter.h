#pragma once

#include <ostream>
#include "Aurora/MC/AsmPrinter.h"

namespace aurora {

class X86RegisterInfo;

class X86AsmPrinter : public AsmPrinter {
public:
    X86AsmPrinter(MCStreamer& streamer, const X86RegisterInfo& /*regInfo*/);

    void emitInstruction(const MachineInstr& mi) override;

protected:
    void emitFunctionHeader(MachineFunction& mf) override;
    void emitFunctionFooter(MachineFunction& mf) override;

private:
    void printOperand(const MachineOperand& mo, std::ostream& os) const;
    void print8BitOperand(const MachineOperand& mo, std::ostream& os) const;
    void printMemOperand(const MachineOperand& base, const MachineOperand& offset, std::ostream& os) const;
};

} // namespace aurora

