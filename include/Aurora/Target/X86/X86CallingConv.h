#pragma once

#include "Aurora/Target/TargetCallingConv.h"

namespace aurora {

class X86CallingConv : public TargetCallingConv {
public:
    [[nodiscard]] SmallVector<CCValAssign, 8> analyzeArguments(
        Type** argTypes, unsigned numArgs) const override;

    [[nodiscard]] SmallVector<CCValAssign, 2> analyzeReturn(Type* retTy) const override;

    [[nodiscard]] unsigned getStackAlignment() const override { return 16; }
    [[nodiscard]] unsigned getShadowStoreSize() const override { return 0; }
};

} // namespace aurora

