#include "minic/codegen/CodeGen.h"
#include "minic/ast/ASTUtils.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <stdexcept>

namespace minic {

unsigned CodeGen::genExpr(const Expr& node) {
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&node))
        return genBinaryExpr(*binary);
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&node))
        return genUnaryExpr(*unary);
    if (auto* cast = dynamic_cast<const CastExpr*>(&node))
        return genCastExpr(*cast);
    if (auto* assign = dynamic_cast<const AssignExpr*>(&node))
        return genAssignExpr(*assign);
    if (auto* incDec = dynamic_cast<const IncDecExpr*>(&node))
        return genIncDecExpr(*incDec);
    if (auto* index = dynamic_cast<const IndexExpr*>(&node))
        return genIndexExpr(*index);
    if (auto* member = dynamic_cast<const MemberExpr*>(&node))
        return genMemberExpr(*member);
    if (auto* sizeofExpr = dynamic_cast<const SizeofExpr*>(&node))
        return genSizeofExpr(*sizeofExpr);
    if (auto* alignofExpr = dynamic_cast<const AlignofExpr*>(&node))
        return genAlignofExpr(*alignofExpr);
    if (auto* comma = dynamic_cast<const CommaExpr*>(&node))
        return genCommaExpr(*comma);
    if (auto* call = dynamic_cast<const CallExpr*>(&node))
        return genCallExpr(*call);
    if (auto* conditional = dynamic_cast<const ConditionalExpr*>(&node))
        return genConditionalExpr(*conditional);
    if (auto* literal = dynamic_cast<const IntLitExpr*>(&node))
        return genIntLitExpr(*literal);
    if (auto* variable = dynamic_cast<const VarExpr*>(&node))
        return genVarExpr(*variable);
    throw std::runtime_error("Unknown expression node");
}

unsigned CodeGen::genConditionValue(const Expr& expr) {
    unsigned value = genExpr(expr);
    unsigned zero = builder_->createConstantInt(0);
    return builder_->createICmp(aurora::ICmpCond::NE, value, zero);
}

unsigned CodeGen::genIntLitExpr(const IntLitExpr& ie) const {
    return builder_->createConstantInt(ie.value);
}

unsigned CodeGen::genVarExpr(const VarExpr& ve) {
    if (auto* local = findVariableInScopes(ve.name)) {
        if (local->type.arraySize > 0)
            return genAddressOfVariable(ve);
        if (local->type.kind == CTypeKind::Struct && local->type.pointerDepth == 0)
            throw std::runtime_error("Struct values cannot be loaded directly: " + ve.name);
        return builder_->createLoad(toAirType(local->type, false), local->pointerVReg);
    }
    auto& variable = findGlobal(ve.name);
    if (variable.type.arraySize > 0)
        return genGlobalAddress(ve.name);
    if (variable.type.kind == CTypeKind::Struct && variable.type.pointerDepth == 0)
        throw std::runtime_error("Struct values cannot be loaded directly: " + ve.name);
    return builder_->createLoad(toAirType(variable.type, false), genGlobalAddress(ve.name));
}

unsigned CodeGen::genAddressOfVariable(const VarExpr& ve) {
    auto* local = findVariableInScopes(ve.name);
    if (!local)
        return genGlobalAddress(ve.name);
    auto& variable = *local;
    CType addressType = variable.type.arraySize > 0 ? variable.type.decayArray() : variable.type;
    if (variable.type.arraySize == 0)
        ++addressType.pointerDepth;
    aurora::SmallVector<unsigned, 4> indices;
    return builder_->createGEP(toAirType(addressType, false), variable.pointerVReg, indices);
}

