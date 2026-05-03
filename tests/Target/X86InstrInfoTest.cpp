#include <gtest/gtest.h>
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"

using namespace aurora;

TEST(X86InstrInfoTest, NumOpcodes) {
    X86RegisterInfo ri;
    X86InstrInfo ii(ri);
    EXPECT_EQ(ii.getNumOpcodes(), X86::NUM_OPS);
}

TEST(X86InstrInfoTest, OpcodeDescriptions) {
    X86RegisterInfo ri;
    X86InstrInfo ii(ri);

    // Test that known opcodes have descriptions
    const auto& addDesc = ii.get(X86::ADD64rr);
    EXPECT_STREQ(addDesc.asmString, "addq\t$src, $dst");
    EXPECT_EQ(addDesc.numOperands, 2u);

    const auto& movDesc = ii.get(X86::MOV64rr);
    EXPECT_STREQ(movDesc.asmString, "movq\t$src, $dst");
    EXPECT_TRUE(movDesc.isMove);
    EXPECT_EQ(movDesc.numOperands, 2u);
}

TEST(X86InstrInfoTest, TerminatorOpcodes) {
    X86RegisterInfo ri;
    X86InstrInfo ii(ri);

    const auto& retDesc = ii.get(X86::RETQ);
    EXPECT_TRUE(retDesc.isTerminator);
    EXPECT_TRUE(retDesc.isReturn);

    const auto& jmpDesc = ii.get(X86::JMP_1);
    EXPECT_TRUE(jmpDesc.isTerminator);
    EXPECT_TRUE(jmpDesc.isBranch);

    const auto& callDesc = ii.get(X86::CALL64pcrel32);
    EXPECT_TRUE(callDesc.isCall);
}

TEST(X86InstrInfoTest, CompareOpcodes) {
    X86RegisterInfo ri;
    X86InstrInfo ii(ri);

    const auto& cmpDesc = ii.get(X86::CMP64rr);
    EXPECT_TRUE(cmpDesc.isCompare);
    EXPECT_STREQ(cmpDesc.asmString, "cmpq\t$src, $dst");
}

TEST(X86InstrInfoTest, GetMoveOpcode) {
    X86RegisterInfo ri;
    X86InstrInfo ii(ri);

    EXPECT_EQ(ii.getMoveOpcode(64, 64), X86::MOV64rr);
    EXPECT_EQ(ii.getMoveOpcode(32, 32), X86::MOV32rr);
}

TEST(X86InstrInfoTest, GetArithOpcode) {
    X86RegisterInfo ri;
    X86InstrInfo ii(ri);

    // 0=add, size=64, reg
    EXPECT_EQ(ii.getArithOpcode(0, 64, false), X86::ADD64rr);
    // 1=sub, size=32, imm
    EXPECT_EQ(ii.getArithOpcode(1, 32, true), X86::SUB32ri);
    // 2=and, size=64, reg
    EXPECT_EQ(ii.getArithOpcode(2, 64, false), X86::AND64rr);
    // 3=or, size=32, reg
    EXPECT_EQ(ii.getArithOpcode(3, 32, false), X86::OR32rr);
    // 4=xor, size=64, reg
    EXPECT_EQ(ii.getArithOpcode(4, 64, false), X86::XOR64rr);
}
