#include "Aurora/MC/AArch64/AArch64AsmPrinter.h"
#include "Aurora/MC/MCStreamer.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/AArch64/AArch64InstrInfo.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/Air/Function.h"
#include <sstream>

namespace aurora {

AArch64AsmPrinter::AArch64AsmPrinter(MCStreamer& streamer, const AArch64RegisterInfo& /*regInfo*/)
    : AsmPrinter(streamer) {}

void AArch64AsmPrinter::emitBasicBlock(MachineBasicBlock& mbb) {
    getStreamer().emitLabel("L" + mbb.getName());

    const MachineInstr* mi = mbb.getFirst();
    while (mi) {
        emitInstruction(*mi);
        mi = mi->getNext();
    }
}

void AArch64AsmPrinter::emitInstruction(const MachineInstr& mi) {
    std::ostringstream oss;
    oss << "\t";

    switch (mi.getOpcode()) {
    case AArch64::MOVrr:
        if (mi.getNumOperands() >= 2) {
            oss << "mov\t";
            printOperand(mi.getOperand(1), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::MOVri:
        if (mi.getNumOperands() >= 2) {
            oss << "mov\t";
            printOperand(mi.getOperand(1), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::ADDrr:
        if (mi.getNumOperands() >= 3) {
            oss << "add\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::ADDri:
        if (mi.getNumOperands() >= 3) {
            oss << "add\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::SUBrr:
        if (mi.getNumOperands() >= 3) {
            oss << "sub\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::SUBri:
        if (mi.getNumOperands() >= 3) {
            oss << "sub\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::MULrr:
        if (mi.getNumOperands() >= 3) {
            oss << "mul\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::SDIVrr:
        if (mi.getNumOperands() >= 3) {
            oss << "sdiv\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::ANDrr: case AArch64::ORRrr: case AArch64::EORrr:
    case AArch64::LSLrr: case AArch64::LSRrr: case AArch64::ASRrr:
        if (mi.getNumOperands() >= 3) {
            const char* mnemonic = "and";
            switch (mi.getOpcode()) {
            case AArch64::ORRrr: mnemonic = "orr"; break;
            case AArch64::EORrr: mnemonic = "eor"; break;
            case AArch64::LSLrr: mnemonic = "lsl"; break;
            case AArch64::LSRrr: mnemonic = "lsr"; break;
            case AArch64::ASRrr: mnemonic = "asr"; break;
            default: break;
            }
            oss << mnemonic << "\t";
            printOperand(mi.getOperand(2), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::CMPrr:
        if (mi.getNumOperands() >= 2) {
            oss << "cmp\t";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::CMPri:
        if (mi.getNumOperands() >= 2) {
            oss << "cmp\t";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::CSETEQ: case AArch64::CSETNE: case AArch64::CSETLT:
    case AArch64::CSETLE: case AArch64::CSETGT: case AArch64::CSETGE:
        if (mi.getNumOperands() >= 1) {
            const char* cond = "eq";
            switch (mi.getOpcode()) {
            case AArch64::CSETNE: cond = "ne"; break;
            case AArch64::CSETLT: cond = "lt"; break;
            case AArch64::CSETLE: cond = "le"; break;
            case AArch64::CSETGT: cond = "gt"; break;
            case AArch64::CSETGE: cond = "ge"; break;
            default: break;
            }
            oss << "cset\t";
            printOperand(mi.getOperand(0), oss);
            oss << ", " << cond;
        }
        break;
    case AArch64::B: case AArch64::BEQ: case AArch64::BNE:
    case AArch64::BLT: case AArch64::BLE: case AArch64::BGT: case AArch64::BGE:
        if (mi.getNumOperands() >= 1) {
            const char* mnemonic = "b";
            switch (mi.getOpcode()) {
            case AArch64::BEQ: mnemonic = "b.eq"; break;
            case AArch64::BNE: mnemonic = "b.ne"; break;
            case AArch64::BLT: mnemonic = "b.lt"; break;
            case AArch64::BLE: mnemonic = "b.le"; break;
            case AArch64::BGT: mnemonic = "b.gt"; break;
            case AArch64::BGE: mnemonic = "b.ge"; break;
            default: break;
            }
            oss << mnemonic << "\t";
            printLabelOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::BL:
        if (mi.getNumOperands() >= 1) {
            oss << "bl\t";
            printLabelOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::RET:
        oss << "ret";
        break;
    case AArch64::LDRfi:
        if (mi.getNumOperands() >= 2) {
            oss << "ldr\t";
            printOperand(mi.getOperand(1), oss);
            oss << ", ";
            printOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::STRfi:
        if (mi.getNumOperands() >= 2) {
            oss << "str\t";
            printOperand(mi.getOperand(0), oss);
            oss << ", ";
            printOperand(mi.getOperand(1), oss);
        }
        break;
    case AArch64::STPpre:
        oss << "stp\tx29, x30, [sp, #-16]!";
        break;
    case AArch64::LDPpost:
        oss << "ldp\tx29, x30, [sp], #16";
        break;
    case AArch64::MOVsp:
        oss << "mov\tx29, sp";
        break;
    case AArch64::SUBSPi:
        if (mi.getNumOperands() >= 1) {
            oss << "sub\tsp, sp, ";
            printOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::ADDSPi:
        if (mi.getNumOperands() >= 1) {
            oss << "add\tsp, sp, ";
            printOperand(mi.getOperand(0), oss);
        }
        break;
    case AArch64::BRK:
        oss << "brk\t#0";
        break;
    default:
        oss << "// unknown opcode " << mi.getOpcode();
        break;
    }

    getStreamer().emitRawText(oss.str());
}

void AArch64AsmPrinter::emitFunctionHeader(MachineFunction& mf) {
    const auto& fn = mf.getAIRFunction();
    getStreamer().emitRawText(".p2align 2");
    getStreamer().emitRawText(".globl " + symbolName(fn.getName()));
    getStreamer().emitLabel(symbolName(fn.getName()));
}

void AArch64AsmPrinter::emitFunctionFooter(MachineFunction& /*mf*/) {}

void AArch64AsmPrinter::printOperand(const MachineOperand& mo, std::ostream& os) const {
    switch (mo.getKind()) {
    case MachineOperandKind::MO_Register:
        os << AArch64RegisterInfo::getReg(mo.getReg()).name;
        break;
    case MachineOperandKind::MO_VirtualReg:
        os << "//vreg" << mo.getVirtualReg();
        break;
    case MachineOperandKind::MO_Immediate:
        os << "#" << mo.getImm();
        break;
    case MachineOperandKind::MO_MBB:
        os << "L" << mo.getMBB()->getName();
        break;
    case MachineOperandKind::MO_FrameIndex:
        os << "[x29, #-" << ((mo.getFrameIndex() + 1) * 8) << "]";
        break;
    case MachineOperandKind::MO_GlobalSym:
        os << symbolName(mo.getGlobalSym() ? mo.getGlobalSym() : "");
        break;
    default:
        os << "?";
        break;
    }
}

void AArch64AsmPrinter::printLabelOperand(const MachineOperand& mo, std::ostream& os) const {
    if (mo.getKind() == MachineOperandKind::MO_MBB) {
        os << "L" << mo.getMBB()->getName();
        return;
    }
    printOperand(mo, os);
}

std::string AArch64AsmPrinter::symbolName(const std::string& name) const {
    return "_" + name;
}

} // namespace aurora