CodeGen::LValue CodeGen::genLValue(const Expr& expr) {
    if (auto* variableExpr = dynamic_cast<const VarExpr*>(&expr)) {
        if (auto* variable = findVariableInScopes(variableExpr->name))
            return {variable->type, variable->pointerVReg};
        auto& global = findGlobal(variableExpr->name);
        return {global.type, genGlobalAddress(variableExpr->name)};
    }

    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (unary->op == UnaryExpr::Deref) {
            CType pointeeType = inferExprType(*unary->operand).pointee();
            return {pointeeType, genExpr(*unary->operand)};
        }
    }

    if (auto* index = dynamic_cast<const IndexExpr*>(&expr)) {
        CType baseType = inferExprType(*index->base).decayArray();
        if (!baseType.isPointerLike())
            throw std::runtime_error("Subscript base must be a pointer-like expression");
        CType elementType = baseType.pointee();
        unsigned base = genExpr(*index->base);
        unsigned indexValue = genExpr(*index->index);
        unsigned elementSize = builder_->createConstantInt(static_cast<int64_t>(sizeOfType(elementType)));
        unsigned offset = builder_->createMul(aurora::Type::getInt64Ty(), indexValue, elementSize);
        unsigned address = builder_->createAdd(aurora::Type::getInt64Ty(), base, offset);
        return {elementType, address};
    }

    if (auto* member = dynamic_cast<const MemberExpr*>(&expr))
        return genMemberLValue(*member);

    throw std::runtime_error("Expression is not assignable");
}

unsigned CodeGen::genBinaryExpr(const BinaryExpr& be) {
    auto* intTy = aurora::Type::getInt64Ty();
    CType lhsType = inferExprType(*be.lhs).decayArray();
    CType rhsType = inferExprType(*be.rhs).decayArray();
    const bool lhsPointer = lhsType.isPointerLike();
    const bool rhsPointer = rhsType.isPointerLike();
    const bool useUnsigned = lhsType.isUnsigned || rhsType.isUnsigned;
    const bool useUnsignedCompare = lhsPointer || rhsPointer || useUnsigned;

    if (be.op == BinaryExpr::LogAnd || be.op == BinaryExpr::LogOr) {
        auto* function = builder_->getInsertBlock()->getParent();
        const unsigned conditionalId = conditionalCounter_++;
        auto* rhsBlock = function->createBasicBlock("logic.rhs." + std::to_string(conditionalId));
        auto* mergeBlock = function->createBasicBlock("logic.end." + std::to_string(conditionalId));
        unsigned lhs = genConditionValue(*be.lhs);
        auto* lhsBlock = builder_->getInsertBlock();

        if (be.op == BinaryExpr::LogAnd) {
            unsigned falseValue = builder_->createConstantInt(0);
            builder_->createCondBr(lhs, rhsBlock, mergeBlock);
            builder_->setInsertPoint(rhsBlock);
            unsigned rhs = genConditionValue(*be.rhs);
            auto* rhsIncomingBlock = builder_->getInsertBlock();
            branchToIfOpen(mergeBlock);

            builder_->setInsertPoint(mergeBlock);
            aurora::SmallVector<std::pair<aurora::BasicBlock*, unsigned>, 4> incomings;
            incomings.push_back({lhsBlock, falseValue});
            incomings.push_back({rhsIncomingBlock, rhs});
            return builder_->createPhi(aurora::Type::getInt1Ty(), incomings);
        }

        unsigned trueValue = builder_->createConstantInt(1);
        builder_->createCondBr(lhs, mergeBlock, rhsBlock);
        builder_->setInsertPoint(rhsBlock);
        unsigned rhs = genConditionValue(*be.rhs);
        auto* rhsIncomingBlock = builder_->getInsertBlock();
        branchToIfOpen(mergeBlock);

        builder_->setInsertPoint(mergeBlock);
        aurora::SmallVector<std::pair<aurora::BasicBlock*, unsigned>, 4> incomings;
        incomings.push_back({lhsBlock, trueValue});
        incomings.push_back({rhsIncomingBlock, rhs});
        return builder_->createPhi(aurora::Type::getInt1Ty(), incomings);
    }

    unsigned lhs;
    unsigned rhs;
    if (containsCall(*be.rhs) && !containsCall(*be.lhs)) {
        rhs = genExpr(*be.rhs);
        lhs = genExpr(*be.lhs);
    } else {
        lhs = genExpr(*be.lhs);
        rhs = genExpr(*be.rhs);
    }

    switch (be.op) {
    case BinaryExpr::Add:
        if (lhsPointer && !rhsPointer)
            return builder_->createAdd(intTy, lhs, scalePointerOffset(lhsType, rhs));
        if (!lhsPointer && rhsPointer)
            return builder_->createAdd(intTy, rhs, scalePointerOffset(rhsType, lhs));
        if (lhsPointer && rhsPointer)
            throw std::runtime_error("Cannot add two pointers");
        return builder_->createAdd(intTy, lhs, rhs);
    case BinaryExpr::Sub:
        if (lhsPointer && !rhsPointer)
            return builder_->createSub(intTy, lhs, scalePointerOffset(lhsType, rhs));
        if (lhsPointer && rhsPointer) {
            unsigned byteDiff = builder_->createSub(intTy, lhs, rhs);
            uint64_t elementSize = sizeOfType(lhsType.pointee());
            if (elementSize <= 1)
                return byteDiff;
            return builder_->createSDiv(intTy, byteDiff, builder_->createConstantInt(static_cast<int64_t>(elementSize)));
        }
        if (!lhsPointer && rhsPointer)
            throw std::runtime_error("Cannot subtract a pointer from an integer");
        return builder_->createSub(intTy, lhs, rhs);
    case BinaryExpr::Mul: return builder_->createMul(intTy, lhs, rhs);
    case BinaryExpr::Div: return useUnsigned ? builder_->createUDiv(intTy, lhs, rhs) : builder_->createSDiv(intTy, lhs, rhs);
    case BinaryExpr::Rem: return genRemainder(lhsType, rhsType, lhs, rhs);
    case BinaryExpr::Eq: return builder_->createICmp(aurora::ICmpCond::EQ, lhs, rhs);
    case BinaryExpr::Ne: return builder_->createICmp(aurora::ICmpCond::NE, lhs, rhs);
    case BinaryExpr::Lt: return builder_->createICmp(useUnsignedCompare ? aurora::ICmpCond::ULT : aurora::ICmpCond::SLT, lhs, rhs);
    case BinaryExpr::Le: return builder_->createICmp(useUnsignedCompare ? aurora::ICmpCond::ULE : aurora::ICmpCond::SLE, lhs, rhs);
    case BinaryExpr::Gt: return builder_->createICmp(useUnsignedCompare ? aurora::ICmpCond::UGT : aurora::ICmpCond::SGT, lhs, rhs);
    case BinaryExpr::Ge: return builder_->createICmp(useUnsignedCompare ? aurora::ICmpCond::UGE : aurora::ICmpCond::SGE, lhs, rhs);
    case BinaryExpr::BitAnd: return builder_->createAnd(intTy, lhs, rhs);
    case BinaryExpr::BitOr: return builder_->createOr(intTy, lhs, rhs);
    case BinaryExpr::BitXor: return builder_->createXor(intTy, lhs, rhs);
    case BinaryExpr::Shl: return builder_->createShl(intTy, lhs, rhs);
    case BinaryExpr::Shr: return lhsType.isUnsigned ? builder_->createLShr(intTy, lhs, rhs) : builder_->createAShr(intTy, lhs, rhs);
    case BinaryExpr::LogAnd:
    case BinaryExpr::LogOr:
        break;
    }
    throw std::runtime_error("Unknown binary operator");
}

