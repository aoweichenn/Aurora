#include "Aurora/CodeGen/ISel/X86OpcodeMapper.h"
#include "Aurora/Target/X86/X86InstrInfo.h"

namespace aurora {

uint16_t X86OpcodeMapper::getJccForCond(ICmpCond cond, bool negate) {
    switch (cond) {
    case ICmpCond::EQ:  return negate ? X86::JNE_1 : X86::JE_1;
    case ICmpCond::NE:  return negate ? X86::JE_1  : X86::JNE_1;
    case ICmpCond::SLT: return negate ? X86::JGE_1 : X86::JL_1;
    case ICmpCond::SLE: return negate ? X86::JG_1  : X86::JLE_1;
    case ICmpCond::SGT: return negate ? X86::JLE_1 : X86::JG_1;
    case ICmpCond::SGE: return negate ? X86::JL_1  : X86::JGE_1;
    case ICmpCond::ULT: return negate ? X86::JAE_1 : X86::JB_1;
    case ICmpCond::ULE: return negate ? X86::JA_1  : X86::JBE_1;
    case ICmpCond::UGT: return negate ? X86::JBE_1 : X86::JA_1;
    case ICmpCond::UGE: return negate ? X86::JB_1  : X86::JAE_1;
    }
    return X86::JMP_1;
}

uint16_t X86OpcodeMapper::getBinaryOpcode(AIROpcode airOp, unsigned sizeBits) {
    switch (airOp) {
    case AIROpcode::Add:  return sizeBits == 64 ? X86::ADD64rr : X86::ADD32rr;
    case AIROpcode::Sub:  return sizeBits == 64 ? X86::SUB64rr : X86::SUB32rr;
    case AIROpcode::Mul:  return sizeBits == 64 ? X86::IMUL64rr : X86::IMUL32rr;
    case AIROpcode::And:  return sizeBits == 64 ? X86::AND64rr : X86::AND32rr;
    case AIROpcode::Or:   return sizeBits == 64 ? X86::OR64rr  : X86::OR32rr;
    case AIROpcode::Xor:  return sizeBits == 64 ? X86::XOR64rr : X86::XOR32rr;
    case AIROpcode::SDiv: return sizeBits == 64 ? X86::IDIV64r : X86::IDIV32r;
    case AIROpcode::Shl:  return sizeBits == 64 ? X86::SHL64rCL : X86::SHL32rCL;
    case AIROpcode::LShr: return sizeBits == 64 ? X86::SHR64rCL : X86::SHR32rCL;
    case AIROpcode::AShr: return sizeBits == 64 ? X86::SAR64rCL : X86::SAR32rCL;
    default: return 0;
    }
}

} // namespace aurora
