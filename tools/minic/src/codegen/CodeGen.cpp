#include "minic/codegen/CodeGen.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Constant.h"
#include "Aurora/Air/Function.h"
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace minic {

namespace {

bool sameType(CType lhs, CType rhs) {
    lhs = lhs.decayArray();
    rhs = rhs.decayArray();
    if (lhs.kind != rhs.kind || lhs.pointerDepth != rhs.pointerDepth ||
        lhs.arraySize != rhs.arraySize || lhs.isUnsigned != rhs.isUnsigned)
        return false;
    if (lhs.kind == CTypeKind::Struct || lhs.kind == CTypeKind::Union)
        return lhs.structInfo == rhs.structInfo;
    return true;
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

size_t findFieldIndex(const CStructInfo& info, const std::string& fieldName) {
    for (size_t index = 0; index < info.fields.size(); ++index) {
        if (info.fields[index].name == fieldName)
            return index;
    }
    return info.fields.size();
}

bool isRecordObject(CType type) noexcept {
    return (type.kind == CTypeKind::Struct || type.kind == CTypeKind::Union) && type.pointerDepth == 0;
}

void storeGlobalRecordValue(std::vector<uint8_t>& bytes, uint64_t offset, int64_t value) {
    for (uint64_t byteIndex = 0; byteIndex < 8 && offset + byteIndex < bytes.size(); ++byteIndex) {
        bytes[static_cast<size_t>(offset + byteIndex)] = static_cast<uint8_t>(
            (static_cast<uint64_t>(value) >> (byteIndex * 8)) & 0xff);
    }
}

void zeroGlobalBytes(std::vector<uint8_t>& bytes, uint64_t offset, uint64_t size) {
    for (uint64_t byteIndex = 0; byteIndex < size && offset + byteIndex < bytes.size(); ++byteIndex)
        bytes[static_cast<size_t>(offset + byteIndex)] = 0;
}

int64_t readGlobalRecordValue(const std::vector<uint8_t>& bytes, uint64_t offset) {
    uint64_t value = 0;
    for (uint64_t byteIndex = 0; byteIndex < 8 && offset + byteIndex < bytes.size(); ++byteIndex)
        value |= static_cast<uint64_t>(bytes[static_cast<size_t>(offset + byteIndex)]) << (byteIndex * 8);
    return static_cast<int64_t>(value);
}

std::vector<aurora::Constant*> recordBytesToConstants(const std::vector<uint8_t>& bytes, uint64_t offset, uint64_t slots) {
    std::vector<aurora::Constant*> elements;
    elements.reserve(static_cast<size_t>(slots));
    for (uint64_t slot = 0; slot < slots; ++slot) {
        elements.push_back(aurora::ConstantInt::getInt64(readGlobalRecordValue(bytes, offset + slot * 8)));
    }
    return elements;
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
        aurora::Type* airType = ((elementType.kind == CTypeKind::Struct || elementType.kind == CTypeKind::Union) &&
                                 (!elementType.structInfo || !elementType.structInfo->complete))
            ? aurora::Type::getInt8Ty()
            : toAirType(elementType, true);
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
    case CTypeKind::Struct:
    case CTypeKind::Union: {
        uint64_t size = sizeOfType(type);
        if (size == 0)
            throw std::runtime_error("Incomplete record type cannot be used as an object");
        uint64_t slots = (size + 7) / 8;
        return aurora::Type::getArrayTy(aurora::Type::getInt64Ty(), static_cast<unsigned>(slots == 0 ? 1 : slots));
    }
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

aurora::Constant* CodeGen::buildGlobalInitializer(CType type, const Expr& init, const std::string& name) {
    uint64_t byteSize = sizeOfType(type);
    if (byteSize == 0)
        byteSize = 1;
    std::vector<uint8_t> bytes(static_cast<size_t>(byteSize), 0);
    storeGlobalInitializerBytes(type, bytes, 0, init, name);
    return buildGlobalConstantFromBytes(type, bytes, 0);
}

aurora::Constant* CodeGen::buildGlobalConstantFromBytes(CType type, const std::vector<uint8_t>& bytes, uint64_t offset) {
    if (type.arraySize > 0) {
        CType elementType = type;
        elementType.arraySize = 0;
        std::vector<aurora::Constant*> elements;
        elements.reserve(static_cast<size_t>(type.arraySize));
        for (uint64_t index = 0; index < type.arraySize; ++index)
            elements.push_back(buildGlobalConstantFromBytes(elementType, bytes, offset + index * sizeOfType(elementType)));
        return aurora::ConstantArray::get(toAirType(type, false), std::move(elements));
    }
    if (isRecordObject(type)) {
        if (!type.structInfo || !type.structInfo->complete)
            throw std::runtime_error("Global record initializer requires a complete record type");
        const uint64_t slots = (sizeOfType(type) + 7) / 8;
        return aurora::ConstantArray::get(
            toAirType(type, false),
            recordBytesToConstants(bytes, offset, slots == 0 ? 1 : slots));
    }
    return aurora::ConstantInt::getInt64(readGlobalRecordValue(bytes, offset));
}

void CodeGen::storeGlobalInitializerBytes(CType type, std::vector<uint8_t>& bytes, uint64_t offset, const Expr& init, const std::string& name) {
    if (type.arraySize > 0) {
        auto* initList = dynamic_cast<const InitListExpr*>(&init);
        if (!initList)
            throw std::runtime_error("Global array initializer must be a braced list: " + name);
        zeroGlobalBytes(bytes, offset, sizeOfType(type));
        CType elementType = type;
        elementType.arraySize = 0;
        uint64_t nextIndex = 0;
        for (const auto& entry : initList->entries) {
            uint64_t targetIndex = nextIndex;
            if (!entry.designator.empty()) {
                if (entry.designator.first().kind == InitListExpr::Designator::Field)
                    throw std::runtime_error("Global array initializer cannot use a field designator: " + name);
                targetIndex = entry.designator.first().index;
            }
            if (targetIndex >= type.arraySize)
                throw std::runtime_error("Too many values in global array initializer: " + name);
            if (entry.designator.empty()) {
                storeGlobalInitializerBytes(
                    elementType,
                    bytes,
                    offset + targetIndex * sizeOfType(elementType),
                    *entry.value,
                    name);
            } else {
                storeGlobalDesignatedInitializerBytes(type, bytes, offset, entry.designator, 0, *entry.value, name);
            }
            nextIndex = targetIndex + 1;
        }
        return;
    }

    if (!isRecordObject(type)) {
        if (auto* initList = dynamic_cast<const InitListExpr*>(&init)) {
            if (initList->entries.size() > 1)
                throw std::runtime_error("Global scalar initializer has too many values: " + name);
            if (initList->entries.empty())
                return;
            if (!initList->entries.front().designator.empty())
                throw std::runtime_error("Global scalar initializer cannot use a designator: " + name);
            storeGlobalInitializerBytes(type, bytes, offset, *initList->entries.front().value, name);
            return;
        }
        int64_t value = 0;
        try {
            value = evalConstantExpr(init);
        } catch (const std::exception&) {
            throw std::runtime_error("Global initializer must be an integer constant expression: " + name);
        }
        storeGlobalRecordValue(bytes, offset, value);
        return;
    }

    auto* initList = dynamic_cast<const InitListExpr*>(&init);
    if (!initList)
        throw std::runtime_error("Global record initializer must be a braced list: " + name);
    if (!type.structInfo || !type.structInfo->complete)
        throw std::runtime_error("Global record initializer requires a complete record type: " + name);
    zeroGlobalBytes(bytes, offset, sizeOfType(type));

    if (type.kind == CTypeKind::Union) {
        if (initList->entries.size() > 1)
            throw std::runtime_error("Too many values in global union initializer: " + name);
        if (!initList->entries.empty()) {
            const auto& entry = initList->entries.front();
            if (!entry.designator.empty()) {
                if (entry.designator.first().kind == InitListExpr::Designator::Index)
                    throw std::runtime_error("Global union initializer cannot use an array designator: " + name);
                storeGlobalDesignatedInitializerBytes(type, bytes, offset, entry.designator, 0, *entry.value, name);
            } else if (!type.structInfo->fields.empty()) {
                storeGlobalInitializerBytes(
                    type.structInfo->fields[0].type,
                    bytes,
                    offset + type.structInfo->fields[0].offset,
                    *entry.value,
                    name);
            }
        }
    } else {
        size_t nextField = 0;
        for (const auto& entry : initList->entries) {
            if (!entry.designator.empty()) {
                if (entry.designator.first().kind == InitListExpr::Designator::Index)
                    throw std::runtime_error("Global struct initializer cannot use an array designator: " + name);
                size_t fieldIndex = findFieldIndex(*type.structInfo, entry.designator.first().field);
                if (fieldIndex == type.structInfo->fields.size())
                    throw std::runtime_error("Unknown global struct initializer field: " + entry.designator.first().field);
                storeGlobalDesignatedInitializerBytes(type, bytes, offset, entry.designator, 0, *entry.value, name);
                nextField = fieldIndex + 1;
                continue;
            }
            size_t fieldIndex = nextField;
            if (fieldIndex >= type.structInfo->fields.size())
                throw std::runtime_error("Too many values in global record initializer: " + name);
            storeGlobalInitializerBytes(
                type.structInfo->fields[fieldIndex].type,
                bytes,
                offset + type.structInfo->fields[fieldIndex].offset,
                *entry.value,
                name);
            nextField = fieldIndex + 1;
        }
    }
}

void CodeGen::storeGlobalDesignatedInitializerBytes(
    CType type,
    std::vector<uint8_t>& bytes,
    uint64_t offset,
    const InitListExpr::Designator& designator,
    size_t partIndex,
    const Expr& init,
    const std::string& name) {
    if (partIndex >= designator.parts.size()) {
        storeGlobalInitializerBytes(type, bytes, offset, init, name);
        return;
    }

    const auto& part = designator.parts[partIndex];
    if (type.arraySize > 0) {
        if (part.kind != InitListExpr::Designator::Index)
            throw std::runtime_error("Global array initializer cannot use a field designator: " + name);
        if (part.index >= type.arraySize)
            throw std::runtime_error("Too many values in global array initializer: " + name);
        CType elementType = type;
        elementType.arraySize = 0;
        storeGlobalDesignatedInitializerBytes(
            elementType,
            bytes,
            offset + part.index * sizeOfType(elementType),
            designator,
            partIndex + 1,
            init,
            name);
        return;
    }

    if (isRecordObject(type)) {
        if (part.kind != InitListExpr::Designator::Field)
            throw std::runtime_error("Global record initializer cannot use an array designator: " + name);
        if (!type.structInfo || !type.structInfo->complete)
            throw std::runtime_error("Global record initializer requires a complete record type: " + name);
        size_t fieldIndex = findFieldIndex(*type.structInfo, part.field);
        if (fieldIndex == type.structInfo->fields.size())
            throw std::runtime_error("Unknown global record initializer field: " + part.field);
        const CField& field = type.structInfo->fields[fieldIndex];
        storeGlobalDesignatedInitializerBytes(
            field.type,
            bytes,
            offset + field.offset,
            designator,
            partIndex + 1,
            init,
            name);
        return;
    }

    throw std::runtime_error("Global scalar initializer cannot use a nested designator: " + name);
}

void CodeGen::declareGlobal(const GlobalDecl& decl) {
    if (decl.type.isVoid())
        throw std::runtime_error("Global variable cannot have void type: " + decl.name);
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

    if (decl.type.arraySize > 0) {
        if (decl.init) {
            if (!globalIt->second.variable)
                throw std::runtime_error("Extern global declaration cannot have an initializer: " + decl.name);
            auto* initList = dynamic_cast<const InitListExpr*>(decl.init.get());
            if (!initList)
                throw std::runtime_error("Global array initializer must be a braced list: " + decl.name);
            globalIt->second.variable->setInitializer(buildGlobalInitializer(decl.type, *initList, decl.name));
        }
        return;
    }

    if ((decl.type.kind == CTypeKind::Struct || decl.type.kind == CTypeKind::Union) && decl.type.pointerDepth == 0) {
        if (decl.init) {
            if (!globalIt->second.variable)
                throw std::runtime_error("Extern global declaration cannot have an initializer: " + decl.name);
            auto* initList = dynamic_cast<const InitListExpr*>(decl.init.get());
            if (!initList)
                throw std::runtime_error("Global record initializer must be a braced list: " + decl.name);
            globalIt->second.variable->setInitializer(buildGlobalInitializer(decl.type, *initList, decl.name));
        }
        return;
    }

    if (decl.init) {
        if (!globalIt->second.variable)
            throw std::runtime_error("Extern global declaration cannot have an initializer: " + decl.name);
        globalIt->second.variable->setInitializer(buildGlobalInitializer(decl.type, *decl.init, decl.name));
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
    CType addressType = global.type.arraySize > 0 ? global.type.decayArray() : global.type;
    if (global.type.arraySize == 0)
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
