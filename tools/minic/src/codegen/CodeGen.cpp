#include "minic/codegen/CodeGen.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <stdexcept>
#include <utility>

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
            paramTypes.push_back(toAirType(param.type.decayArray(), false));

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
            CType paramType = param.type.decayArray();
            if (paramType.isVoid())
                throw std::runtime_error("Parameter cannot have void type: " + param.name);
            unsigned pointer = builder_->createAlloca(toAirType(paramType, false));
            builder_->createStore(static_cast<unsigned>(paramIndex), pointer);
            scopes_.back()[param.name] = Variable{paramType, pointer};
        }

        if (auto* block = dynamic_cast<const BlockStmt*>(function.body.get()))
            genBlock(*block, false);
        else
            genStmt(*function.body);

        if (!currentBlockTerminated()) {
            if (function.returnType.isVoid())
                builder_->createRetVoid();
            else
                builder_->createRet(builder_->createConstantInt(0));
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

} // namespace minic
