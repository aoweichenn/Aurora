#include "Aurora/MC/MCStreamer.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include <iostream>

namespace aurora {

AsmTextStreamer::AsmTextStreamer(std::ostream& os) : os_(os) {}
AsmTextStreamer::~AsmTextStreamer() = default;

void AsmTextStreamer::emitInstruction(const MachineInstr& mi) {
    os_ << "\t" << ".byte 0x" << std::hex << mi.getOpcode() << std::dec
        << " # instruction placeholder\n";
}

void AsmTextStreamer::emitLabel(const std::string& name) {
    os_ << name << ":\n";
}

void AsmTextStreamer::emitComment(const std::string& text) {
    os_ << "# " << text << "\n";
}

void AsmTextStreamer::emitGlobalSymbol(const std::string& name) {
    os_ << ".globl " << name << "\n";
}

void AsmTextStreamer::emitAlignment(const unsigned bytes) {
    os_ << ".align " << bytes << "\n";
}

void AsmTextStreamer::emitBytes(const std::vector<uint8_t>& data) {
    os_ << "\t.byte ";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i) os_ << ",";
        os_ << "0x" << std::hex << static_cast<int>(data[i]) << std::dec;
    }
    os_ << "\n";
}

void AsmTextStreamer::emitString(const std::string& str) {
    os_ << "\t.asciz \"" << str << "\"\n";
}

void AsmTextStreamer::emitRawText(const std::string& text) {
    os_ << text << "\n";
}

} // namespace aurora
