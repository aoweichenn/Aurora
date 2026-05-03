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
