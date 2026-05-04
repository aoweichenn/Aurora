#include "Aurora/MC/X86/X86ObjectEncoder.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include <stdexcept>
#include <string>

namespace aurora {

void X86ObjectEncoder::encode(const MachineInstr& mi,
                              std::vector<uint8_t>& out,
                              uint64_t textBaseOffset,
                              const SymbolResolver& resolveSymbol,
                              const RelocationSink& addRelocation) const {
    const uint16_t opc = mi.getOpcode();
    const unsigned numOps = mi.getNumOperands();

    auto emitModRM = [&](uint8_t mod, uint8_t reg, uint8_t rm) {
        out.push_back(static_cast<uint8_t>((mod << 6) | ((reg & 7) << 3) | (rm & 7)));
    };
    auto emitImm32 = [&](int64_t value) {
        for (int i = 0; i < 4; ++i)
            out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    };
    auto emitImm8 = [&](int64_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    };
    auto getReg = [&](unsigned idx) -> uint8_t {
        if (idx < numOps && mi.getOperand(idx).isReg())
            return static_cast<uint8_t>(mi.getOperand(idx).getReg());
        return 0;
    };
    auto getImm = [&](unsigned idx) -> int64_t {
        if (idx < numOps && mi.getOperand(idx).isImm())
            return mi.getOperand(idx).getImm();
        return 0;
    };
    auto emitREX = [&](unsigned regFieldIdx, unsigned rmFieldIdx) {
        uint8_t rex = 0x48;
        if (regFieldIdx < numOps && mi.getOperand(regFieldIdx).isReg() && mi.getOperand(regFieldIdx).getReg() >= 8)
            rex |= 4;
        if (rmFieldIdx < numOps && mi.getOperand(rmFieldIdx).isReg() && mi.getOperand(rmFieldIdx).getReg() >= 8)
            rex |= 1;
        out.push_back(rex);
    };
    auto emitREXForRM = [&](unsigned rmFieldIdx) {
        uint8_t rex = 0x48;
        if (rmFieldIdx < numOps && mi.getOperand(rmFieldIdx).isReg() && mi.getOperand(rmFieldIdx).getReg() >= 8)
            rex |= 1;
        out.push_back(rex);
    };
    auto emitOptionalREX32 = [&](unsigned regFieldIdx, unsigned rmFieldIdx) {
        uint8_t rex = 0x40;
        bool needed = false;
        if (regFieldIdx < numOps && mi.getOperand(regFieldIdx).isReg() && mi.getOperand(regFieldIdx).getReg() >= 8) {
            rex |= 4;
            needed = true;
        }
        if (rmFieldIdx < numOps && mi.getOperand(rmFieldIdx).isReg()) {
            const unsigned rmReg = mi.getOperand(rmFieldIdx).getReg();
            if (rmReg >= 8) {
                rex |= 1;
                needed = true;
            } else if (rmReg >= X86RegisterInfo::RSP && rmReg <= X86RegisterInfo::RDI) {
                needed = true;
            }
        }
        if (needed) out.push_back(rex);
    };
    auto emitDisp32 = [&](int disp) {
        for (int i = 0; i < 4; ++i)
            out.push_back(static_cast<uint8_t>((disp >> (i * 8)) & 0xFF));
    };

    switch (opc) {
    case X86::MOV64rr:
        emitREX(0, 1); out.push_back(0x89); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::MOV64ri32:
        emitREXForRM(1); out.push_back(0xC7); emitModRM(3, 0, getReg(1));
        if (numOps >= 1 && mi.getOperand(0).getKind() == MachineOperandKind::MO_GlobalSym && resolveSymbol && addRelocation) {
            const size_t symIdx = resolveSymbol(mi.getOperand(0).getGlobalSym());
            emitImm32(0);
            addRelocation(textBaseOffset + out.size() - 4, symIdx, R_X86_64_32S, 0);
        } else {
            emitImm32(getImm(0));
        }
        break;
    case X86::ADD64rr: emitREX(0, 1); out.push_back(0x01); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::ADD64ri32: emitREXForRM(1); out.push_back(0x81); emitModRM(3, 0, getReg(1)); emitImm32(getImm(0)); break;
    case X86::SUB64rr: emitREX(0, 1); out.push_back(0x29); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::SUB64ri32: emitREXForRM(1); out.push_back(0x81); emitModRM(3, 5, getReg(1)); emitImm32(getImm(0)); break;
    case X86::IMUL64rr: emitREX(1, 0); out.push_back(0x0F); out.push_back(0xAF); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::AND64rr: emitREX(0, 1); out.push_back(0x21); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::OR64rr: emitREX(0, 1); out.push_back(0x09); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::XOR64rr: emitREX(0, 1); out.push_back(0x31); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::XOR32rr: out.push_back(0x31); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::AND64ri32: emitREXForRM(1); out.push_back(0x81); emitModRM(3, 4, getReg(1)); emitImm32(getImm(0)); break;
    case X86::OR64ri32: emitREXForRM(1); out.push_back(0x81); emitModRM(3, 1, getReg(1)); emitImm32(getImm(0)); break;
    case X86::XOR64ri32: emitREXForRM(1); out.push_back(0x81); emitModRM(3, 6, getReg(1)); emitImm32(getImm(0)); break;
    case X86::CMP64rr: emitREX(0, 1); out.push_back(0x39); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::CMP64ri32: emitREXForRM(1); out.push_back(0x81); emitModRM(3, 7, getReg(1)); emitImm32(getImm(0)); break;
    case X86::TEST64rr: emitREX(0, 1); out.push_back(0x85); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::RETQ: out.push_back(0xC3); break;
    case X86::CALL64pcrel32:
        if (numOps >= 1 && mi.getOperand(0).getKind() == MachineOperandKind::MO_GlobalSym && resolveSymbol && addRelocation) {
            const size_t symIdx = resolveSymbol(mi.getOperand(0).getGlobalSym());
            out.push_back(0xE8);
            emitImm32(0);
            addRelocation(textBaseOffset + out.size() - 4, symIdx, R_X86_64_PLT32, -4);
        } else if (numOps >= 1 && mi.getOperand(0).isReg()) {
            emitREXForRM(0); out.push_back(0xFF); emitModRM(3, 2, getReg(0));
        } else {
            throw std::runtime_error("unsupported CALL64pcrel32 operand");
        }
        break;
    case X86::PUSH64r: {
        const uint8_t reg = getReg(0);
        if (reg >= 8) out.push_back(0x41);
        out.push_back(static_cast<uint8_t>(0x50 + (reg & 7)));
        break;
    }
    case X86::POP64r: {
        const uint8_t reg = getReg(0);
        if (reg >= 8) out.push_back(0x41);
        out.push_back(static_cast<uint8_t>(0x58 + (reg & 7)));
        break;
    }
    case X86::MOV64rm: {
        emitREX(0, 1); out.push_back(0x8B);
        const uint8_t dst = getReg(0);
        if (numOps >= 2 && mi.getOperand(1).getKind() == MachineOperandKind::MO_FrameIndex) {
            const int disp = -(mi.getOperand(1).getFrameIndex() + 6) * 8;
            const uint8_t mod = (disp >= -128 && disp <= 127) ? 1 : 2;
            emitModRM(mod, dst, 5);
            if (mod == 1) out.push_back(static_cast<uint8_t>(disp)); else emitDisp32(disp);
        } else {
            emitModRM(3, dst, getReg(1));
        }
        break;
    }
    case X86::MOV64mr: {
        emitREX(1, 0); out.push_back(0x89);
        if (numOps >= 1 && mi.getOperand(0).getKind() == MachineOperandKind::MO_FrameIndex) {
            const int disp = -(mi.getOperand(0).getFrameIndex() + 6) * 8;
            const uint8_t mod = (disp >= -128 && disp <= 127) ? 1 : 2;
            emitModRM(mod, numOps >= 2 ? getReg(1) : 0, 5);
            if (mod == 1) out.push_back(static_cast<uint8_t>(disp)); else emitDisp32(disp);
        } else {
            emitModRM(3, numOps >= 2 ? getReg(1) : 0, getReg(0));
        }
        break;
    }
    case X86::MOV32rr: out.push_back(0x89); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::NOP: out.push_back(0x90); break;
    case X86::CQO: out.push_back(0x48); out.push_back(0x99); break;
    case X86::MOVSX64rr32: emitREX(1, 0); out.push_back(0x63); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::IDIV64r: emitREXForRM(0); out.push_back(0xF7); emitModRM(3, 7, getReg(0)); break;
    case X86::ADDSDrr: out.push_back(0xF2); out.push_back(0x0F); out.push_back(0x58); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::SUBSDrr: out.push_back(0xF2); out.push_back(0x0F); out.push_back(0x5C); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::MULSDrr: out.push_back(0xF2); out.push_back(0x0F); out.push_back(0x59); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::DIVSDrr: out.push_back(0xF2); out.push_back(0x0F); out.push_back(0x5E); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::UCOMISDrr: out.push_back(0x66); out.push_back(0x0F); out.push_back(0x2E); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::CVTSI2SDrr: out.push_back(0xF2); out.push_back(0x0F); out.push_back(0x2A); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::CVTTSD2SIrr: out.push_back(0xF2); out.push_back(0x0F); out.push_back(0x2C); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::SETEr: out.push_back(0x0F); out.push_back(0x94); emitModRM(3, 0, getReg(0)); break;
    case X86::SETNEr: out.push_back(0x0F); out.push_back(0x95); emitModRM(3, 0, getReg(0)); break;
    case X86::SETLr: out.push_back(0x0F); out.push_back(0x9C); emitModRM(3, 0, getReg(0)); break;
    case X86::SETGr: out.push_back(0x0F); out.push_back(0x9F); emitModRM(3, 0, getReg(0)); break;
    case X86::SETLEr: out.push_back(0x0F); out.push_back(0x9E); emitModRM(3, 0, getReg(0)); break;
    case X86::SETGEr: out.push_back(0x0F); out.push_back(0x9D); emitModRM(3, 0, getReg(0)); break;
    case X86::JMP_1: out.push_back(0xEB); out.push_back(0x00); break;
    case X86::JE_1: out.push_back(0x74); out.push_back(0x00); break;
    case X86::JNE_1: out.push_back(0x75); out.push_back(0x00); break;
    case X86::JL_1: out.push_back(0x7C); out.push_back(0x00); break;
    case X86::JG_1: out.push_back(0x7F); out.push_back(0x00); break;
    case X86::JLE_1: out.push_back(0x7E); out.push_back(0x00); break;
    case X86::JGE_1: out.push_back(0x7D); out.push_back(0x00); break;
    case X86::JAE_1: out.push_back(0x73); out.push_back(0x00); break;
    case X86::JBE_1: out.push_back(0x76); out.push_back(0x00); break;
    case X86::JA_1: out.push_back(0x77); out.push_back(0x00); break;
    case X86::JB_1: out.push_back(0x72); out.push_back(0x00); break;
    case X86::SHL64ri: emitREXForRM(1); out.push_back(0xC1); emitModRM(3, 4, getReg(1)); emitImm8(getImm(0)); break;
    case X86::MOVSX32rr8_op: out.push_back(0x0F); out.push_back(0xBE); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::MOVZX32rr8:
    case X86::MOVZX32rr8_op: emitOptionalREX32(1, 0); out.push_back(0x0F); out.push_back(0xB6); emitModRM(3, getReg(1), getReg(0)); break;
    case X86::SETAEr: out.push_back(0x0F); out.push_back(0x93); emitModRM(3, 0, getReg(0)); break;
    case X86::SETAr: out.push_back(0x0F); out.push_back(0x97); emitModRM(3, 0, getReg(0)); break;
    case X86::SETBEr: out.push_back(0x0F); out.push_back(0x96); emitModRM(3, 0, getReg(0)); break;
    default: throw std::runtime_error("unsupported x86 opcode: " + std::to_string(opc));
    }
}

} // namespace aurora
