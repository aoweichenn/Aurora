#include "minic/codegen/CodeGen.h"
#include "minic/ast/ASTUtils.h"
#include <stdexcept>

namespace minic {

bool CodeGen::containsCall(const Expr& expr) const {
    if (dynamic_cast<const CallExpr*>(&expr))
        return true;
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr))
        return containsCall(*binary->lhs) || containsCall(*binary->rhs);
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr))
        return containsCall(*unary->operand);
    if (auto* cast = dynamic_cast<const CastExpr*>(&expr))
        return containsCall(*cast->operand);
    if (auto* assign = dynamic_cast<const AssignExpr*>(&expr))
        return containsCall(*assign->target) || containsCall(*assign->value);
    if (auto* incDec = dynamic_cast<const IncDecExpr*>(&expr))
        return containsCall(*incDec->target);
    if (auto* index = dynamic_cast<const IndexExpr*>(&expr))
        return containsCall(*index->base) || containsCall(*index->index);
    if (auto* sizeofExpr = dynamic_cast<const SizeofExpr*>(&expr))
        return sizeofExpr->expr && containsCall(*sizeofExpr->expr);
    if (auto* comma = dynamic_cast<const CommaExpr*>(&expr))
        return containsCall(*comma->lhs) || containsCall(*comma->rhs);
    if (auto* conditional = dynamic_cast<const ConditionalExpr*>(&expr))
        return containsCall(*conditional->cond) || containsCall(*conditional->trueExpr) || containsCall(*conditional->falseExpr);
    return false;
}

uint64_t CodeGen::sizeOfType(CType type) const {
    return minic::sizeOfType(type);
}

uint64_t CodeGen::alignOfType(CType type) const {
    return minic::alignOfType(type);
}

int64_t CodeGen::evalConstantExpr(const Expr& expr) const {
    if (auto* literal = dynamic_cast<const IntLitExpr*>(&expr))
        return literal->value;
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        int64_t value = evalConstantExpr(*unary->operand);
        switch (unary->op) {
        case UnaryExpr::Plus: return value;
        case UnaryExpr::Neg: return -value;
        case UnaryExpr::LogicalNot: return value == 0;
        case UnaryExpr::BitNot: return ~value;
        case UnaryExpr::AddressOf:
        case UnaryExpr::Deref:
            break;
        }
    }
    if (auto* cast = dynamic_cast<const CastExpr*>(&expr))
        return evalConstantExpr(*cast->operand);
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) {
        int64_t lhs = evalConstantExpr(*binary->lhs);
        int64_t rhs = evalConstantExpr(*binary->rhs);
        switch (binary->op) {
        case BinaryExpr::Add: return lhs + rhs;
        case BinaryExpr::Sub: return lhs - rhs;
        case BinaryExpr::Mul: return lhs * rhs;
        case BinaryExpr::Div: return rhs == 0 ? 0 : lhs / rhs;
        case BinaryExpr::Rem: return rhs == 0 ? 0 : lhs % rhs;
        case BinaryExpr::Eq: return lhs == rhs;
        case BinaryExpr::Ne: return lhs != rhs;
        case BinaryExpr::Lt: return lhs < rhs;
        case BinaryExpr::Le: return lhs <= rhs;
        case BinaryExpr::Gt: return lhs > rhs;
        case BinaryExpr::Ge: return lhs >= rhs;
        case BinaryExpr::BitAnd: return lhs & rhs;
        case BinaryExpr::BitOr: return lhs | rhs;
        case BinaryExpr::BitXor: return lhs ^ rhs;
        case BinaryExpr::Shl: return lhs << rhs;
        case BinaryExpr::Shr: return lhs >> rhs;
        case BinaryExpr::LogAnd: return lhs && rhs;
        case BinaryExpr::LogOr: return lhs || rhs;
        }
    }
    if (auto* sizeofExpr = dynamic_cast<const SizeofExpr*>(&expr))
        return static_cast<int64_t>(sizeofExpr->expr ? 8 : sizeOfType(sizeofExpr->type));
    if (auto* alignofExpr = dynamic_cast<const AlignofExpr*>(&expr))
        return static_cast<int64_t>(alignOfType(alignofExpr->type));
    throw std::runtime_error("case label must be an integer constant expression");
}

