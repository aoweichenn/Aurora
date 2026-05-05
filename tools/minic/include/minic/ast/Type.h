#pragma once

#include <cstdint>

namespace minic {

enum class CTypeKind { Void, Bool, Char, Short, Int, Long };

struct CType {
    CTypeKind kind = CTypeKind::Long;
    unsigned pointerDepth = 0;
    uint64_t arraySize = 0;
    bool isUnsigned = false;

    [[nodiscard]] bool isVoid() const noexcept { return kind == CTypeKind::Void && pointerDepth == 0 && arraySize == 0; }
    [[nodiscard]] bool isPointerLike() const noexcept { return pointerDepth > 0 || arraySize > 0; }

    [[nodiscard]] CType decayArray() const noexcept {
        CType result = *this;
        if (result.arraySize > 0) {
            result.arraySize = 0;
            ++result.pointerDepth;
        }
        return result;
    }

    [[nodiscard]] CType pointee() const noexcept {
        CType result = *this;
        if (result.arraySize > 0) {
            result.arraySize = 0;
            return result;
        }
        if (result.pointerDepth > 0)
            --result.pointerDepth;
        return result;
    }
};

} // namespace minic
