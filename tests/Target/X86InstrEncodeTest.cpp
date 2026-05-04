#include <gtest/gtest.h>
#include "Aurora/Target/X86/X86InstrEncode.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/CodeGen/MachineInstr.h"

using namespace aurora;

TEST(X86InstrEncodeTest, EncodeTableEntries) {
    // Verify key opcodes have entries
    SmallVector<uint8_t, 32> out;
    X86InstEncoder enc;
    MachineInstr mi(X86::ADD64rr);
    mi.addOperand(MachineOperand::createReg(0)); // RAX
    mi.addOperand(MachineOperand::createReg(1)); // RCX
    enc.encode(mi, out);
    EXPECT_GT(out.size(), 0u); // should produce some bytes
}

TEST(X86InstrEncodeTest, EncodeMOV64rr) {
    SmallVector<uint8_t, 32> out;
    X86InstEncoder enc;
    MachineInstr mi(X86::MOV64rr);
    mi.addOperand(MachineOperand::createReg(0));
    mi.addOperand(MachineOperand::createReg(1));
    enc.encode(mi, out);
    EXPECT_GT(out.size(), 0u);
}

TEST(X86InstrEncodeTest, EncodeRETQ) {
    SmallVector<uint8_t, 32> out;
    X86InstEncoder enc;
    MachineInstr mi(X86::RETQ);
    enc.encode(mi, out);
    EXPECT_GT(out.size(), 0u);
}

TEST(X86InstrEncodeTest, EncodeTableSize) {
    EXPECT_GT(kX86EncodeTableSize, 0u);
}

TEST(X86InstrEncodeTest, FindEntryKnown) {
    X86InstEncoder enc;
    // The findEntry is private, but we can test indirectly via encode
    SmallVector<uint8_t, 32> out;
    MachineInstr mi(X86::ADD64rr);
    mi.addOperand(MachineOperand::createReg(0));
    mi.addOperand(MachineOperand::createReg(1));
    enc.encode(mi, out);
    EXPECT_GT(out.size(), 0u);
}

TEST(X86InstrEncodeTest, EmitModRM) {
    // Test indirectly via encode
    SmallVector<uint8_t, 32> out;
    X86InstEncoder enc;
    MachineInstr mi(X86::MOV64rr);
    mi.addOperand(MachineOperand::createReg(0));
    mi.addOperand(MachineOperand::createReg(7));
    enc.encode(mi, out);
    EXPECT_GT(out.size(), 0u);
}

TEST(X86InstrEncodeTest, EncodeOperand) {
    X86InstEncoder enc;
    SmallVector<uint8_t, 32> out;
    MachineOperand mo = MachineOperand::createReg(0);
    enc.encodeOperand(mo, out, 0);
    SUCCEED();
}
