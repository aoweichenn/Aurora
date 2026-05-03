#ifndef AURORA_X86_X86CALLINGCONV_H
#define AURORA_X86_X86CALLINGCONV_H

#include "Aurora/Target/TargetCallingConv.h"

namespace aurora {

class X86CallingConv : public TargetCallingConv {
public:
    SmallVector<CCValAssign, 8> analyzeArguments(
        Type** argTypes, unsigned numArgs) const override;

    SmallVector<CCValAssign, 2> analyzeReturn(Type* retTy) const override;

    unsigned getStackAlignment() const override { return 16; }
    unsigned getShadowStoreSize() const override { return 0; }
};

} // namespace aurora

#endif // AURORA_X86_X86CALLINGCONV_H
