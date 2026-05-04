#pragma once

#include "Aurora/Target/TargetLowering.h"

namespace aurora {

class AArch64TargetLowering : public TargetLowering {
public:
    AArch64TargetLowering();

    [[nodiscard]] LegalizeAction getOperationAction(AIROpcode op, unsigned vtSize) const override;
    [[nodiscard]] bool isTypeLegal(unsigned typeSize) const override;
    [[nodiscard]] unsigned getRegisterSizeForType(Type* ty) const override;

private:
    void initActions() override;

    unsigned actionMap_[64][32];
    bool typeLegal_[257];
};

} // namespace aurora
