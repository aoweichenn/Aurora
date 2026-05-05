#include "minic/codegen/CodeGen.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/Air/Function.h"
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace minic {

namespace {

bool sameType(CType lhs, CType rhs) {
    lhs = lhs.decayArray();
    rhs = rhs.decayArray();
    return lhs.kind == rhs.kind && lhs.pointerDepth == rhs.pointerDepth &&
           lhs.arraySize == rhs.arraySize && lhs.isUnsigned == rhs.isUnsigned;
}

bool sameSignature(const Function& lhs, const Function& rhs) {
    if (!sameType(lhs.returnType, rhs.returnType) || lhs.params.size() != rhs.params.size())
        return false;
    for (size_t index = 0; index < lhs.params.size(); ++index) {
        if (!sameType(lhs.params[index].type, rhs.params[index].type))
            return false;
    }
    return true;
}

} // namespace

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

std::unique_ptr<aurora::Module> CodeGen::generate(const Program& program) {
    module_ = std::make_unique<aurora::Module>("minic");
    functionMap_.clear();
    functionReturnTypes_.clear();
    globals_.clear();
    std::unordered_map<std::string, const Function*> functionSignatures;
    std::unordered_set<std::string> definedFunctions;

    for (const auto& function : program.functions) {
        auto signatureIt = functionSignatures.find(function.name);
        if (signatureIt != functionSignatures.end()) {
            if (!sameSignature(*signatureIt->second, function))
                throw std::runtime_error("Conflicting function declaration: " + function.name);
            if (function.body) {
                if (!definedFunctions.insert(function.name).second)
                    throw std::runtime_error("Duplicate function definition: " + function.name);
                functionMap_.at(function.name)->setDeclaration(false);
            }
            continue;
        }

        aurora::SmallVector<aurora::Type*, 8> paramTypes;
        for (const auto& param : function.params)
            paramTypes.push_back(toAirType(param.type.decayArray(), false));

        auto* functionType = new aurora::FunctionType(toAirType(function.returnType), paramTypes);
        auto* airFunction = module_->createFunction(functionType, function.name);
        airFunction->setDeclaration(function.body == nullptr);
        functionMap_[function.name] = airFunction;
        functionReturnTypes_[function.name] = function.returnType;
        functionSignatures[function.name] = &function;
        if (function.body)
            definedFunctions.insert(function.name);
    }

    for (const auto& global : program.globals)
        declareGlobal(global);

    for (const auto& function : program.functions) {
        if (!function.body)
            continue;
        auto* airFunction = functionMap_.at(function.name);
        airFunction->setDeclaration(false);
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
            if (param.name.empty())
                continue;
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

void CodeGen::declareGlobal(const GlobalDecl& decl) {
    if (decl.type.isVoid())
        throw std::runtime_error("Global variable cannot have void type: " + decl.name);
    if (decl.type.arraySize > 0)
        throw std::runtime_error("Global arrays are not supported yet: " + decl.name);
    if (functionMap_.find(decl.name) != functionMap_.end())
        throw std::runtime_error("Global variable conflicts with function: " + decl.name);

    auto globalIt = globals_.find(decl.name);
    if (globalIt != globals_.end()) {
        if (!sameType(globalIt->second.type, decl.type))
            throw std::runtime_error("Conflicting global declaration: " + decl.name);
        if (!decl.isExtern) {
            if (!globalIt->second.isExtern)
                throw std::runtime_error("Duplicate global definition: " + decl.name);
            globalIt->second.variable = module_->createGlobal(toAirType(decl.type, false), decl.name);
            globalIt->second.isExtern = false;
        }
    } else {
        aurora::GlobalVariable* variable = nullptr;
        if (!decl.isExtern)
            variable = module_->createGlobal(toAirType(decl.type, false), decl.name);
        globals_.emplace(decl.name, Global{decl.type, variable, decl.name, decl.isExtern});
        globalIt = globals_.find(decl.name);
    }

    if (decl.init) {
        if (!globalIt->second.variable)
            throw std::runtime_error("Extern global declaration cannot have an initializer: " + decl.name);
        int64_t value = 0;
        try {
            value = evalConstantExpr(*decl.init);
        } catch (const std::exception&) {
            throw std::runtime_error("Global initializer must be an integer constant expression: " + decl.name);
        }
        globalIt->second.variable->setInitializer(aurora::ConstantInt::getInt64(value));
    }
}

void CodeGen::pushScope() {
    scopes_.emplace_back();
}

void CodeGen::popScope() {
    scopes_.pop_back();
}

CodeGen::Variable& CodeGen::findVariable(const std::string& name) {
    if (auto* variable = findVariableInScopes(name))
        return *variable;
    throw std::runtime_error("Undefined variable: " + name);
}

CodeGen::Variable* CodeGen::findVariableInScopes(const std::string& name) {
    for (auto scopeIt = scopes_.rbegin(); scopeIt != scopes_.rend(); ++scopeIt) {
        auto variableIt = scopeIt->find(name);
        if (variableIt != scopeIt->end())
            return &variableIt->second;
    }
    return nullptr;
}

CodeGen::Global& CodeGen::findGlobal(const std::string& name) {
    auto globalIt = globals_.find(name);
    if (globalIt != globals_.end())
        return globalIt->second;
    throw std::runtime_error("Undefined variable: " + name);
}

unsigned CodeGen::genGlobalAddress(const std::string& name) {
    auto& global = findGlobal(name);
    CType addressType = global.type;
    ++addressType.pointerDepth;
    return builder_->createGlobalAddress(toAirType(addressType, false), global.name.c_str());
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
