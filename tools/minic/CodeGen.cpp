#include "CodeGen.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <stdexcept>

namespace minic {

CodeGen::CodeGen() : builder_(nullptr) {}

aurora::Type* CodeGen::toAirType(CType type, bool allowVoid) const {
    if (type.arraySize > 0) {
        CType elementType = type;
        elementType.arraySize = 0;
        return aurora::Type::getArrayTy(toAirType(elementType, false), static_cast<unsigned>(type.arraySize));
    }

    if (type.pointerDepth > 0) {
        CType elementType = type;
        elementType.pointerDepth = 0;
        aurora::Type* airType = toAirType(elementType, true);
        for (unsigned depth = 0; depth < type.pointerDepth; ++depth)
            airType = aurora::Type::getPointerTy(airType);
        return airType;
    }

    switch (type.kind) {
    case CTypeKind::Void:
        if (!allowVoid)
            throw std::runtime_error("void is not a scalar value type in MiniC");
        return aurora::Type::getVoidTy();
    case CTypeKind::Bool:
    case CTypeKind::Char:
    case CTypeKind::Short:
    case CTypeKind::Int:
    case CTypeKind::Long:
        return aurora::Type::getInt64Ty();
    }
    return aurora::Type::getInt64Ty();
}

std::unique_ptr<aurora::Module> CodeGen::generate(const std::vector<Function>& functions) {
    module_ = std::make_unique<aurora::Module>("minic");
    functionMap_.clear();
    functionReturnTypes_.clear();

    for (const auto& function : functions) {
        if (functionMap_.find(function.name) != functionMap_.end())
            throw std::runtime_error("Duplicate function: " + function.name);

        aurora::SmallVector<aurora::Type*, 8> paramTypes;
        for (const auto& param : function.params)
            paramTypes.push_back(toAirType(param.type, false));

        auto* functionType = new aurora::FunctionType(toAirType(function.returnType), paramTypes);
        functionMap_[function.name] = module_->createFunction(functionType, function.name);
        functionReturnTypes_[function.name] = function.returnType;
    }

    for (const auto& function : functions) {
        auto* airFunction = functionMap_.at(function.name);
        aurora::AIRBuilder builder(airFunction->getEntryBlock());
        builder_ = &builder;
        currentReturnType_ = function.returnType;
        ifCounter_ = 0;
        loopCounter_ = 0;
        conditionalCounter_ = 0;
        switchCounter_ = 0;
        breakStack_.clear();
        continueStack_.clear();
        scopes_.clear();
        pushScope();

        for (size_t paramIndex = 0; paramIndex < function.params.size(); ++paramIndex) {
            const auto& param = function.params[paramIndex];
            if (param.type.isVoid())
                throw std::runtime_error("Parameter cannot have void type: " + param.name);
            unsigned pointer = builder_->createAlloca(toAirType(param.type, false));
            builder_->createStore(static_cast<unsigned>(paramIndex), pointer);
            scopes_.back()[param.name] = Variable{param.type, pointer};
        }

        if (auto* block = dynamic_cast<const BlockStmt*>(function.body.get()))
            genBlock(*block, false);
        else
            genStmt(*function.body);

        if (!currentBlockTerminated()) {
            if (function.returnType.isVoid()) {
                builder_->createRetVoid();
            } else {
                builder_->createRet(builder_->createConstantInt(0));
            }
        }

        popScope();
    }

    return std::move(module_);
}

void CodeGen::pushScope() {
    scopes_.emplace_back();
}

void CodeGen::popScope() {
    scopes_.pop_back();
}

CodeGen::Variable& CodeGen::findVariable(const std::string& name) {
    for (auto scopeIt = scopes_.rbegin(); scopeIt != scopes_.rend(); ++scopeIt) {
        auto variableIt = scopeIt->find(name);
        if (variableIt != scopeIt->end())
            return variableIt->second;
    }
    throw std::runtime_error("Undefined variable: " + name);
}

bool CodeGen::currentBlockTerminated() const {
    auto* block = builder_->getInsertBlock();
    return block && block->getTerminator() != nullptr;
}

void CodeGen::branchToIfOpen(aurora::BasicBlock* target) {
    if (!currentBlockTerminated())
        builder_->createBr(target);
}

void CodeGen::declareVariable(const std::string& name, CType type, const Expr* init) {
    if (type.isVoid())
        throw std::runtime_error("Variable cannot have void type: " + name);
    auto& scope = scopes_.back();
    if (scope.find(name) != scope.end())
        throw std::runtime_error("Duplicate variable in scope: " + name);

    unsigned pointer = builder_->createAlloca(toAirType(type, false));
    scope[name] = Variable{type, pointer};

    if (type.arraySize > 0) {
        if (init)
            throw std::runtime_error("Array initializers are not supported yet: " + name);
        return;
    }

    unsigned initialValue = init ? genExpr(*init) : builder_->createConstantInt(0);
    builder_->createStore(initialValue, pointer);
}

void CodeGen::genStmt(const Stmt& stmt) {
    if (currentBlockTerminated())
        return;
    if (auto* block = dynamic_cast<const BlockStmt*>(&stmt)) {
        genBlock(*block);
    } else if (auto* decl = dynamic_cast<const DeclStmt*>(&stmt)) {
        genDeclStmt(*decl);
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt)) {
        genReturnStmt(*ret);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        genIfStmt(*ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        genWhileStmt(*whileStmt);
    } else if (auto* doWhileStmt = dynamic_cast<const DoWhileStmt*>(&stmt)) {
        genDoWhileStmt(*doWhileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        genForStmt(*forStmt);
    } else if (auto* switchStmt = dynamic_cast<const SwitchStmt*>(&stmt)) {
        genSwitchStmt(*switchStmt);
    } else if (auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        genExprStmt(*exprStmt);
    } else if (dynamic_cast<const BreakStmt*>(&stmt)) {
        genBreakStmt();
    } else if (dynamic_cast<const ContinueStmt*>(&stmt)) {
        genContinueStmt();
    } else {
        throw std::runtime_error("Unknown statement node");
    }
}

void CodeGen::genBlock(const BlockStmt& block, bool createScope) {
    if (createScope)
        pushScope();
    for (const auto& statement : block.statements) {
        if (currentBlockTerminated())
            break;
        genStmt(*statement);
    }
    if (createScope)
        popScope();
}

void CodeGen::genDeclStmt(const DeclStmt& stmt) {
    for (const auto& declarator : stmt.declarators)
        declareVariable(declarator.name, declarator.type, declarator.init.get());
}

void CodeGen::genReturnStmt(const ReturnStmt& stmt) {
    if (currentReturnType_.isVoid()) {
        builder_->createRetVoid();
        return;
    }
    unsigned value = stmt.value ? genExpr(*stmt.value) : builder_->createConstantInt(0);
    builder_->createRet(value);
}

void CodeGen::genIfStmt(const IfStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned ifId = ifCounter_++;
    auto* thenBlock = function->createBasicBlock("if.then." + std::to_string(ifId));
    auto* elseBlock = function->createBasicBlock("if.else." + std::to_string(ifId));
    auto* mergeBlock = function->createBasicBlock("if.end." + std::to_string(ifId));

    builder_->createCondBr(genConditionValue(*stmt.cond), thenBlock, elseBlock);

    builder_->setInsertPoint(thenBlock);
    genStmt(*stmt.thenStmt);
    branchToIfOpen(mergeBlock);

    builder_->setInsertPoint(elseBlock);
    if (stmt.elseStmt)
        genStmt(*stmt.elseStmt);
    branchToIfOpen(mergeBlock);

    builder_->setInsertPoint(mergeBlock);
}

void CodeGen::genWhileStmt(const WhileStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned loopId = loopCounter_++;
    auto* condBlock = function->createBasicBlock("while.cond." + std::to_string(loopId));
    auto* bodyBlock = function->createBasicBlock("while.body." + std::to_string(loopId));
    auto* endBlock = function->createBasicBlock("while.end." + std::to_string(loopId));

    branchToIfOpen(condBlock);
    builder_->setInsertPoint(condBlock);
    builder_->createCondBr(genConditionValue(*stmt.cond), bodyBlock, endBlock);

    builder_->setInsertPoint(bodyBlock);
    breakStack_.push_back(endBlock);
    continueStack_.push_back(condBlock);
    genStmt(*stmt.body);
    continueStack_.pop_back();
    breakStack_.pop_back();
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(endBlock);
}

void CodeGen::genDoWhileStmt(const DoWhileStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned loopId = loopCounter_++;
    auto* bodyBlock = function->createBasicBlock("do.body." + std::to_string(loopId));
    auto* condBlock = function->createBasicBlock("do.cond." + std::to_string(loopId));
    auto* endBlock = function->createBasicBlock("do.end." + std::to_string(loopId));

    branchToIfOpen(bodyBlock);

    builder_->setInsertPoint(bodyBlock);
    breakStack_.push_back(endBlock);
    continueStack_.push_back(condBlock);
    genStmt(*stmt.body);
    continueStack_.pop_back();
    breakStack_.pop_back();
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(condBlock);
    builder_->createCondBr(genConditionValue(*stmt.cond), bodyBlock, endBlock);

    builder_->setInsertPoint(endBlock);
}

void CodeGen::genForStmt(const ForStmt& stmt) {
    pushScope();
    if (stmt.init)
        genStmt(*stmt.init);

    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned loopId = loopCounter_++;
    auto* condBlock = function->createBasicBlock("for.cond." + std::to_string(loopId));
    auto* bodyBlock = function->createBasicBlock("for.body." + std::to_string(loopId));
    auto* stepBlock = function->createBasicBlock("for.step." + std::to_string(loopId));
    auto* endBlock = function->createBasicBlock("for.end." + std::to_string(loopId));

    branchToIfOpen(condBlock);

    builder_->setInsertPoint(condBlock);
    unsigned condValue = stmt.cond ? genConditionValue(*stmt.cond) : builder_->createConstantInt(1);
    builder_->createCondBr(condValue, bodyBlock, endBlock);

    builder_->setInsertPoint(bodyBlock);
    breakStack_.push_back(endBlock);
    continueStack_.push_back(stepBlock);
    genStmt(*stmt.body);
    continueStack_.pop_back();
    breakStack_.pop_back();
    branchToIfOpen(stepBlock);

    builder_->setInsertPoint(stepBlock);
    if (stmt.step)
        (void)genExpr(*stmt.step);
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(endBlock);
    popScope();
}

void CodeGen::genSwitchStmt(const SwitchStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned switchId = switchCounter_++;
    unsigned condValue = genExpr(*stmt.cond);
    auto* endBlock = function->createBasicBlock("switch.end." + std::to_string(switchId));

    if (stmt.sections.empty()) {
        branchToIfOpen(endBlock);
        builder_->setInsertPoint(endBlock);
        return;
    }

    std::vector<aurora::BasicBlock*> bodyBlocks;
    std::vector<size_t> caseSectionIndices;
    std::vector<aurora::BasicBlock*> caseTestBlocks;
    aurora::BasicBlock* defaultBlock = nullptr;

    for (size_t index = 0; index < stmt.sections.size(); ++index) {
        const bool isDefault = stmt.sections[index].value == nullptr;
        std::string name = isDefault ? "switch.default." : "switch.case.";
        name += std::to_string(switchId) + "." + std::to_string(index);
        auto* bodyBlock = function->createBasicBlock(name);
        bodyBlocks.push_back(bodyBlock);
        if (isDefault) {
            defaultBlock = bodyBlock;
        } else {
            caseSectionIndices.push_back(index);
            caseTestBlocks.push_back(function->createBasicBlock("switch.test." + std::to_string(switchId) + "." + std::to_string(index)));
        }
    }

    aurora::BasicBlock* firstDispatch = !caseTestBlocks.empty()
        ? caseTestBlocks.front()
        : (defaultBlock ? defaultBlock : endBlock);
    branchToIfOpen(firstDispatch);

    for (size_t caseIndex = 0; caseIndex < caseSectionIndices.size(); ++caseIndex) {
        const size_t sectionIndex = caseSectionIndices[caseIndex];
        builder_->setInsertPoint(caseTestBlocks[caseIndex]);
        unsigned caseValue = builder_->createConstantInt(evalConstantExpr(*stmt.sections[sectionIndex].value));
        unsigned cmp = builder_->createICmp(aurora::ICmpCond::EQ, condValue, caseValue);
        aurora::BasicBlock* falseTarget = caseIndex + 1 < caseTestBlocks.size()
            ? caseTestBlocks[caseIndex + 1]
            : (defaultBlock ? defaultBlock : endBlock);
        builder_->createCondBr(cmp, bodyBlocks[sectionIndex], falseTarget);
    }

    breakStack_.push_back(endBlock);
    for (size_t index = 0; index < stmt.sections.size(); ++index) {
        builder_->setInsertPoint(bodyBlocks[index]);
        for (const auto& statement : stmt.sections[index].statements) {
            if (currentBlockTerminated())
                break;
            genStmt(*statement);
        }
        aurora::BasicBlock* fallthroughTarget = index + 1 < bodyBlocks.size() ? bodyBlocks[index + 1] : endBlock;
        branchToIfOpen(fallthroughTarget);
    }
    breakStack_.pop_back();

    builder_->setInsertPoint(endBlock);
}

void CodeGen::genExprStmt(const ExprStmt& stmt) {
    if (stmt.expr)
        (void)genExpr(*stmt.expr);
}

void CodeGen::genBreakStmt() {
    if (breakStack_.empty())
        throw std::runtime_error("break used outside of a loop or switch");
    builder_->createBr(breakStack_.back());
}

void CodeGen::genContinueStmt() {
    if (continueStack_.empty())
        throw std::runtime_error("continue used outside of a loop");
    builder_->createBr(continueStack_.back());
}

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
    if (auto* sizeofExpr = dynamic_cast<const SizeofExpr*>(&node))
        return genSizeofExpr(*sizeofExpr);
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

unsigned CodeGen::genIntLitExpr(const IntLitExpr& ie) const {
    return builder_->createConstantInt(ie.value);
}

unsigned CodeGen::genVarExpr(const VarExpr& ve) {
    auto& variable = findVariable(ve.name);
    if (variable.type.arraySize > 0)
        return genAddressOfVariable(ve);
    return builder_->createLoad(toAirType(variable.type, false), variable.pointerVReg);
}

unsigned CodeGen::genAddressOfVariable(const VarExpr& ve) {
    auto& variable = findVariable(ve.name);
    CType addressType = variable.type.arraySize > 0 ? variable.type.decayArray() : variable.type;
    if (variable.type.arraySize == 0)
        ++addressType.pointerDepth;
    aurora::SmallVector<unsigned, 4> indices;
    return builder_->createGEP(toAirType(addressType, false), variable.pointerVReg, indices);
}

CodeGen::LValue CodeGen::genLValue(const Expr& expr) {
    if (auto* variableExpr = dynamic_cast<const VarExpr*>(&expr)) {
        auto& variable = findVariable(variableExpr->name);
        return {variable.type, variable.pointerVReg};
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

    throw std::runtime_error("Expression is not assignable");
}

uint64_t CodeGen::sizeOfType(CType type) const {
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
    throw std::runtime_error("case label must be an integer constant expression");
}

CType CodeGen::inferExprType(const Expr& expr) {
    if (auto* variable = dynamic_cast<const VarExpr*>(&expr))
        return findVariable(variable->name).type.decayArray();
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
        return CType{CTypeKind::Long};
    }
    if (auto* call = dynamic_cast<const CallExpr*>(&expr)) {
        auto it = functionReturnTypes_.find(call->callee);
        if (it != functionReturnTypes_.end())
            return it->second;
    }
    return CType{CTypeKind::Long};
}

unsigned CodeGen::genBinaryExpr(const BinaryExpr& be) {
    auto* intTy = aurora::Type::getInt64Ty();
    CType lhsType = inferExprType(*be.lhs).decayArray();
    CType rhsType = inferExprType(*be.rhs).decayArray();
    const bool lhsPointer = lhsType.isPointerLike();
    const bool rhsPointer = rhsType.isPointerLike();

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
    case BinaryExpr::Div: return builder_->createSDiv(intTy, lhs, rhs);
    case BinaryExpr::Rem: {
        unsigned quotient = builder_->createSDiv(intTy, lhs, rhs);
        unsigned product = builder_->createMul(intTy, quotient, rhs);
        return builder_->createSub(intTy, lhs, product);
    }
    case BinaryExpr::Eq: return builder_->createICmp(aurora::ICmpCond::EQ, lhs, rhs);
    case BinaryExpr::Ne: return builder_->createICmp(aurora::ICmpCond::NE, lhs, rhs);
    case BinaryExpr::Lt: return builder_->createICmp(aurora::ICmpCond::SLT, lhs, rhs);
    case BinaryExpr::Le: return builder_->createICmp(aurora::ICmpCond::SLE, lhs, rhs);
    case BinaryExpr::Gt: return builder_->createICmp(aurora::ICmpCond::SGT, lhs, rhs);
    case BinaryExpr::Ge: return builder_->createICmp(aurora::ICmpCond::SGE, lhs, rhs);
    case BinaryExpr::BitAnd: return builder_->createAnd(intTy, lhs, rhs);
    case BinaryExpr::BitOr: return builder_->createOr(intTy, lhs, rhs);
    case BinaryExpr::BitXor: return builder_->createXor(intTy, lhs, rhs);
    case BinaryExpr::Shl: return builder_->createShl(intTy, lhs, rhs);
    case BinaryExpr::Shr: return builder_->createAShr(intTy, lhs, rhs);
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
        case AssignExpr::DivAssign: result = builder_->createSDiv(intTy, lhs, rhs); break;
        case AssignExpr::RemAssign: {
            unsigned quotient = builder_->createSDiv(intTy, lhs, rhs);
            unsigned product = builder_->createMul(intTy, quotient, rhs);
            result = builder_->createSub(intTy, lhs, product);
            break;
        }
        case AssignExpr::BitAndAssign: result = builder_->createAnd(intTy, lhs, rhs); break;
        case AssignExpr::BitOrAssign: result = builder_->createOr(intTy, lhs, rhs); break;
        case AssignExpr::BitXorAssign: result = builder_->createXor(intTy, lhs, rhs); break;
        case AssignExpr::ShlAssign: result = builder_->createShl(intTy, lhs, rhs); break;
        case AssignExpr::ShrAssign: result = builder_->createAShr(intTy, lhs, rhs); break;
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

unsigned CodeGen::genSizeofExpr(const SizeofExpr& se) {
    CType type = se.type;
    if (se.expr) {
        if (auto* variable = dynamic_cast<const VarExpr*>(se.expr.get()))
            type = findVariable(variable->name).type;
        else
            type = inferExprType(*se.expr);
    }
    return builder_->createConstantInt(static_cast<int64_t>(sizeOfType(type)));
}

unsigned CodeGen::genCommaExpr(const CommaExpr& ce) {
    (void)genExpr(*ce.lhs);
    return genExpr(*ce.rhs);
}

unsigned CodeGen::genCallExpr(const CallExpr& ce) {
    auto functionIt = functionMap_.find(ce.callee);
    if (functionIt == functionMap_.end())
        throw std::runtime_error("Unknown function: " + ce.callee);

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
