#include <gtest/gtest.h>
#include "Aurora/MC/X86/X86ObjectEncoder.h"
#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"

#include <string>
#include <stdexcept>

using namespace aurora;

namespace {

std::vector<uint8_t> encode(const MachineInstr& mi) {
    X86ObjectEncoder encoder;
    std::vector<uint8_t> bytes;
    encoder.encode(mi, bytes);
    return bytes;
}

MachineInstr makeRegReg(uint16_t opcode, unsigned lhs, unsigned rhs) {
    MachineInstr mi(opcode);
    mi.addOperand(MachineOperand::createReg(lhs));
    mi.addOperand(MachineOperand::createReg(rhs));
    return mi;
}

MachineInstr makeImmReg(uint16_t opcode, int64_t imm, unsigned reg) {
    MachineInstr mi(opcode);
    mi.addOperand(MachineOperand::createImm(imm));
    mi.addOperand(MachineOperand::createReg(reg));
    return mi;
}

MachineInstr makeReg(uint16_t opcode, unsigned reg) {
    MachineInstr mi(opcode);
    mi.addOperand(MachineOperand::createReg(reg));
    return mi;
}

} // namespace

TEST(X86ObjectEncoderTest, EncodesRegisterToRegisterIntegerOps) {
    EXPECT_EQ(encode(makeRegReg(X86::MOV64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x89, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::ADD64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x01, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::SUB64rr, X86RegisterInfo::RDX, X86RegisterInfo::RBX)),
              (std::vector<uint8_t>{0x48, 0x29, 0xD3}));
    EXPECT_EQ(encode(makeRegReg(X86::XOR32rr, X86RegisterInfo::RAX, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x31, 0xC0}));
}

TEST(X86ObjectEncoderTest, EncodesImmediateIntegerOps) {
    EXPECT_EQ(encode(makeImmReg(X86::MOV64ri32, 0x12345678, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x48, 0xC7, 0xC0, 0x78, 0x56, 0x34, 0x12}));
    EXPECT_EQ(encode(makeImmReg(X86::SUB64ri32, 7, X86RegisterInfo::RBX)),
              (std::vector<uint8_t>{0x48, 0x81, 0xEB, 0x07, 0x00, 0x00, 0x00}));
    EXPECT_EQ(encode(makeImmReg(X86::AND64ri32, -1, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x81, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF}));
    EXPECT_EQ(encode(makeImmReg(X86::SHL64ri, 3, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x48, 0xC1, 0xE0, 0x03}));

    EXPECT_EQ(encode(makeImmReg(X86::ADD64ri32, 5, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x48, 0x81, 0xC0, 0x05, 0x00, 0x00, 0x00}));
    EXPECT_EQ(encode(makeImmReg(X86::OR64ri32, 2, X86RegisterInfo::RDX)),
              (std::vector<uint8_t>{0x48, 0x81, 0xCA, 0x02, 0x00, 0x00, 0x00}));
    EXPECT_EQ(encode(makeImmReg(X86::XOR64ri32, 3, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x81, 0xF1, 0x03, 0x00, 0x00, 0x00}));
    EXPECT_EQ(encode(makeImmReg(X86::CMP64ri32, 9, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x48, 0x81, 0xF8, 0x09, 0x00, 0x00, 0x00}));
}

TEST(X86ObjectEncoderTest, EncodesRemainingRegisterIntegerOps) {
    EXPECT_EQ(encode(makeRegReg(X86::IMUL64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x0F, 0xAF, 0xC8}));
    EXPECT_EQ(encode(makeRegReg(X86::AND64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x21, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::OR64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x09, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::XOR64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x31, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::CMP64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x39, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::TEST64rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x85, 0xC1}));
}

TEST(X86ObjectEncoderTest, EncodesExtendedRegistersWithRexBits) {
    EXPECT_EQ(encode(makeRegReg(X86::MOV64rr, X86RegisterInfo::R8, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x4C, 0x89, 0xC0}));
    EXPECT_EQ(encode(makeRegReg(X86::MOV64rr, X86RegisterInfo::RAX, X86RegisterInfo::R8)),
              (std::vector<uint8_t>{0x49, 0x89, 0xC0}));
    EXPECT_EQ(encode(makeImmReg(X86::MOV64ri32, 1, X86RegisterInfo::R8)),
              (std::vector<uint8_t>{0x49, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00}));
}

