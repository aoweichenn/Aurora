#ifndef AURORA_TARGET_TARGETCALLINGCONV_H
#define AURORA_TARGET_TARGETCALLINGCONV_H

#include "Aurora/ADT/SmallVector.h"
#include <cstdint>

namespace aurora {

class Type;
class Register;

struct CCValAssign {
    enum LocType { GPR, XMM, Stack };
    LocType loc;
    unsigned regId;
    unsigned size;
    unsigned stackOffset;
    bool isReg() const { return loc == GPR || loc == XMM; }
};

class TargetCallingConv {
public:
    virtual ~TargetCallingConv() = default;

    virtual SmallVector<CCValAssign, 8> analyzeArguments(
        Type** argTypes, unsigned numArgs) const = 0;

    virtual SmallVector<CCValAssign, 2> analyzeReturn(Type* retTy) const = 0;

    virtual unsigned getStackAlignment() const = 0;
    virtual unsigned getShadowStoreSize() const = 0;
};

} // namespace aurora

#endif // AURORA_TARGET_TARGETCALLINGCONV_H