CType CodeGen::inferExprType(const Expr& expr) {
    if (auto* variable = dynamic_cast<const VarExpr*>(&expr)) {
        if (auto* local = findVariableInScopes(variable->name))
            return local->type.decayArray();
        return findGlobal(variable->name).type.decayArray();
    }
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (unary->op == UnaryExpr::AddressOf) {
            CType type = inferExprType(*unary->operand);
            if (type.arraySize > 0)
                return type.decayArray();
            ++type.pointerDepth;
            return type;
        }
        if (unary->op == UnaryExpr::Deref)
            return inferExprType(*unary->operand).pointee();
        return CType{CTypeKind::Long};
    }
    if (auto* cast = dynamic_cast<const CastExpr*>(&expr))
        return cast->targetType;
    if (auto* index = dynamic_cast<const IndexExpr*>(&expr))
        return inferExprType(*index->base).decayArray().pointee();
    if (auto* assign = dynamic_cast<const AssignExpr*>(&expr))
        return inferExprType(*assign->target);
    if (auto* incDec = dynamic_cast<const IncDecExpr*>(&expr))
        return inferExprType(*incDec->target);
    if (auto* conditional = dynamic_cast<const ConditionalExpr*>(&expr))
        return inferExprType(*conditional->trueExpr);
    if (auto* comma = dynamic_cast<const CommaExpr*>(&expr))
        return inferExprType(*comma->rhs);
    if (dynamic_cast<const AlignofExpr*>(&expr))
        return CType{CTypeKind::Long};
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) {
        CType lhsType = inferExprType(*binary->lhs).decayArray();
        CType rhsType = inferExprType(*binary->rhs).decayArray();
        if (binary->op == BinaryExpr::Add) {
            if (lhsType.isPointerLike() && !rhsType.isPointerLike())
                return lhsType;
            if (!lhsType.isPointerLike() && rhsType.isPointerLike())
                return rhsType;
        }
        if (binary->op == BinaryExpr::Sub && lhsType.isPointerLike() && !rhsType.isPointerLike())
            return lhsType;
        if (binary->op == BinaryExpr::Shl || binary->op == BinaryExpr::Shr)
            return lhsType;
        CType result{CTypeKind::Long};
        result.isUnsigned = lhsType.isUnsigned || rhsType.isUnsigned;
        return result;
    }
    if (auto* call = dynamic_cast<const CallExpr*>(&expr)) {
        auto it = functionReturnTypes_.find(call->callee);
        if (it != functionReturnTypes_.end())
            return it->second;
    }
    return CType{CTypeKind::Long};
}

unsigned CodeGen::scalePointerOffset(CType pointerType, unsigned value) {
    pointerType = pointerType.decayArray();
    if (!pointerType.isPointerLike())
        return value;
    const uint64_t elementSize = sizeOfType(pointerType.pointee());
    if (elementSize == 1)
        return value;
    return builder_->createMul(
        aurora::Type::getInt64Ty(),
        value,
        builder_->createConstantInt(static_cast<int64_t>(elementSize)));
}

unsigned CodeGen::genRemainder(CType lhsType, CType rhsType, unsigned lhs, unsigned rhs) {
    auto* intTy = aurora::Type::getInt64Ty();
    const bool useUnsigned = lhsType.isUnsigned || rhsType.isUnsigned;
    unsigned quotient = useUnsigned ? builder_->createUDiv(intTy, lhs, rhs) : builder_->createSDiv(intTy, lhs, rhs);
    unsigned product = builder_->createMul(intTy, quotient, rhs);
    return builder_->createSub(intTy, lhs, product);
}

} // namespace minic
