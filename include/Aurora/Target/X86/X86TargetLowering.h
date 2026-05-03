#ifndef AURORA_X86_X86TARGETLOWERING_H
#define AURORA_X86_X86TARGETLOWERING_H

#include "Aurora/Target/TargetLowering.h"

namespace aurora {

class X86TargetLowering : public TargetLowering {
public:
    X86TargetLowering();

    [[nodiscard]] LegalizeAction getOperationAction(AIROpcode op, unsigned vtSize) const override;
    [[nodiscard]] bool isTypeLegal(unsigned typeSize) const override;
    unsigned getRegisterSizeForType(Type* ty) const override;

private:
    void initActions() override;

    unsigned actionMap_[64][32]; // [opcode][typeSize]
    bool typeLegal_[257]; // indexed by bit size up to 256
};

} // namespace aurora

#endif // AURORA_X86_X86TARGETLOWERING_H
