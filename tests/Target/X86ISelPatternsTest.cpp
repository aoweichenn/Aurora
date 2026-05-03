#include <gtest/gtest.h>
#include "Aurora/Target/X86/X86ISelPatterns.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Air/Type.h"
#include <vector>

using namespace aurora;

TEST(X86ISelPatternsTest, GetAllPatterns) {
    const auto& patterns = X86ISelPatterns::getAllPatterns();
    EXPECT_FALSE(patterns.empty());
    EXPECT_GT(patterns.size(), 5u); // We registered at least a few
}

TEST(X86ISelPatternsTest, MatchAdd64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Add, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::ADD64rr);
}

TEST(X86ISelPatternsTest, MatchAdd32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Add, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::ADD32rr);
}

TEST(X86ISelPatternsTest, MatchSub64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Sub, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::SUB64rr);
}

TEST(X86ISelPatternsTest, MatchMul64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Mul, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::IMUL64rr);
}

TEST(X86ISelPatternsTest, MatchAnd64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::And, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::AND64rr);
}

TEST(X86ISelPatternsTest, MatchOr64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Or, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::OR64rr);
}

TEST(X86ISelPatternsTest, MatchXor64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Xor, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::XOR64rr);
}

TEST(X86ISelPatternsTest, MatchShl64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Shl, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::SHL64rCL);
}

TEST(X86ISelPatternsTest, MatchLShr64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::LShr, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
}

TEST(X86ISelPatternsTest, NoMatchStore) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Store, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_FALSE(result.matched);
}

TEST(X86ISelPatternsTest, MatchSub32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Sub, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::SUB32rr);
}

TEST(X86ISelPatternsTest, MatchMul32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Mul, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::IMUL32rr);
}

TEST(X86ISelPatternsTest, MatchAnd32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::And, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::AND32rr);
}

TEST(X86ISelPatternsTest, MatchOr32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Or, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::OR32rr);
}

TEST(X86ISelPatternsTest, MatchXor32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Xor, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::XOR32rr);
}

TEST(X86ISelPatternsTest, MatchICmp64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::ICmp, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::CMP64rr);
}

TEST(X86ISelPatternsTest, MatchICmp32) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::ICmp, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::CMP32rr);
}

TEST(X86ISelPatternsTest, MatchSDiv64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::SDiv, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::IDIV64r);
}

TEST(X86ISelPatternsTest, MatchRet) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Ret, nullptr, vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::RETQ);
}

TEST(X86ISelPatternsTest, MatchBr) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Br, nullptr, vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::JMP_1);
}

TEST(X86ISelPatternsTest, MatchCondBr) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::CondBr, nullptr, vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::JE_1);
}

TEST(X86ISelPatternsTest, MatchAShr64) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::AShr, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.x86Opcode, X86::SAR64rCL);
}

TEST(X86ISelPatternsTest, NoMatchAlloca) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Alloca, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_FALSE(result.matched);
}

TEST(X86ISelPatternsTest, NoMatchLoad) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Load, Type::getInt64Ty(), vregTypes, 0, 0);
    EXPECT_FALSE(result.matched);
}

TEST(X86ISelPatternsTest, NoMatchTrunc) {
    const std::vector<unsigned> vregTypes;
    const auto result = X86ISelPatterns::matchPattern(AIROpcode::Trunc, Type::getInt32Ty(), vregTypes, 0, 0);
    EXPECT_FALSE(result.matched);
}

TEST(X86ISelPatternsTest, PatternsCount) {
    const auto& patterns = X86ISelPatterns::getAllPatterns();
    EXPECT_GE(patterns.size(), 20u); // After adding branches, shifts, cmp, etc.
}
