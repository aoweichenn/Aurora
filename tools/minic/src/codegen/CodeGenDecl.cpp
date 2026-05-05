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
        CType elementType = type;
        elementType.arraySize = 0;
        if ((elementType.kind == CTypeKind::Struct || elementType.kind == CTypeKind::Union) && elementType.pointerDepth == 0 && init)
            throw std::runtime_error("Record array initializer is not supported yet: " + name);
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
    if ((type.kind != CTypeKind::Struct && type.kind != CTypeKind::Union) || !type.structInfo || !type.structInfo->complete)
        throw std::runtime_error("Record initializer requires a complete record type: " + name);
    if (type.kind == CTypeKind::Union && init.entries.size() > 1)
        throw std::runtime_error("Too many values in union initializer: " + name);

    aurora::SmallVector<unsigned, 4> indices;
    unsigned base = builder_->createGEP(toAirType(type, false), pointer, indices);

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

    if (type.kind == CTypeKind::Union) {
        if (type.structInfo->fields.empty())
            return;
        size_t fieldIndex = 0;
        unsigned value = builder_->createConstantInt(0);
        if (!init.entries.empty()) {
            const auto& entry = init.entries.front();
            if (entry.designator.kind == InitListExpr::Designator::Index)
                throw std::runtime_error("Union initializer cannot use an array designator: " + name);
            if (entry.designator.kind == InitListExpr::Designator::Field) {
                fieldIndex = findFieldIndex(*type.structInfo, entry.designator.field);
                if (fieldIndex == type.structInfo->fields.size())
                    throw std::runtime_error("Unknown union initializer field: " + entry.designator.field);
            }
            value = genExpr(*entry.value);
        }
        builder_->createStore(value, fieldAddress(fieldIndex));
        return;
    }

    for (size_t index = 0; index < type.structInfo->fields.size(); ++index)
        builder_->createStore(builder_->createConstantInt(0), fieldAddress(index));

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
        builder_->createStore(genExpr(*entry.value), fieldAddress(fieldIndex));
        nextField = fieldIndex + 1;
    }
}

void CodeGen::genArrayInitializer(CType type, unsigned pointer, const InitListExpr& init, const std::string& name) {
    CType elementType = type;
    elementType.arraySize = 0;
    aurora::SmallVector<unsigned, 4> indices;
    unsigned base = builder_->createGEP(toAirType(type.decayArray(), false), pointer, indices);

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

    for (uint64_t index = 0; index < type.arraySize; ++index)
        builder_->createStore(builder_->createConstantInt(0), elementAddress(index));

    uint64_t nextIndex = 0;
    for (const auto& entry : init.entries) {
        if (entry.designator.kind == InitListExpr::Designator::Field)
            throw std::runtime_error("Array initializer cannot use a field designator: " + name);
        uint64_t targetIndex = entry.designator.kind == InitListExpr::Designator::Index
            ? entry.designator.index
            : nextIndex;
        if (targetIndex >= type.arraySize)
            throw std::runtime_error("Too many values in array initializer: " + name);
        builder_->createStore(genExpr(*entry.value), elementAddress(targetIndex));
        nextIndex = targetIndex + 1;
    }
}

void CodeGen::genDeclStmt(const DeclStmt& stmt) {
    for (const auto& declarator : stmt.declarators)
        declareVariable(declarator.name, declarator.type, declarator.init.get());
}

} // namespace minic