unsigned CodeGen::genUnaryExpr(const UnaryExpr& ue) {
    auto* intTy = aurora::Type::getInt64Ty();
    switch (ue.op) {
    case UnaryExpr::Plus: {
        unsigned value = genExpr(*ue.operand);
        return value;
    }
    case UnaryExpr::Neg: {
        unsigned value = genExpr(*ue.operand);
        return builder_->createSub(intTy, builder_->createConstantInt(0), value);
    }
    case UnaryExpr::LogicalNot: {
        unsigned value = genExpr(*ue.operand);
        return builder_->createICmp(aurora::ICmpCond::EQ, value, builder_->createConstantInt(0));
    }
    case UnaryExpr::BitNot: {
        unsigned value = genExpr(*ue.operand);
        return builder_->createXor(intTy, value, builder_->createConstantInt(-1));
    }
    case UnaryExpr::AddressOf:
        if (auto* variable = dynamic_cast<const VarExpr*>(ue.operand.get()))
            return genAddressOfVariable(*variable);
        return genLValue(*ue.operand).pointerVReg;
    case UnaryExpr::Deref: {
        LValue lvalue = genLValue(ue);
        return builder_->createLoad(toAirType(lvalue.type, false), lvalue.pointerVReg);
    }
    }
    throw std::runtime_error("Unknown unary operator");
}

