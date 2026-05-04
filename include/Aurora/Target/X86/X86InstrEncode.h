#pragma once

#include <cstdint>
#include "Aurora/ADT/SmallVector.h"

namespace aurora {

class MachineInstr;
class MachineOperand;

struct X86EncodeEntry {
    uint16_t opcode;
    uint8_t prefixes[4];
    uint8_t numPrefixes;
    uint8_t baseOpcode[3];
    uint8_t opcodeSize;
    uint8_t hasModRM : 1;
    uint8_t hasSIB : 1;
    uint8_t hasREX : 1;
    uint8_t immSize : 3;  // 0,1,2,4,8
    uint8_t dispSize : 3;
};

class X86InstEncoder {
public:
    void encode(const MachineInstr& mi, SmallVector<uint8_t, 32>& out);
    void encodeOperand(const MachineOperand& mo, SmallVector<uint8_t, 32>& out, unsigned opIdx) const;

private:
    [[nodiscard]] const X86EncodeEntry* findEntry(uint16_t opcode) const;
    void emitPrefixes(const X86EncodeEntry* e, SmallVector<uint8_t, 32>& out) const;
    void emitOpcode(const X86EncodeEntry* e, SmallVector<uint8_t, 32>& out) const;
    void emitModRM(uint8_t mod, uint8_t regOp, uint8_t rm, SmallVector<uint8_t, 32>& out) const;
    void emitSIB(uint8_t scale, uint8_t index, uint8_t base, SmallVector<uint8_t, 32>& out) const;
    void emitDisp(int64_t disp, unsigned size, SmallVector<uint8_t, 32>& out) const;
    void emitImm(uint64_t imm, unsigned size, SmallVector<uint8_t, 32>& out) const;
};

extern const X86EncodeEntry X86EncodeTable[];
extern const unsigned kX86EncodeTableSize;

} // namespace aurora

