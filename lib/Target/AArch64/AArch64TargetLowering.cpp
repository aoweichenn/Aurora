#include "Aurora/Target/AArch64/AArch64TargetLowering.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Type.h"
#include <cstring>

namespace aurora {

AArch64TargetLowering::AArch64TargetLowering() {
    AArch64TargetLowering::initActions();
}

LegalizeAction AArch64TargetLowering::getOperationAction(AIROpcode op, const unsigned vtSize) const {
    const auto opIdx = static_cast<unsigned>(op);
    if (opIdx >= 64 || vtSize >= 32) return LegalizeAction::Legal;
    return static_cast<LegalizeAction>(actionMap_[opIdx][vtSize]);
}

bool AArch64TargetLowering::isTypeLegal(const unsigned typeSize) const {
    if (typeSize >= sizeof(typeLegal_)) return false;
    return typeLegal_[typeSize];
}

unsigned AArch64TargetLowering::getRegisterSizeForType(Type* ty) const {
    if (!ty) return 64;
    const unsigned bits = ty->getSizeInBits();
    if (bits <= 32) return 32;
    if (bits <= 64) return 64;
    if (bits <= 128) return 128;
    return 64;
}

void AArch64TargetLowering::initActions() {
    memset(actionMap_, static_cast<uint8_t>(LegalizeAction::Legal), sizeof(actionMap_));
    memset(typeLegal_, 0, sizeof(typeLegal_));

    for (const unsigned bits : {8u, 16u, 32u, 64u, 128u}) {
        if (bits < sizeof(typeLegal_)) typeLegal_[bits] = true;
    }

    for (int op = 0; op < 64; ++op)
        actionMap_[op][1] = static_cast<uint8_t>(LegalizeAction::Promote);
    typeLegal_[1] = false;
}

} // namespace aurora