unsigned CodeGen::genCastExpr(const CastExpr& ce) {
    (void)ce.targetType;
    return genExpr(*ce.operand);
}

unsigned CodeGen::genAssignExpr(const AssignExpr& ae) {
    LValue lvalue = genLValue(*ae.target);
    CType lhsType = lvalue.type.decayArray();
    CType rhsType = inferExprType(*ae.value).decayArray();
    auto* intTy = aurora::Type::getInt64Ty();
    unsigned rhs = genExpr(*ae.value);
    unsigned result = rhs;

    if (ae.op != AssignExpr::Assign) {
        if (lhsType.isPointerLike() && rhsType.isPointerLike())
            throw std::runtime_error("Pointer compound assignment requires an integer offset");
        if (lhsType.isPointerLike() && ae.op != AssignExpr::AddAssign && ae.op != AssignExpr::SubAssign)
            throw std::runtime_error("Only += and -= are supported for pointer compound assignment");
        unsigned lhs = builder_->createLoad(toAirType(lvalue.type, false), lvalue.pointerVReg);
        switch (ae.op) {
        case AssignExpr::AddAssign:
            result = builder_->createAdd(intTy, lhs, lhsType.isPointerLike() ? scalePointerOffset(lhsType, rhs) : rhs);
            break;
        case AssignExpr::SubAssign:
            result = builder_->createSub(intTy, lhs, lhsType.isPointerLike() ? scalePointerOffset(lhsType, rhs) : rhs);
            break;
        case AssignExpr::MulAssign: result = builder_->createMul(intTy, lhs, rhs); break;
        case AssignExpr::DivAssign: result = lhsType.isUnsigned ? builder_->createUDiv(intTy, lhs, rhs) : builder_->createSDiv(intTy, lhs, rhs); break;
        case AssignExpr::RemAssign: result = genRemainder(lhsType, rhsType, lhs, rhs); break;
        case AssignExpr::BitAndAssign: result = builder_->createAnd(intTy, lhs, rhs); break;
        case AssignExpr::BitOrAssign: result = builder_->createOr(intTy, lhs, rhs); break;
        case AssignExpr::BitXorAssign: result = builder_->createXor(intTy, lhs, rhs); break;
        case AssignExpr::ShlAssign: result = builder_->createShl(intTy, lhs, rhs); break;
        case AssignExpr::ShrAssign: result = lhsType.isUnsigned ? builder_->createLShr(intTy, lhs, rhs) : builder_->createAShr(intTy, lhs, rhs); break;
        case AssignExpr::Assign:
            break;
        }
    }

    builder_->createStore(result, lvalue.pointerVReg);
    return result;
}

unsigned CodeGen::genIncDecExpr(const IncDecExpr& ie) {
    LValue lvalue = genLValue(*ie.target);
    CType type = lvalue.type.decayArray();
    auto* intTy = aurora::Type::getInt64Ty();
    unsigned oldValue = builder_->createLoad(toAirType(lvalue.type, false), lvalue.pointerVReg);
    unsigned one = type.isPointerLike()
        ? builder_->createConstantInt(static_cast<int64_t>(sizeOfType(type.pointee())))
        : builder_->createConstantInt(1);
    unsigned newValue = ie.increment
        ? builder_->createAdd(intTy, oldValue, one)
        : builder_->createSub(intTy, oldValue, one);
    builder_->createStore(newValue, lvalue.pointerVReg);
    return ie.prefix ? newValue : oldValue;
}

unsigned CodeGen::genIndexExpr(const IndexExpr& ie) {
    LValue lvalue = genLValue(ie);
    return builder_->createLoad(toAirType(lvalue.type, false), lvalue.pointerVReg);
}

