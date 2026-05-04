#include "Aurora/MC/X86AsmPrinter.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include <sstream>

namespace aurora {

X86AsmPrinter::X86AsmPrinter(MCStreamer& streamer, const X86RegisterInfo& /*regInfo*/)
    : AsmPrinter(streamer) {}

void X86AsmPrinter::emitInstruction(const MachineInstr& mi) {
    std::ostringstream oss;

    // Use instruction info to get asm string template
    // For now, do basic formatting
    oss << "\t";

    // Get opcode-level info
    const unsigned opcode = mi.getOpcode();

    // Simple mapping to AT&T syntax for common instructions
    // In a full implementation, this would use the InstrInfo tables
    switch (opcode) {
        case X86::MOV64rm:
            if (mi.getNumOperands() >= 2) {
                oss << "movq\t";
                printOperand(mi.getOperand(1), oss); // src (memory)
                oss << ", ";
                printOperand(mi.getOperand(0), oss); // dst (register)
            }
            break;
        case X86::MOV64mr:
            if (mi.getNumOperands() >= 2) {
                oss << "movq\t";
                printOperand(mi.getOperand(1), oss); // src (register)
                oss << ", ";
                printOperand(mi.getOperand(0), oss); // dst (memory)
            }
            break;
        case X86::MOV32rr:
        case X86::MOV64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "movq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::MOV64ri32:
            if (mi.getNumOperands() >= 2) {
                oss << "movq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::ADD64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "addq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::ADD64ri32:
            if (mi.getNumOperands() >= 2) {
                oss << "addq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::SUB64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "subq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::SUB64ri32:
            if (mi.getNumOperands() >= 2) {
                oss << "subq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::IMUL64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "imulq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::AND64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "andq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::OR64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "orq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::XOR64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "xorq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::SHL64rCL:
            if (mi.getNumOperands() >= 1) {
                oss << "shlq\t%cl, ";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::SHL64ri:
            if (mi.getNumOperands() >= 2) {
                oss << "shlq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::CMP64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "cmpq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::TEST64rr:
            if (mi.getNumOperands() >= 2) {
                oss << "testq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::MOVSX64rr32:
            if (mi.getNumOperands() >= 2) {
                oss << "movslq\t";
                printOperand(mi.getOperand(0), oss);
                oss << ", ";
                printOperand(mi.getOperand(1), oss);
            }
            break;
        case X86::IDIV64r:
            if (mi.getNumOperands() >= 1) {
                oss << "idivq\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::AND64ri32:
            if (mi.getNumOperands() >= 2) { oss << "andq\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::OR64ri32:
            if (mi.getNumOperands() >= 2) { oss << "orq\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::XOR64ri32:
            if (mi.getNumOperands() >= 2) { oss << "xorq\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::CMP64ri32:
            if (mi.getNumOperands() >= 2) { oss << "cmpq\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::SAR64rCL:
            if (mi.getNumOperands() >= 1) { oss << "sarq\t%cl, "; printOperand(mi.getOperand(0), oss); }
            break;
        case X86::SHR64rCL:
            if (mi.getNumOperands() >= 1) { oss << "shrq\t%cl, "; printOperand(mi.getOperand(0), oss); }
            break;
        case X86::CQO:
            oss << "cqo";
            break;
        case X86::SETEr:
            if (mi.getNumOperands() >= 1) { oss << "sete\t"; printOperand(mi.getOperand(0), oss); }
            break;
        case X86::CALL64pcrel32:
            if (mi.getNumOperands() >= 1) {
                oss << "call\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::RETQ:
            oss << "ret";
            break;
        case X86::PUSH64r:
            if (mi.getNumOperands() >= 1) {
                oss << "pushq\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::POP64r:
            if (mi.getNumOperands() >= 1) {
                oss << "popq\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JE_1:
        case X86::JE_4:
            if (mi.getNumOperands() >= 1) {
                oss << "je\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JNE_1:
        case X86::JNE_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jne\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JL_1:
        case X86::JL_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jl\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JG_1:
        case X86::JG_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jg\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JLE_1:
        case X86::JLE_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jle\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JGE_1:
        case X86::JGE_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jge\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JA_1:
            if (mi.getNumOperands() >= 1) {
                oss << "ja\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JB_1:
            if (mi.getNumOperands() >= 1) {
                oss << "jb\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JAE_1:
        case X86::JAE_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jae\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JBE_1:
        case X86::JBE_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jbe\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::JMP_1:
        case X86::JMP_4:
            if (mi.getNumOperands() >= 1) {
                oss << "jmp\t";
                printOperand(mi.getOperand(0), oss);
            }
            break;
        case X86::ADDSDrr:
            if (mi.getNumOperands() >= 2) { oss << "addsd\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::SUBSDrr:
            if (mi.getNumOperands() >= 2) { oss << "subsd\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::MULSDrr:
            if (mi.getNumOperands() >= 2) { oss << "mulsd\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::DIVSDrr:
            if (mi.getNumOperands() >= 2) { oss << "divsd\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::UCOMISDrr:
            if (mi.getNumOperands() >= 2) { oss << "ucomisd\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::CVTSI2SDrr:
            if (mi.getNumOperands() >= 2) { oss << "cvtsi2sd\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        case X86::CVTTSD2SIrr:
            if (mi.getNumOperands() >= 2) { oss << "cvttsd2si\t"; printOperand(mi.getOperand(0), oss); oss << ", "; printOperand(mi.getOperand(1), oss); }
            break;
        default:
            oss << "# unknown opcode " << opcode << " (0x" << std::hex << opcode << std::dec << ")";
            break;
    }

    getStreamer().emitRawText(oss.str());
}

void X86AsmPrinter::emitFunctionHeader(MachineFunction& mf) {
    AsmPrinter::emitFunctionHeader(mf);
    // Emit CFI directives for debugging
    getStreamer().emitRawText("\t.cfi_startproc");
}

void X86AsmPrinter::emitFunctionFooter(MachineFunction& mf) {
    getStreamer().emitRawText("\t.cfi_endproc");
    AsmPrinter::emitFunctionFooter(mf);
}

void X86AsmPrinter::printOperand(const MachineOperand& mo, std::ostream& os) const
{
    switch (mo.getKind()) {
        case MachineOperandKind::MO_Register: {
            const auto reg = X86RegisterInfo::getReg(mo.getReg());
            os << "%" << reg.name;
            break;
        }
        case MachineOperandKind::MO_VirtualReg:
            os << "#vreg" << mo.getVirtualReg();
            break;
        case MachineOperandKind::MO_Immediate:
            os << "$" << mo.getImm();
            break;
        case MachineOperandKind::MO_MBB:
            os << "." << mo.getMBB()->getName();
            break;
        case MachineOperandKind::MO_FrameIndex:
            os << "-" << ((mo.getFrameIndex() + 1) * 8) << "(%rbp)";
            break;
        case MachineOperandKind::MO_GlobalSym:
            os << mo.getGlobalSym();
            break;
        default:
            os << "?";
            break;
    }
}

void X86AsmPrinter::printMemOperand(const MachineOperand& base, const MachineOperand& offset, std::ostream& os) const
{
    os << offset.getImm() << "(%";
    const auto reg = X86RegisterInfo::getReg(base.getReg());
    os << reg.name << ")";
}

} // namespace aurora
