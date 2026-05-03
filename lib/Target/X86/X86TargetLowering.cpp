#include "Aurora/Target/X86/X86TargetLowering.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Type.h"
#include <cstring>

namespace aurora {

X86TargetLowering::X86TargetLowering() {
    X86TargetLowering::initActions();
}

LegalizeAction X86TargetLowering::getOperationAction(AIROpcode op, const unsigned vtSize) const {
    const auto opIdx = static_cast<unsigned>(op);
    if (opIdx >= 64 || vtSize >= 32) return LegalizeAction::Legal;
    return static_cast<LegalizeAction>(actionMap_[opIdx][vtSize]);
}

bool X86TargetLowering::isTypeLegal(const unsigned typeSize) const {
    if (typeSize >= sizeof(typeLegal_)) return false;
    return typeLegal_[typeSize];
}

unsigned X86TargetLowering::getRegisterSizeForType(Type* ty) const {
    if (!ty) return 64;
    const unsigned bits = ty->getSizeInBits();
    if (bits <= 8)  return 8;
    if (bits <= 16) return 16;
    if (bits <= 32) return 32;
    if (bits <= 64) return 64;
    if (bits <= 128) return 128;
    return 64;
}

void X86TargetLowering::initActions() {
    memset(actionMap_, static_cast<uint8_t>(LegalizeAction::Legal), sizeof(actionMap_));
    memset(typeLegal_, 0, sizeof(typeLegal_));

    // Types legal on x86-64
    for (const unsigned b : {8u, 16u, 32u, 64u, 128u, 256u}) {
        if (b < sizeof(typeLegal_))
            typeLegal_[b] = true;
    }

    // i1: not legal, promote to i8
    for (int op = 0; op < 64; ++op)
        actionMap_[op][1] = static_cast<uint8_t>(LegalizeAction::Promote);
    typeLegal_[1] = false;

    // i8/i16: promote to i32 for arithmetic
    unsigned promoteOps[] = {
        static_cast<unsigned>(AIROpcode::Add), static_cast<unsigned>(AIROpcode::Sub),
        static_cast<unsigned>(AIROpcode::Mul), static_cast<unsigned>(AIROpcode::UDiv),
        static_cast<unsigned>(AIROpcode::SDiv), static_cast<unsigned>(AIROpcode::URem),
        static_cast<unsigned>(AIROpcode::SRem),
        static_cast<unsigned>(AIROpcode::And), static_cast<unsigned>(AIROpcode::Or),
        static_cast<unsigned>(AIROpcode::Xor),
        static_cast<unsigned>(AIROpcode::Shl), static_cast<unsigned>(AIROpcode::LShr),
        static_cast<unsigned>(AIROpcode::AShr),
    };
    for (const auto op : promoteOps) {
        actionMap_[op][8] = static_cast<uint8_t>(LegalizeAction::Promote);
        actionMap_[op][16] = static_cast<uint8_t>(LegalizeAction::Promote);
    }

    // Memory ops legal for all sizes
    typeLegal_[8] = true;
    typeLegal_[16] = true;
}

} // namespace aurora
