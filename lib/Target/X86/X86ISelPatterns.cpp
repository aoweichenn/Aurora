#include "Aurora/Target/X86/X86ISelPatterns.h"
#include "Aurora/Air/Type.h"

namespace aurora {

static std::vector<ISelPattern> g_patterns;

static void initPatterns() {
    if (!g_patterns.empty()) return;

    // add i64 rr
    g_patterns.push_back({AIROpcode::Add, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::S64}, X86::ADD64rr, {0, 1}});
    // add i64 ri
    g_patterns.push_back({AIROpcode::Add, 2, {OperandKind::Reg, OperandKind::Imm}, {OperandSize::S64, OperandSize::S64}, X86::ADD64ri32, {0, 1}});
    // add i32 rr
    g_patterns.push_back({AIROpcode::Add, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S32, OperandSize::S32}, X86::ADD32rr, {0, 1}});
    // add i32 ri
    g_patterns.push_back({AIROpcode::Add, 2, {OperandKind::Reg, OperandKind::Imm}, {OperandSize::S32, OperandSize::S32}, X86::ADD32ri, {0, 1}});

    // sub i64 rr
    g_patterns.push_back({AIROpcode::Sub, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::S64}, X86::SUB64rr, {0, 1}});
    // sub i64 ri
    g_patterns.push_back({AIROpcode::Sub, 2, {OperandKind::Reg, OperandKind::Imm}, {OperandSize::S64, OperandSize::S64}, X86::SUB64ri32, {0, 1}});
    // sub i32 rr
    g_patterns.push_back({AIROpcode::Sub, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S32, OperandSize::S32}, X86::SUB32rr, {0, 1}});

    // mul i64 rr
    g_patterns.push_back({AIROpcode::Mul, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::S64}, X86::IMUL64rr, {0, 1}});
    // mul i32 rr
    g_patterns.push_back({AIROpcode::Mul, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S32, OperandSize::S32}, X86::IMUL32rr, {0, 1}});

    // and i64 rr
    g_patterns.push_back({AIROpcode::And, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::S64}, X86::AND64rr, {0, 1}});
    // and i32 rr
    g_patterns.push_back({AIROpcode::And, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S32, OperandSize::S32}, X86::AND32rr, {0, 1}});

    // or i64 rr
    g_patterns.push_back({AIROpcode::Or, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::S64}, X86::OR64rr, {0, 1}});
    // or i32 rr
    g_patterns.push_back({AIROpcode::Or, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S32, OperandSize::S32}, X86::OR32rr, {0, 1}});

    // xor i64 rr
    g_patterns.push_back({AIROpcode::Xor, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::S64}, X86::XOR64rr, {0, 1}});
    // xor i32 rr
    g_patterns.push_back({AIROpcode::Xor, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S32, OperandSize::S32}, X86::XOR32rr, {0, 1}});

    // shl i64
    g_patterns.push_back({AIROpcode::Shl, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::Any}, X86::SHL64rCL, {0, 1}});
    // shr i64
    g_patterns.push_back({AIROpcode::LShr, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::Any}, X86::SHR64rCL, {0, 1}});
    // ashr i64
    g_patterns.push_back({AIROpcode::AShr, 2, {OperandKind::Reg, OperandKind::Reg}, {OperandSize::S64, OperandSize::Any}, X86::SAR64rCL, {0, 1}});
}

ISelMatchResult X86ISelPatterns::matchPattern(const AIROpcode airOp, Type* resultTy,
                                               const std::vector<unsigned>& /*vregTypes*/,
                                               unsigned /*op0*/, unsigned /*op1*/) {
    initPatterns();
    ISelMatchResult ret = {0, false};

    const unsigned resSize = resultTy ? resultTy->getSizeInBits() : 0;

    for (const auto& pat : g_patterns) {
        if (pat.airOp != airOp) continue;
        if (pat.operandCount == 2 && pat.opSizes[0] == OperandSize::S64 && resSize != 64) continue;
        if (pat.operandCount == 2 && pat.opSizes[0] == OperandSize::S32 && resSize != 32) continue;

        // Matched
        ret.x86Opcode = pat.x86Opcode;
        ret.matched = true;
        return ret;
    }
    return ret;
}

const std::vector<ISelPattern>& X86ISelPatterns::getAllPatterns() {
    initPatterns();
    return g_patterns;
}

} // namespace aurora
