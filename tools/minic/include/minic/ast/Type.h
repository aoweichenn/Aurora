#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

struct CStructInfo;

enum class CTypeKind { Void, Bool, Char, Short, Int, Long, Struct, Union };

struct CType {
    CTypeKind kind = CTypeKind::Long;
    unsigned pointerDepth = 0;
    uint64_t arraySize = 0;
    bool isUnsigned = false;
    std::shared_ptr<CStructInfo> structInfo;

    CType() = default;
    explicit CType(CTypeKind k) : kind(k) {}

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

struct CField {
    CType type;
    std::string name;
    uint64_t offset = 0;

    CField(CType t, std::string n) : type(std::move(t)), name(std::move(n)) {}
};

struct CStructInfo {
    std::string tag;
    std::vector<CField> fields;
    uint64_t size = 0;
    uint64_t align = 1;
    bool complete = false;
    bool isUnion = false;
};

} // namespace minic