TEST(X86ObjectEncoderTest, EncodesStackFrameLoadsAndStores) {
    MachineInstr load(X86::MOV64rm);
    load.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    load.addOperand(MachineOperand::createFrameIndex(0));
    EXPECT_EQ(encode(load), (std::vector<uint8_t>{0x48, 0x8B, 0x45, 0xD0}));

    MachineInstr store(X86::MOV64mr);
    store.addOperand(MachineOperand::createFrameIndex(0));
    store.addOperand(MachineOperand::createReg(X86RegisterInfo::RBX));
    EXPECT_EQ(encode(store), (std::vector<uint8_t>{0x48, 0x89, 0x5D, 0xD0}));

    MachineInstr registerLoad(X86::MOV64rm);
    registerLoad.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    registerLoad.addOperand(MachineOperand::createReg(X86RegisterInfo::RCX));
    EXPECT_EQ(encode(registerLoad), (std::vector<uint8_t>{0x48, 0x8B, 0xC1}));

    MachineInstr registerStore(X86::MOV64mr);
    registerStore.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    registerStore.addOperand(MachineOperand::createReg(X86RegisterInfo::RCX));
    EXPECT_EQ(encode(registerStore), (std::vector<uint8_t>{0x48, 0x89, 0xC8}));

    MachineInstr farLoad(X86::MOV64rm);
    farLoad.addOperand(MachineOperand::createReg(X86RegisterInfo::RDX));
    farLoad.addOperand(MachineOperand::createFrameIndex(20));
    EXPECT_EQ(encode(farLoad), (std::vector<uint8_t>{0x48, 0x8B, 0x95, 0x30, 0xFF, 0xFF, 0xFF}));

    MachineInstr farStore(X86::MOV64mr);
    farStore.addOperand(MachineOperand::createFrameIndex(20));
    farStore.addOperand(MachineOperand::createReg(X86RegisterInfo::RDX));
    EXPECT_EQ(encode(farStore), (std::vector<uint8_t>{0x48, 0x89, 0x95, 0x30, 0xFF, 0xFF, 0xFF}));
}

TEST(X86ObjectEncoderTest, EncodesBranchesAndSetccPlaceholders) {
    EXPECT_EQ(encode(MachineInstr(X86::RETQ)), (std::vector<uint8_t>{0xC3}));
    EXPECT_EQ(encode(MachineInstr(X86::JMP_1)), (std::vector<uint8_t>{0xEB, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JE_1)), (std::vector<uint8_t>{0x74, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JNE_1)), (std::vector<uint8_t>{0x75, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JL_1)), (std::vector<uint8_t>{0x7C, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JG_1)), (std::vector<uint8_t>{0x7F, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JLE_1)), (std::vector<uint8_t>{0x7E, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JGE_1)), (std::vector<uint8_t>{0x7D, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JAE_1)), (std::vector<uint8_t>{0x73, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JBE_1)), (std::vector<uint8_t>{0x76, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JA_1)), (std::vector<uint8_t>{0x77, 0x00}));
    EXPECT_EQ(encode(MachineInstr(X86::JB_1)), (std::vector<uint8_t>{0x72, 0x00}));

    EXPECT_EQ(encode(makeReg(X86::SETEr, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x0F, 0x94, 0xC0}));
    EXPECT_EQ(encode(makeReg(X86::SETNEr, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x0F, 0x95, 0xC1}));
    EXPECT_EQ(encode(makeReg(X86::SETLr, X86RegisterInfo::RDX)),
              (std::vector<uint8_t>{0x0F, 0x9C, 0xC2}));
    EXPECT_EQ(encode(makeReg(X86::SETGr, X86RegisterInfo::RBX)),
              (std::vector<uint8_t>{0x0F, 0x9F, 0xC3}));
    EXPECT_EQ(encode(makeReg(X86::SETLEr, X86RegisterInfo::RSP)),
              (std::vector<uint8_t>{0x0F, 0x9E, 0xC4}));
    EXPECT_EQ(encode(makeReg(X86::SETGEr, X86RegisterInfo::RBP)),
              (std::vector<uint8_t>{0x0F, 0x9D, 0xC5}));
    MachineInstr setae(X86::SETAEr);
    setae.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));
    EXPECT_EQ(encode(setae), (std::vector<uint8_t>{0x0F, 0x93, 0xC0}));
    EXPECT_EQ(encode(makeReg(X86::SETAr, X86RegisterInfo::RSI)),
              (std::vector<uint8_t>{0x0F, 0x97, 0xC6}));
    EXPECT_EQ(encode(makeReg(X86::SETBEr, X86RegisterInfo::RDI)),
              (std::vector<uint8_t>{0x0F, 0x96, 0xC7}));
}

