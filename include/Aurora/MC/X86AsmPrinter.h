#ifndef AURORA_MC_X86ASMPRINTER_H
#define AURORA_MC_X86ASMPRINTER_H

#include "Aurora/MC/AsmPrinter.h"
#include <ostream>

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
    void printOperand(const MachineOperand& mo, std::ostream& os);
    void printMemOperand(const MachineOperand& base, const MachineOperand& offset, std::ostream& os);
};

} // namespace aurora

#endif // AURORA_MC_X86ASMPRINTER_H
