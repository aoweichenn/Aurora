#include "Aurora/Target/X86/X86InstrEncode.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include <cstring>

namespace aurora {

// x86-64 instruction encoding table
// Format per entry:
// {opcode, {prefixes}, numPrefixes, {baseOpcode}, opcodeSize, hasModRM, hasSIB, hasREX, immSize, dispSize}

const X86EncodeEntry X86EncodeTable[] = {
    // ADD64rr: REX.W + 0x01 /r
    {X86::ADD64rr, {}, 0, {0x01}, 1, true, false, true, 0, 0},
    // ADD64ri8: REX.W + 0x83 /0 ib
    {X86::ADD64ri8, {}, 0, {0x83}, 1, true, false, true, 1, 0},
    // ADD64ri32: REX.W + 0x81 /0 id
    {X86::ADD64ri32, {}, 0, {0x81}, 1, true, false, true, 4, 0},
    // ADD32rr: 0x01 /r
    {X86::ADD32rr, {}, 0, {0x01}, 1, true, false, false, 0, 0},
    // ADD32ri: 0x81 /0 id
    {X86::ADD32ri, {}, 0, {0x81}, 1, true, false, false, 4, 0},

    // SUB64rr: REX.W + 0x29 /r
    {X86::SUB64rr, {}, 0, {0x29}, 1, true, false, true, 0, 0},
    // SUB64ri8: REX.W + 0x83 /5 ib
    {X86::SUB64ri8, {}, 0, {0x83}, 1, true, false, true, 1, 0},
    // SUB64ri32: REX.W + 0x81 /5 id
    {X86::SUB64ri32, {}, 0, {0x81}, 1, true, false, true, 4, 0},
    // SUB32rr: 0x29 /r
    {X86::SUB32rr, {}, 0, {0x29}, 1, true, false, false, 0, 0},
    // SUB32ri: 0x81 /5 id
    {X86::SUB32ri, {}, 0, {0x81}, 1, true, false, false, 4, 0},

    // IMUL64rr: REX.W + 0x0F 0xAF /r
    {X86::IMUL64rr, {}, 0, {0x0F, 0xAF}, 2, true, false, true, 0, 0},
    // IMUL32rr: 0x0F 0xAF /r
    {X86::IMUL32rr, {}, 0, {0x0F, 0xAF}, 2, true, false, false, 0, 0},

    // AND64rr: REX.W + 0x21 /r
    {X86::AND64rr, {}, 0, {0x21}, 1, true, false, true, 0, 0},
    // AND64ri8: REX.W + 0x83 /4 ib
    {X86::AND64ri8, {}, 0, {0x83}, 1, true, false, true, 1, 0},
    // AND64ri32: REX.W + 0x81 /4 id
    {X86::AND64ri32, {}, 0, {0x81}, 1, true, false, true, 4, 0},
    // AND32rr: 0x21 /r
    {X86::AND32rr, {}, 0, {0x21}, 1, true, false, false, 0, 0},
    // AND32ri: 0x81 /4 id
    {X86::AND32ri, {}, 0, {0x81}, 1, true, false, false, 4, 0},

    // OR64rr: REX.W + 0x09 /r
    {X86::OR64rr, {}, 0, {0x09}, 1, true, false, true, 0, 0},
    // OR64ri8: REX.W + 0x83 /1 ib
    {X86::OR64ri8, {}, 0, {0x83}, 1, true, false, true, 1, 0},
    // OR64ri32: REX.W + 0x81 /1 id
    {X86::OR64ri32, {}, 0, {0x81}, 1, true, false, true, 4, 0},
    // OR32rr: 0x09 /r
    {X86::OR32rr, {}, 0, {0x09}, 1, true, false, false, 0, 0},
    // OR32ri: 0x81 /1 id
    {X86::OR32ri, {}, 0, {0x81}, 1, true, false, false, 4, 0},

    // XOR64rr: REX.W + 0x31 /r
    {X86::XOR64rr, {}, 0, {0x31}, 1, true, false, true, 0, 0},
    // XOR64ri8: REX.W + 0x83 /6 ib
    {X86::XOR64ri8, {}, 0, {0x83}, 1, true, false, true, 1, 0},
    // XOR64ri32: REX.W + 0x81 /6 id
    {X86::XOR64ri32, {}, 0, {0x81}, 1, true, false, true, 4, 0},
    // XOR32rr: 0x31 /r
    {X86::XOR32rr, {}, 0, {0x31}, 1, true, false, false, 0, 0},
    // XOR32ri: 0x81 /6 id
    {X86::XOR32ri, {}, 0, {0x81}, 1, true, false, false, 4, 0},

    // MOV64rr: REX.W + 0x89 /r
    {X86::MOV64rr, {}, 0, {0x89}, 1, true, false, true, 0, 0},
    // MOV64ri32: REX.W + 0xC7 /0 id
    {X86::MOV64ri32, {}, 0, {0xC7}, 1, true, false, true, 4, 0},
    // MOV64rm: REX.W + 0x8B /r
    {X86::MOV64rm, {}, 0, {0x8B}, 1, true, false, true, 0, 0},
    // MOV64mr: REX.W + 0x89 /r
    {X86::MOV64mr, {}, 0, {0x89}, 1, true, false, true, 0, 0},
    // MOV32rr: 0x89 /r
    {X86::MOV32rr, {}, 0, {0x89}, 1, true, false, false, 0, 0},
    // MOV32ri: 0xC7 /0 id
    {X86::MOV32ri, {}, 0, {0xC7}, 1, true, false, false, 4, 0},

    // CMP64rr: REX.W + 0x39 /r
    {X86::CMP64rr, {}, 0, {0x39}, 1, true, false, true, 0, 0},
    // CMP64ri8: REX.W + 0x83 /7 ib
    {X86::CMP64ri8, {}, 0, {0x83}, 1, true, false, true, 1, 0},
    // CMP32rr: 0x39 /r
    {X86::CMP32rr, {}, 0, {0x39}, 1, true, false, false, 0, 0},

    // SETEr: REX + 0x0F 0x94 /r (actually /r is the 8-bit reg dest)
    {X86::SETEr, {}, 0, {0x0F, 0x94}, 2, true, false, false, 0, 0},
    // SETNEr:
    {X86::SETNEr, {}, 0, {0x0F, 0x95}, 2, true, false, false, 0, 0},
    // SETGr:
    {X86::SETGr, {}, 0, {0x0F, 0x9F}, 2, true, false, false, 0, 0},
    // SETLr:
    {X86::SETLr, {}, 0, {0x0F, 0x9C}, 2, true, false, false, 0, 0},
    // SETGEr:
    {X86::SETGEr, {}, 0, {0x0F, 0x9D}, 2, true, false, false, 0, 0},
    // SETLEr:
    {X86::SETLEr, {}, 0, {0x0F, 0x9E}, 2, true, false, false, 0, 0},

    // JMP_1: 0xEB cb
    {X86::JMP_1, {}, 0, {0xEB}, 1, false, false, false, 1, 0},
    // JMP_4: 0xE9 cd
    {X86::JMP_4, {}, 0, {0xE9}, 1, false, false, false, 4, 0},
    // JE_1: 0x74 cb
    {X86::JE_1, {}, 0, {0x74}, 1, false, false, false, 1, 0},
    // JE_4: 0x0F 0x84 cd
    {X86::JE_4, {}, 0, {0x0F, 0x84}, 2, false, false, false, 4, 0},
    // JNE_1: 0x75 cb
    {X86::JNE_1, {}, 0, {0x75}, 1, false, false, false, 1, 0},
    // JNE_4: 0x0F 0x85 cd
    {X86::JNE_4, {}, 0, {0x0F, 0x85}, 2, false, false, false, 4, 0},
    // JL_1: 0x7C cb
    {X86::JL_1, {}, 0, {0x7C}, 1, false, false, false, 1, 0},
    // JL_4: 0x0F 0x8C cd
    {X86::JL_4, {}, 0, {0x0F, 0x8C}, 2, false, false, false, 4, 0},
    // JG_1: 0x7F cb
    {X86::JG_1, {}, 0, {0x7F}, 1, false, false, false, 1, 0},
    // JG_4: 0x0F 0x8F cd
    {X86::JG_4, {}, 0, {0x0F, 0x8F}, 2, false, false, false, 4, 0},
    // JLE_1: 0x7E cb
    {X86::JLE_1, {}, 0, {0x7E}, 1, false, false, false, 1, 0},
    // JLE_4: 0x0F 0x8E cd
    {X86::JLE_4, {}, 0, {0x0F, 0x8E}, 2, false, false, false, 4, 0},
    // JGE_1: 0x7D cb
    {X86::JGE_1, {}, 0, {0x7D}, 1, false, false, false, 1, 0},
    // JGE_4: 0x0F 0x8D cd
    {X86::JGE_4, {}, 0, {0x0F, 0x8D}, 2, false, false, false, 4, 0},

    // CALL64pcrel32: 0xE8 cd
    {X86::CALL64pcrel32, {}, 0, {0xE8}, 1, false, false, false, 4, 0},
    // RETQ: 0xC3
    {X86::RETQ, {}, 0, {0xC3}, 1, false, false, false, 0, 0},

    // PUSH64r: 0x50 + rd
    {X86::PUSH64r, {}, 0, {0x50}, 1, false, false, false, 0, 0},
    // POP64r: 0x58 + rd
    {X86::POP64r, {}, 0, {0x58}, 1, false, false, false, 0, 0},

    // SHL64rCL: REX.W + 0xD3 /4
    {X86::SHL64rCL, {}, 0, {0xD3}, 1, true, false, true, 0, 0},
    // SHR64rCL: REX.W + 0xD3 /5
    {X86::SHR64rCL, {}, 0, {0xD3}, 1, true, false, true, 0, 0},
    // SAR64rCL: REX.W + 0xD3 /7
    {X86::SAR64rCL, {}, 0, {0xD3}, 1, true, false, true, 0, 0},

    // NOP: 0x90
    {X86::NOP, {}, 0, {0x90}, 1, false, false, false, 0, 0},
};

void X86InstEncoder::encode(const MachineInstr& mi, SmallVector<uint8_t, 32>& out) {
    const X86EncodeEntry* entry = findEntry(0 /* mi.getOpcode() */);
    if (!entry) return;

    emitPrefixes(entry, out);
    emitOpcode(entry, out);

    if (entry->hasModRM) {
        emitModRM(0, 0, 0, out); // simplified
    }
    if (entry->immSize > 0) {
        emitImm(0, entry->immSize, out);
    }
}

void X86InstEncoder::encodeOperand(const MachineOperand& mo, SmallVector<uint8_t, 32>& out, unsigned opIdx) {
    // Encode a single operand
}

const X86EncodeEntry* X86InstEncoder::findEntry(uint16_t opcode) const {
    for (const auto& e : X86EncodeTable) {
        if (e.opcode == opcode) return &e;
    }
    return nullptr;
}

void X86InstEncoder::emitPrefixes(const X86EncodeEntry* e, SmallVector<uint8_t, 32>& out) {
    for (uint8_t i = 0; i < e->numPrefixes; ++i)
        out.push_back(e->prefixes[i]);
    if (e->hasREX) {
        // REX.W prefix for 64-bit operations
        out.push_back(0x48); // REX.W
    }
}

void X86InstEncoder::emitOpcode(const X86EncodeEntry* e, SmallVector<uint8_t, 32>& out) {
    for (uint8_t i = 0; i < e->opcodeSize; ++i)
        out.push_back(e->baseOpcode[i]);
}

void X86InstEncoder::emitModRM(uint8_t mod, uint8_t regOp, uint8_t rm, SmallVector<uint8_t, 32>& out) {
    out.push_back((mod << 6) | ((regOp & 7) << 3) | (rm & 7));
}

void X86InstEncoder::emitSIB(uint8_t scale, uint8_t index, uint8_t base, SmallVector<uint8_t, 32>& out) {
    out.push_back((scale << 6) | ((index & 7) << 3) | (base & 7));
}

void X86InstEncoder::emitDisp(int64_t disp, unsigned size, SmallVector<uint8_t, 32>& out) {
    for (unsigned i = 0; i < size; ++i)
        out.push_back(static_cast<uint8_t>((disp >> (i * 8)) & 0xFF));
}

void X86InstEncoder::emitImm(uint64_t imm, unsigned size, SmallVector<uint8_t, 32>& out) {
    for (unsigned i = 0; i < size; ++i)
        out.push_back(static_cast<uint8_t>((imm >> (i * 8)) & 0xFF));
}

} // namespace aurora
