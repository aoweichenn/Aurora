#include "CodeGen.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <stdexcept>

namespace minic {

CodeGen::CodeGen() : builder_(nullptr) {}

aurora::Type* CodeGen::toAirType(CType type, bool allowVoid) const {
    switch (type.kind) {
    case CTypeKind::Void:
        if (!allowVoid)
            throw std::runtime_error("void is not a scalar value type in MiniC");
        return aurora::Type::getVoidTy();
    case CTypeKind::Char:
    case CTypeKind::Int:
    case CTypeKind::Long:
        return aurora::Type::getInt64Ty();
    }
    return aurora::Type::getInt64Ty();
}

std::unique_ptr<aurora::Module> CodeGen::generate(const std::vector<Function>& functions) {
    module_ = std::make_unique<aurora::Module>("minic");
    functionMap_.clear();

    for (const auto& function : functions) {
        if (functionMap_.find(function.name) != functionMap_.end())
            throw std::runtime_error("Duplicate function: " + function.name);

        aurora::SmallVector<aurora::Type*, 8> paramTypes;
        for (const auto& param : function.params)
            paramTypes.push_back(toAirType(param.type, false));

        auto* functionType = new aurora::FunctionType(toAirType(function.returnType), paramTypes);
        functionMap_[function.name] = module_->createFunction(functionType, function.name);
    }

    for (const auto& function : functions) {
        auto* airFunction = functionMap_.at(function.name);
        aurora::AIRBuilder builder(airFunction->getEntryBlock());
        builder_ = &builder;
        currentReturnType_ = function.returnType;
        ifCounter_ = 0;
        loopCounter_ = 0;
        conditionalCounter_ = 0;
        loopStack_.clear();
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
    unsigned initialValue = init ? genExpr(*init) : builder_->createConstantInt(0);
    builder_->createStore(initialValue, pointer);
    scope[name] = Variable{type, pointer};
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
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        genForStmt(*forStmt);
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
        declareVariable(declarator.name, stmt.type, declarator.init.get());
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
    loopStack_.push_back({endBlock, condBlock});
    genStmt(*stmt.body);
    loopStack_.pop_back();
    branchToIfOpen(condBlock);

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
    loopStack_.push_back({endBlock, stepBlock});
    genStmt(*stmt.body);
    loopStack_.pop_back();
    branchToIfOpen(stepBlock);

    builder_->setInsertPoint(stepBlock);
    if (stmt.step)
        (void)genExpr(*stmt.step);
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(endBlock);
    popScope();
}

void CodeGen::genExprStmt(const ExprStmt& stmt) {
    if (stmt.expr)
        (void)genExpr(*stmt.expr);
}

void CodeGen::genBreakStmt() {
    if (loopStack_.empty())
        throw std::runtime_error("break used outside of a loop");
    builder_->createBr(loopStack_.back().breakTarget);
}

void CodeGen::genContinueStmt() {
    if (loopStack_.empty())
        throw std::runtime_error("continue used outside of a loop");
    builder_->createBr(loopStack_.back().continueTarget);
}

unsigned CodeGen::genExpr(const Expr& node) {
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&node))
        return genBinaryExpr(*binary);
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&node))
        return genUnaryExpr(*unary);
    if (auto* assign = dynamic_cast<const AssignExpr*>(&node))
        return genAssignExpr(*assign);
    if (auto* incDec = dynamic_cast<const IncDecExpr*>(&node))
        return genIncDecExpr(*incDec);
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
    if (auto* assign = dynamic_cast<const AssignExpr*>(&expr))
        return containsCall(*assign->value);
    if (auto* conditional = dynamic_cast<const ConditionalExpr*>(&expr))
        return containsCall(*conditional->cond) || containsCall(*conditional->trueExpr) || containsCall(*conditional->falseExpr);
    return false;
}

unsigned CodeGen::genIntLitExpr(const IntLitExpr& ie) const {
    return builder_->createConstantInt(ie.value);
}

unsigned CodeGen::genVarExpr(const VarExpr& ve) {
    auto& variable = findVariable(ve.name);
    return builder_->createLoad(toAirType(variable.type, false), variable.pointerVReg);
}

unsigned CodeGen::genBinaryExpr(const BinaryExpr& be) {
    auto* intTy = aurora::Type::getInt64Ty();

    if (be.op == BinaryExpr::LogAnd || be.op == BinaryExpr::LogOr) {
        unsigned lhs = genConditionValue(*be.lhs);
        unsigned rhs = genConditionValue(*be.rhs);
        return be.op == BinaryExpr::LogAnd
            ? builder_->createAnd(aurora::Type::getInt1Ty(), lhs, rhs)
            : builder_->createOr(aurora::Type::getInt1Ty(), lhs, rhs);
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
    case BinaryExpr::Add: return builder_->createAdd(intTy, lhs, rhs);
    case BinaryExpr::Sub: return builder_->createSub(intTy, lhs, rhs);
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
    unsigned value = genExpr(*ue.operand);
    switch (ue.op) {
    case UnaryExpr::Plus:
        return value;
    case UnaryExpr::Neg:
        return builder_->createSub(intTy, builder_->createConstantInt(0), value);
    case UnaryExpr::LogicalNot:
        return builder_->createICmp(aurora::ICmpCond::EQ, value, builder_->createConstantInt(0));
    case UnaryExpr::BitNot:
        return builder_->createXor(intTy, value, builder_->createConstantInt(-1));
    }
    throw std::runtime_error("Unknown unary operator");
}

unsigned CodeGen::genAssignExpr(const AssignExpr& ae) {
    auto& variable = findVariable(ae.name);
    auto* intTy = aurora::Type::getInt64Ty();
    unsigned rhs = genExpr(*ae.value);
    unsigned result = rhs;

    if (ae.op != AssignExpr::Assign) {
        unsigned lhs = builder_->createLoad(toAirType(variable.type, false), variable.pointerVReg);
        switch (ae.op) {
        case AssignExpr::AddAssign: result = builder_->createAdd(intTy, lhs, rhs); break;
        case AssignExpr::SubAssign: result = builder_->createSub(intTy, lhs, rhs); break;
        case AssignExpr::MulAssign: result = builder_->createMul(intTy, lhs, rhs); break;
        case AssignExpr::DivAssign: result = builder_->createSDiv(intTy, lhs, rhs); break;
        case AssignExpr::RemAssign: {
            unsigned quotient = builder_->createSDiv(intTy, lhs, rhs);
            unsigned product = builder_->createMul(intTy, quotient, rhs);
            result = builder_->createSub(intTy, lhs, product);
            break;
        }
        case AssignExpr::Assign:
            break;
        }
    }

    builder_->createStore(result, variable.pointerVReg);
    return result;
}

unsigned CodeGen::genIncDecExpr(const IncDecExpr& ie) {
    auto& variable = findVariable(ie.name);
    auto* intTy = aurora::Type::getInt64Ty();
    unsigned oldValue = builder_->createLoad(toAirType(variable.type, false), variable.pointerVReg);
    unsigned one = builder_->createConstantInt(1);
    unsigned newValue = ie.increment
        ? builder_->createAdd(intTy, oldValue, one)
        : builder_->createSub(intTy, oldValue, one);
    builder_->createStore(newValue, variable.pointerVReg);
    return ie.prefix ? newValue : oldValue;
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
