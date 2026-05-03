#include "Aurora/Target/X86/X86CallingConv.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Air/Type.h"

namespace aurora {

SmallVector<CCValAssign, 8> X86CallingConv::analyzeArguments(
    Type** argTypes, unsigned numArgs) const {
    SmallVector<CCValAssign, 8> result;
    unsigned gprIdx = 0, xmmIdx = 0;
    unsigned stackOff = 8; // skip return address

    static const unsigned ArgGPRs[] = {
        X86RegisterInfo::RDI, X86RegisterInfo::RSI, X86RegisterInfo::RDX,
        X86RegisterInfo::RCX, X86RegisterInfo::R8,  X86RegisterInfo::R9
    };
    static const unsigned ArgXMMs[] = {
        X86RegisterInfo::XMM0, X86RegisterInfo::XMM1,
        X86RegisterInfo::XMM2, X86RegisterInfo::XMM3,
        X86RegisterInfo::XMM4, X86RegisterInfo::XMM5,
        X86RegisterInfo::XMM6, X86RegisterInfo::XMM7,
    };

    for (unsigned i = 0; i < numArgs; ++i) {
        Type* ty = argTypes[i];
        unsigned size = ty->getSizeInBits() / 8;

        if (ty->isInteger() || ty->isPointer()) {
            if (gprIdx < 6) {
                result.push_back({CCValAssign::GPR, ArgGPRs[gprIdx], size, 0});
                gprIdx++;
            } else {
                result.push_back({CCValAssign::Stack, 0, size, stackOff});
                stackOff += 8;
            }
        } else if (ty->isFloat()) {
            if (xmmIdx < 8) {
                result.push_back({CCValAssign::XMM, ArgXMMs[xmmIdx], size, 0});
                xmmIdx++;
            } else {
                result.push_back({CCValAssign::Stack, 0, size, stackOff});
                stackOff += 8;
            }
        } else {
            // Aggregate: pass on stack
            result.push_back({CCValAssign::Stack, 0, size, stackOff});
            stackOff += (size + 7u) & ~7u;
        }
    }
    return result;
}

SmallVector<CCValAssign, 2> X86CallingConv::analyzeReturn(Type* retTy) const {
    SmallVector<CCValAssign, 2> result;
    if (!retTy || retTy->isVoid()) return result;

    if (retTy->isInteger() || retTy->isPointer()) {
        result.push_back({CCValAssign::GPR, X86RegisterInfo::RAX,
                          retTy->getSizeInBits() / 8, 0});
        if (retTy->getSizeInBits() > 64) {
            result.push_back({CCValAssign::GPR, X86RegisterInfo::RDX,
                              retTy->getSizeInBits() / 8 - 8, 0});
        }
    } else if (retTy->isFloat()) {
        result.push_back({CCValAssign::XMM, X86RegisterInfo::XMM0,
                          retTy->getSizeInBits() / 8, 0});
    }
    return result;
}

} // namespace aurora
