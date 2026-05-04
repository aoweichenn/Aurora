#pragma once

#include <cstdint>
#include "Aurora/Air/Instruction.h"

namespace aurora {

class X86OpcodeMapper {
public:
    [[nodiscard]] static uint16_t getJccForCond(ICmpCond cond, bool negate);
    [[nodiscard]] static uint16_t getBinaryOpcode(AIROpcode airOp, unsigned sizeBits);
};

} // namespace aurora
