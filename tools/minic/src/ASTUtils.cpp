#include "minic/ASTUtils.h"

namespace minic {

bool isAssignableExpr(const Expr& expr) noexcept {
    if (dynamic_cast<const VarExpr*>(&expr))
        return true;
    if (dynamic_cast<const IndexExpr*>(&expr))
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
    }
    return 8;
}

} // namespace minic