TEST(X86ObjectEncoderTest, EncodesUnaryAndConversionOps) {
    EXPECT_EQ(encode(makeReg(X86::PUSH64r, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x50}));
    EXPECT_EQ(encode(makeReg(X86::PUSH64r, X86RegisterInfo::R8)),
              (std::vector<uint8_t>{0x41, 0x50}));
    EXPECT_EQ(encode(makeReg(X86::POP64r, X86RegisterInfo::R15)),
              (std::vector<uint8_t>{0x41, 0x5F}));
    EXPECT_EQ(encode(makeRegReg(X86::MOV32rr, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x89, 0xC1}));
    EXPECT_EQ(encode(MachineInstr(X86::NOP)), (std::vector<uint8_t>{0x90}));
    EXPECT_EQ(encode(MachineInstr(X86::CQO)), (std::vector<uint8_t>{0x48, 0x99}));
    EXPECT_EQ(encode(makeRegReg(X86::MOVSX64rr32, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x48, 0x63, 0xC8}));
    EXPECT_EQ(encode(makeReg(X86::IDIV64r, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x48, 0xF7, 0xF8}));
    EXPECT_EQ(encode(makeRegReg(X86::MOVSX32rr8_op, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x0F, 0xBE, 0xC8}));
    EXPECT_EQ(encode(makeRegReg(X86::MOVZX32rr8_op, X86RegisterInfo::RAX, X86RegisterInfo::RCX)),
              (std::vector<uint8_t>{0x0F, 0xB6, 0xC8}));
    EXPECT_EQ(encode(makeReg(X86::CALL64pcrel32, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0x48, 0xFF, 0xD0}));
    EXPECT_THROW(encode(MachineInstr(0xFFFF)), std::runtime_error);
}

TEST(X86ObjectEncoderTest, EncodesFloatingPointOps) {
    EXPECT_EQ(encode(makeRegReg(X86::ADDSDrr, X86RegisterInfo::XMM0, X86RegisterInfo::XMM1)),
              (std::vector<uint8_t>{0xF2, 0x0F, 0x58, 0xC8}));
    EXPECT_EQ(encode(makeRegReg(X86::SUBSDrr, X86RegisterInfo::XMM0, X86RegisterInfo::XMM1)),
              (std::vector<uint8_t>{0xF2, 0x0F, 0x5C, 0xC8}));
    EXPECT_EQ(encode(makeRegReg(X86::MULSDrr, X86RegisterInfo::XMM0, X86RegisterInfo::XMM1)),
              (std::vector<uint8_t>{0xF2, 0x0F, 0x59, 0xC8}));
    EXPECT_EQ(encode(makeRegReg(X86::DIVSDrr, X86RegisterInfo::XMM0, X86RegisterInfo::XMM1)),
              (std::vector<uint8_t>{0xF2, 0x0F, 0x5E, 0xC8}));
    EXPECT_EQ(encode(makeRegReg(X86::UCOMISDrr, X86RegisterInfo::XMM0, X86RegisterInfo::XMM1)),
              (std::vector<uint8_t>{0x66, 0x0F, 0x2E, 0xC1}));
    EXPECT_EQ(encode(makeRegReg(X86::CVTSI2SDrr, X86RegisterInfo::RAX, X86RegisterInfo::XMM0)),
              (std::vector<uint8_t>{0xF2, 0x0F, 0x2A, 0xC0}));
    EXPECT_EQ(encode(makeRegReg(X86::CVTTSD2SIrr, X86RegisterInfo::XMM0, X86RegisterInfo::RAX)),
              (std::vector<uint8_t>{0xF2, 0x0F, 0x2C, 0xC0}));
}

TEST(X86ObjectEncoderTest, EmitsRelocationForGlobalImmediate) {
    X86ObjectEncoder encoder;
    MachineInstr mi(X86::MOV64ri32);
    mi.addOperand(MachineOperand::createGlobalSym("global_value"));
    mi.addOperand(MachineOperand::createReg(X86RegisterInfo::RAX));

    bool resolved = false;
    bool relocated = false;
    std::vector<uint8_t> bytes;
    encoder.encode(
        mi,
        bytes,
        16,
        [&](const char* name) -> size_t {
            resolved = std::string(name) == "global_value";
            return 3;
        },
        [&](uint64_t offset, size_t symIdx, uint32_t type, int64_t addend) {
            relocated = true;
            EXPECT_EQ(offset, 19u);
            EXPECT_EQ(symIdx, 3u);
            EXPECT_EQ(type, R_X86_64_32S);
            EXPECT_EQ(addend, 0);
        });

    EXPECT_TRUE(resolved);
    EXPECT_TRUE(relocated);
    EXPECT_EQ(bytes, (std::vector<uint8_t>{0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00}));
}

TEST(X86ObjectEncoderTest, EmitsRelocationForGlobalCall) {
    X86ObjectEncoder encoder;
    MachineInstr mi(X86::CALL64pcrel32);
    mi.addOperand(MachineOperand::createGlobalSym("callee"));

    bool resolved = false;
    bool relocated = false;
    std::vector<uint8_t> bytes;
    encoder.encode(
        mi,
        bytes,
        32,
        [&](const char* name) -> size_t {
            resolved = std::string(name) == "callee";
            return 7;
        },
        [&](uint64_t offset, size_t symIdx, uint32_t type, int64_t addend) {
            relocated = true;
            EXPECT_EQ(offset, 33u);
            EXPECT_EQ(symIdx, 7u);
            EXPECT_EQ(type, R_X86_64_PLT32);
            EXPECT_EQ(addend, -4);
        });

    EXPECT_TRUE(resolved);
    EXPECT_TRUE(relocated);
    EXPECT_EQ(bytes, (std::vector<uint8_t>{0xE8, 0x00, 0x00, 0x00, 0x00}));
}