CodeGen::LValue CodeGen::genMemberLValue(const MemberExpr& me) {
    CType objectType;
    unsigned baseAddress = 0;
    if (me.viaPointer) {
        CType pointerType = inferExprType(*me.base).decayArray();
        if (!pointerType.isPointerLike())
            throw std::runtime_error("Member access with -> requires a pointer to struct");
        objectType = pointerType.pointee();
        baseAddress = genExpr(*me.base);
    } else {
        LValue base = genLValue(*me.base);
        objectType = base.type;
        baseAddress = base.pointerVReg;
    }

    if (objectType.kind != CTypeKind::Struct || objectType.pointerDepth != 0)
        throw std::runtime_error("Member access requires a struct object");
    const CField* field = findStructField(objectType, me.field);
    if (!field)
        throw std::runtime_error("Unknown struct field: " + me.field);

    unsigned fieldAddress = baseAddress;
    if (field->offset != 0) {
        if (!me.viaPointer) {
            aurora::SmallVector<unsigned, 4> indices;
            baseAddress = builder_->createGEP(toAirType(objectType, false), baseAddress, indices);
        }
        fieldAddress = builder_->createAdd(
            aurora::Type::getInt64Ty(),
            baseAddress,
            builder_->createConstantInt(static_cast<int64_t>(field->offset)));
    }
    return {field->type, fieldAddress};
}

unsigned CodeGen::genMemberExpr(const MemberExpr& me) {
    LValue lvalue = genMemberLValue(me);
    if (lvalue.type.arraySize > 0)
        return lvalue.pointerVReg;
    if (lvalue.type.kind == CTypeKind::Struct && lvalue.type.pointerDepth == 0)
        throw std::runtime_error("Struct values cannot be loaded directly");
    return builder_->createLoad(toAirType(lvalue.type, false), lvalue.pointerVReg);
}

unsigned CodeGen::genSizeofExpr(const SizeofExpr& se) {
    CType type = se.type;
    if (se.expr) {
        if (auto* variable = dynamic_cast<const VarExpr*>(se.expr.get())) {
            if (auto* local = findVariableInScopes(variable->name))
                type = local->type;
            else
                type = findGlobal(variable->name).type;
        } else {
            type = inferExprType(*se.expr);
        }
    }
    return builder_->createConstantInt(static_cast<int64_t>(sizeOfType(type)));
}

unsigned CodeGen::genAlignofExpr(const AlignofExpr& ae) {
    return builder_->createConstantInt(static_cast<int64_t>(alignOfType(ae.type)));
}

unsigned CodeGen::genCommaExpr(const CommaExpr& ce) {
    (void)genExpr(*ce.lhs);
    return genExpr(*ce.rhs);
}

unsigned CodeGen::genCallExpr(const CallExpr& ce) {
    auto functionIt = functionMap_.find(ce.callee);
    if (functionIt == functionMap_.end())
        throw std::runtime_error("Unknown function: " + ce.callee);
    auto* functionType = functionIt->second->getFunctionType();
    if (!functionType->isVarArg() && ce.args.size() != functionType->getNumParams())
        throw std::runtime_error("Wrong number of arguments in call to: " + ce.callee);

    aurora::SmallVector<unsigned, 8> args;
    for (const auto& arg : ce.args)
        args.push_back(genExpr(*arg));
    return builder_->createCall(functionIt->second, args);
}

unsigned CodeGen::genConditionalExpr(const ConditionalExpr& ce) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned conditionalId = conditionalCounter_++;
    auto* trueBlock = function->createBasicBlock("cond.true." + std::to_string(conditionalId));
    auto* falseBlock = function->createBasicBlock("cond.false." + std::to_string(conditionalId));
    auto* mergeBlock = function->createBasicBlock("cond.end." + std::to_string(conditionalId));

    builder_->createCondBr(genConditionValue(*ce.cond), trueBlock, falseBlock);

    builder_->setInsertPoint(trueBlock);
    unsigned trueValue = genExpr(*ce.trueExpr);
    branchToIfOpen(mergeBlock);

    builder_->setInsertPoint(falseBlock);
    unsigned falseValue = genExpr(*ce.falseExpr);
    branchToIfOpen(mergeBlock);

    builder_->setInsertPoint(mergeBlock);
    aurora::SmallVector<std::pair<aurora::BasicBlock*, unsigned>, 4> incomings;
    incomings.push_back({trueBlock, trueValue});
    incomings.push_back({falseBlock, falseValue});
    return builder_->createPhi(aurora::Type::getInt64Ty(), incomings);
}

} // namespace minic
