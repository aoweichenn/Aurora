#include "minic/ast/ASTUtils.h"

namespace minic {

bool isAssignableExpr(const Expr& expr) noexcept {
    if (dynamic_cast<const VarExpr*>(&expr))
        return true;
    if (dynamic_cast<const IndexExpr*>(&expr))
        return true;
    if (dynamic_cast<const MemberExpr*>(&expr))
        return true;
    if (dynamic_cast<const CompoundLiteralExpr*>(&expr))
        return true;
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr))
        return unary->op == UnaryExpr::Deref;
    return false;
}

uint64_t sizeOfType(CType type) noexcept {
    if (type.arraySize > 0) {
        CType elementType = type;
        elementType.arraySize = 0;
        return type.arraySize * sizeOfType(elementType);
    }
    if (type.pointerDepth > 0)
        return 8;
    switch (type.kind) {
    case CTypeKind::Bool: return 1;
    case CTypeKind::Char: return 1;
    case CTypeKind::Short: return 2;
    case CTypeKind::Int: return 4;
    case CTypeKind::Long: return 8;
    case CTypeKind::Void: return 1;
    case CTypeKind::Struct:
    case CTypeKind::Union:
        return type.structInfo && type.structInfo->complete ? type.structInfo->size : 0;
    }
    return 8;
}

uint64_t alignOfType(CType type) noexcept {
    if (type.arraySize > 0) {
        CType elementType = type;
        elementType.arraySize = 0;
        return alignOfType(elementType);
    }
    if (type.pointerDepth > 0)
        return 8;
    switch (type.kind) {
    case CTypeKind::Bool: return 1;
    case CTypeKind::Char: return 1;
    case CTypeKind::Short: return 2;
    case CTypeKind::Int: return 4;
    case CTypeKind::Long: return 8;
    case CTypeKind::Void: return 1;
    case CTypeKind::Struct:
    case CTypeKind::Union:
        return type.structInfo && type.structInfo->complete ? type.structInfo->align : 1;
    }
    return 8;
}

const CField* findStructField(const CType& type, const std::string& name) noexcept {
    if ((type.kind != CTypeKind::Struct && type.kind != CTypeKind::Union) || !type.structInfo || !type.structInfo->complete)
        return nullptr;
    for (const auto& field : type.structInfo->fields) {
        if (field.name == name)
            return &field;
    }
    return nullptr;
}

} // namespace minic
