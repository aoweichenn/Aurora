#include "Aurora/Target/AArch64/AArch64CallingConv.h"
#include "Aurora/Target/AArch64/AArch64RegisterInfo.h"
#include "Aurora/Air/Type.h"

namespace aurora {

SmallVector<CCValAssign, 8> AArch64CallingConv::analyzeArguments(Type** argTypes, const unsigned numArgs) const {
    SmallVector<CCValAssign, 8> result;
    unsigned regIdx = 0;
    unsigned stackOff = 0;
    static const unsigned ArgRegs[] = {
        AArch64RegisterInfo::X0, AArch64RegisterInfo::X1, AArch64RegisterInfo::X2, AArch64RegisterInfo::X3,
        AArch64RegisterInfo::X4, AArch64RegisterInfo::X5, AArch64RegisterInfo::X6, AArch64RegisterInfo::X7,
    };

    for (unsigned i = 0; i < numArgs; ++i) {
        const Type* ty = argTypes[i];
        const unsigned size = ty ? ty->getSizeInBits() / 8 : 8;
        if (regIdx < 8 && ty && (ty->isInteger() || ty->isPointer())) {
            result.push_back({CCValAssign::GPR, ArgRegs[regIdx++], size, 0});
        } else {
            result.push_back({CCValAssign::Stack, 0, size, stackOff});
            stackOff += (size + 7u) & ~7u;
        }
    }
    return result;
}

SmallVector<CCValAssign, 2> AArch64CallingConv::analyzeReturn(Type* retTy) const {
    SmallVector<CCValAssign, 2> result;
    if (!retTy || retTy->isVoid()) return result;
    result.push_back({CCValAssign::GPR, AArch64RegisterInfo::X0, retTy->getSizeInBits() / 8, 0});
    return result;
}

} // namespace aurora
