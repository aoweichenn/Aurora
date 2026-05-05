#include "minic/codegen/CodeGen.h"
#include <stdexcept>

namespace minic {

namespace {

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

} // namespace

void CodeGen::declareVariable(const std::string& name, CType type, const Expr* init) {
    if (type.isVoid())
        throw std::runtime_error("Variable cannot have void type: " + name);
    auto& scope = scopes_.back();
    if (scope.find(name) != scope.end())
        throw std::runtime_error("Duplicate variable in scope: " + name);

    unsigned pointer = builder_->createAlloca(toAirType(type, false));
    scope[name] = Variable{type, pointer};

    if (type.arraySize > 0) {
        if (init) {
            auto* initList = dynamic_cast<const InitListExpr*>(init);
            if (!initList)
                throw std::runtime_error("Array initializer must be a braced list: " + name);
            genArrayInitializer(type, pointer, *initList, name);
        }
        return;
    }

    if ((type.kind == CTypeKind::Struct || type.kind == CTypeKind::Union) && type.pointerDepth == 0) {
        if (init) {
            auto* initList = dynamic_cast<const InitListExpr*>(init);
            if (!initList)
                throw std::runtime_error("Record initializer must be a braced list: " + name);
            genStructInitializer(type, pointer, *initList, name);
        }
        return;
    }

    unsigned initialValue = builder_->createConstantInt(0);
    if (init) {
        if (auto* initList = dynamic_cast<const InitListExpr*>(init)) {
            if (initList->entries.size() > 1)
                throw std::runtime_error("Scalar initializer has too many values: " + name);
            if (!initList->entries.empty()) {
                if (!initList->entries.front().designator.empty())
                    throw std::runtime_error("Scalar initializer cannot use a designator: " + name);
                initialValue = genExpr(*initList->entries.front().value);
            }
        } else {
            initialValue = genExpr(*init);
        }
    }
    builder_->createStore(initialValue, pointer);
}

void CodeGen::genStructInitializer(CType type, unsigned pointer, const InitListExpr& init, const std::string& name) {
    aurora::SmallVector<unsigned, 4> indices;
    unsigned base = builder_->createGEP(toAirType(type, false), pointer, indices);
    genRecordInitializerAtAddress(type, base, init, name);
}

void CodeGen::genRecordInitializerAtAddress(CType type, unsigned base, const InitListExpr& init, const std::string& name) {
    if ((type.kind != CTypeKind::Struct && type.kind != CTypeKind::Union) || !type.structInfo || !type.structInfo->complete)
        throw std::runtime_error("Record initializer requires a complete record type: " + name);
    if (type.kind == CTypeKind::Union && init.entries.size() > 1)
        throw std::runtime_error("Too many values in union initializer: " + name);

    auto fieldAddress = [&](size_t index) {
        return addByteOffset(base, type.structInfo->fields[index].offset);
    };

    genZeroInitializerAtAddress(type, base);

    if (type.kind == CTypeKind::Union) {
        if (type.structInfo->fields.empty())
            return;
        size_t fieldIndex = 0;
        if (!init.entries.empty()) {
            const auto& entry = init.entries.front();
            if (!entry.designator.empty()) {
                if (entry.designator.first().kind == InitListExpr::Designator::Index)
                    throw std::runtime_error("Union initializer cannot use an array designator: " + name);
                genDesignatedInitializerAtAddress(type, base, entry.designator, 0, *entry.value, name);
            } else {
                genInitializerAtAddress(type.structInfo->fields[fieldIndex].type, fieldAddress(fieldIndex), *entry.value, name);
            }
        }
        return;
    }

    size_t nextField = 0;
    for (const auto& entry : init.entries) {
        if (!entry.designator.empty()) {
            if (entry.designator.first().kind == InitListExpr::Designator::Index)
                throw std::runtime_error("Struct initializer cannot use an array designator: " + name);
            size_t fieldIndex = findFieldIndex(*type.structInfo, entry.designator.first().field);
            if (fieldIndex == type.structInfo->fields.size())
                throw std::runtime_error("Unknown struct initializer field: " + entry.designator.first().field);
            genDesignatedInitializerAtAddress(type, base, entry.designator, 0, *entry.value, name);
            nextField = fieldIndex + 1;
            continue;
        }

        size_t fieldIndex = nextField;
        if (fieldIndex >= type.structInfo->fields.size())
            throw std::runtime_error("Too many values in record initializer: " + name);
        genInitializerAtAddress(type.structInfo->fields[fieldIndex].type, fieldAddress(fieldIndex), *entry.value, name);
        nextField = fieldIndex + 1;
    }
}

void CodeGen::genArrayInitializer(CType type, unsigned pointer, const InitListExpr& init, const std::string& name) {
    aurora::SmallVector<unsigned, 4> indices;
    unsigned base = builder_->createGEP(toAirType(type.decayArray(), false), pointer, indices);
    genArrayInitializerAtAddress(type, base, init, name);
}

void CodeGen::genArrayInitializerAtAddress(CType type, unsigned base, const InitListExpr& init, const std::string& name) {
    CType elementType = type;
    elementType.arraySize = 0;

    auto elementAddress = [&](uint64_t index) {
        const uint64_t offset = index * sizeOfType(elementType);
        return addByteOffset(base, offset);
    };

    genZeroInitializerAtAddress(type, base);

    uint64_t nextIndex = 0;
    for (const auto& entry : init.entries) {
        uint64_t targetIndex = nextIndex;
        if (!entry.designator.empty()) {
            if (entry.designator.first().kind == InitListExpr::Designator::Field)
                throw std::runtime_error("Array initializer cannot use a field designator: " + name);
            targetIndex = entry.designator.first().index;
        }
        if (targetIndex >= type.arraySize)
            throw std::runtime_error("Too many values in array initializer: " + name);
        if (entry.designator.empty())
            genInitializerAtAddress(elementType, elementAddress(targetIndex), *entry.value, name);
        else
            genDesignatedInitializerAtAddress(type, base, entry.designator, 0, *entry.value, name);
        nextIndex = targetIndex + 1;
    }
}

void CodeGen::genInitializerAtAddress(CType type, unsigned address, const Expr& init, const std::string& name) {
    if (type.arraySize > 0) {
        auto* initList = dynamic_cast<const InitListExpr*>(&init);
        if (!initList)
            throw std::runtime_error("Array initializer must be a braced list: " + name);
        genArrayInitializerAtAddress(type, address, *initList, name);
        return;
    }
    if (isRecordObject(type)) {
        auto* initList = dynamic_cast<const InitListExpr*>(&init);
        if (!initList)
            throw std::runtime_error("Record initializer must be a braced list: " + name);
        genRecordInitializerAtAddress(type, address, *initList, name);
        return;
    }
    if (auto* initList = dynamic_cast<const InitListExpr*>(&init)) {
        if (initList->entries.size() > 1)
            throw std::runtime_error("Scalar initializer has too many values: " + name);
        if (initList->entries.empty()) {
            builder_->createStore(builder_->createConstantInt(0), address);
            return;
        }
        if (!initList->entries.front().designator.empty())
            throw std::runtime_error("Scalar initializer cannot use a designator: " + name);
        genInitializerAtAddress(type, address, *initList->entries.front().value, name);
        return;
    }
    builder_->createStore(genExpr(init), address);
}

void CodeGen::genDesignatedInitializerAtAddress(
    CType type,
    unsigned address,
    const InitListExpr::Designator& designator,
    size_t partIndex,
    const Expr& init,
    const std::string& name) {
    if (partIndex >= designator.parts.size()) {
        genInitializerAtAddress(type, address, init, name);
        return;
    }

    const auto& part = designator.parts[partIndex];
    if (type.arraySize > 0) {
        if (part.kind != InitListExpr::Designator::Index)
            throw std::runtime_error("Array initializer cannot use a field designator: " + name);
        if (part.index >= type.arraySize)
            throw std::runtime_error("Too many values in array initializer: " + name);
        CType elementType = type;
        elementType.arraySize = 0;
        unsigned elementAddress = addByteOffset(address, part.index * sizeOfType(elementType));
        genDesignatedInitializerAtAddress(elementType, elementAddress, designator, partIndex + 1, init, name);
        return;
    }

    if (isRecordObject(type)) {
        if (part.kind != InitListExpr::Designator::Field)
            throw std::runtime_error("Record initializer cannot use an array designator: " + name);
        if (!type.structInfo || !type.structInfo->complete)
            throw std::runtime_error("Record initializer requires a complete record type: " + name);
        size_t fieldIndex = findFieldIndex(*type.structInfo, part.field);
        if (fieldIndex == type.structInfo->fields.size())
            throw std::runtime_error("Unknown record initializer field: " + part.field);
        const CField& field = type.structInfo->fields[fieldIndex];
        genDesignatedInitializerAtAddress(field.type, addByteOffset(address, field.offset), designator, partIndex + 1, init, name);
        return;
    }

    throw std::runtime_error("Scalar initializer cannot use a nested designator: " + name);
}

unsigned CodeGen::addByteOffset(unsigned base, uint64_t offset) {
    if (offset == 0)
        return base;
    return builder_->createAdd(
        aurora::Type::getInt64Ty(),
        base,
        builder_->createConstantInt(static_cast<int64_t>(offset)));
}

void CodeGen::genZeroInitializerAtAddress(CType type, unsigned address) {
    const uint64_t slots = (sizeOfType(type) + 7) / 8;
    const uint64_t count = slots == 0 ? 1 : slots;
    for (uint64_t slot = 0; slot < count; ++slot) {
        unsigned slotAddress = address;
        const uint64_t offset = slot * 8;
        if (offset != 0) {
            slotAddress = addByteOffset(address, offset);
        }
        builder_->createStore(builder_->createConstantInt(0), slotAddress);
    }
}

void CodeGen::genDeclStmt(const DeclStmt& stmt) {
    for (const auto& declarator : stmt.declarators)
        declareVariable(declarator.name, declarator.type, declarator.init.get());
}

} // namespace minic
