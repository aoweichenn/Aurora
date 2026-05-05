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
                if (initList->entries.front().designator.kind != InitListExpr::Designator::None)
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
        const CField& field = type.structInfo->fields[index];
        unsigned address = base;
        if (field.offset != 0) {
            address = builder_->createAdd(
                aurora::Type::getInt64Ty(),
                base,
                builder_->createConstantInt(static_cast<int64_t>(field.offset)));
        }
        return address;
    };

    genZeroInitializerAtAddress(type, base);

    if (type.kind == CTypeKind::Union) {
        if (type.structInfo->fields.empty())
            return;
        size_t fieldIndex = 0;
        if (!init.entries.empty()) {
            const auto& entry = init.entries.front();
            if (entry.designator.kind == InitListExpr::Designator::Index)
                throw std::runtime_error("Union initializer cannot use an array designator: " + name);
            if (entry.designator.kind == InitListExpr::Designator::Field) {
                fieldIndex = findFieldIndex(*type.structInfo, entry.designator.field);
                if (fieldIndex == type.structInfo->fields.size())
                    throw std::runtime_error("Unknown union initializer field: " + entry.designator.field);
            }
            genInitializerAtAddress(type.structInfo->fields[fieldIndex].type, fieldAddress(fieldIndex), *entry.value, name);
        }
        return;
    }

    size_t nextField = 0;
    for (const auto& entry : init.entries) {
        if (entry.designator.kind == InitListExpr::Designator::Index)
            throw std::runtime_error("Struct initializer cannot use an array designator: " + name);

        size_t fieldIndex = nextField;
        if (entry.designator.kind == InitListExpr::Designator::Field) {
            fieldIndex = findFieldIndex(*type.structInfo, entry.designator.field);
            if (fieldIndex == type.structInfo->fields.size())
                throw std::runtime_error("Unknown struct initializer field: " + entry.designator.field);
        }
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
        unsigned address = base;
        if (offset != 0) {
            address = builder_->createAdd(
                aurora::Type::getInt64Ty(),
                base,
                builder_->createConstantInt(static_cast<int64_t>(offset)));
        }
        return address;
    };

    genZeroInitializerAtAddress(type, base);

    uint64_t nextIndex = 0;
    for (const auto& entry : init.entries) {
        if (entry.designator.kind == InitListExpr::Designator::Field)
            throw std::runtime_error("Array initializer cannot use a field designator: " + name);
        uint64_t targetIndex = entry.designator.kind == InitListExpr::Designator::Index
            ? entry.designator.index
            : nextIndex;
        if (targetIndex >= type.arraySize)
            throw std::runtime_error("Too many values in array initializer: " + name);
        genInitializerAtAddress(elementType, elementAddress(targetIndex), *entry.value, name);
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
        if (initList->entries.front().designator.kind != InitListExpr::Designator::None)
            throw std::runtime_error("Scalar initializer cannot use a designator: " + name);
        genInitializerAtAddress(type, address, *initList->entries.front().value, name);
        return;
    }
    builder_->createStore(genExpr(init), address);
}

void CodeGen::genZeroInitializerAtAddress(CType type, unsigned address) {
    const uint64_t slots = (sizeOfType(type) + 7) / 8;
    const uint64_t count = slots == 0 ? 1 : slots;
    for (uint64_t slot = 0; slot < count; ++slot) {
        unsigned slotAddress = address;
        const uint64_t offset = slot * 8;
        if (offset != 0) {
            slotAddress = builder_->createAdd(
                aurora::Type::getInt64Ty(),
                address,
                builder_->createConstantInt(static_cast<int64_t>(offset)));
        }
        builder_->createStore(builder_->createConstantInt(0), slotAddress);
    }
}

void CodeGen::genDeclStmt(const DeclStmt& stmt) {
    for (const auto& declarator : stmt.declarators)
        declareVariable(declarator.name, declarator.type, declarator.init.get());
}

} // namespace minic
