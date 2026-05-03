#ifndef AURORA_X86_X86ISELPATTERNS_H
#define AURORA_X86_X86ISELPATTERNS_H

#include "Aurora/Air/Instruction.h"
#include <vector>

namespace aurora {

class Type;
class SDNode;
class MachineInstr;

enum class OperandKind : uint8_t {
    Reg, Imm, Mem, None
};

enum class OperandSize : uint8_t {
    S8, S16, S32, S64, S128, S256, Any
};

struct ISelPattern {
    AIROpcode airOp;
    unsigned operandCount;
    OperandKind opKinds[4];
    OperandSize opSizes[4];
    uint16_t x86Opcode;
    // 0=first air op goes to src, 1=second air op goes to src
    // -1 means it's a dest-only pattern
    int8_t srcOperandMap[4];
};

struct ISelMatchResult {
    uint16_t x86Opcode;
    bool matched;
};

class X86ISelPatterns {
public:
    static ISelMatchResult matchPattern(AIROpcode airOp, Type* resultTy,
                                        const std::vector<unsigned>& vregTypes,
                                        unsigned op0, unsigned op1);

    static const std::vector<ISelPattern>& getAllPatterns();
};

} // namespace aurora

#endif // AURORA_X86_X86ISELPATTERNS_H
