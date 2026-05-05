#include "minic/codegen/CodeGen.h"
#include <stdexcept>

namespace minic {

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

    unsigned initialValue = builder_->createConstantInt(0);
    if (init) {
        if (auto* initList = dynamic_cast<const InitListExpr*>(init)) {
            if (initList->values.size() > 1)
                throw std::runtime_error("Scalar initializer has too many values: " + name);
            if (!initList->values.empty())
                initialValue = genExpr(*initList->values.front());
        } else {
            initialValue = genExpr(*init);
        }
    }
    builder_->createStore(initialValue, pointer);
}

void CodeGen::genArrayInitializer(CType type, unsigned pointer, const InitListExpr& init, const std::string& name) {
    if (init.values.size() > type.arraySize)
        throw std::runtime_error("Too many values in array initializer: " + name);

    CType elementType = type;
    elementType.arraySize = 0;
    aurora::SmallVector<unsigned, 4> indices;
    unsigned base = builder_->createGEP(toAirType(type.decayArray(), false), pointer, indices);

    for (uint64_t index = 0; index < type.arraySize; ++index) {
        unsigned value = index < init.values.size()
            ? genExpr(*init.values[static_cast<size_t>(index)])
            : builder_->createConstantInt(0);
        const uint64_t offset = index * sizeOfType(elementType);
        unsigned address = base;
        if (offset != 0) {
            address = builder_->createAdd(
                aurora::Type::getInt64Ty(),
                base,
                builder_->createConstantInt(static_cast<int64_t>(offset)));
        }
        builder_->createStore(value, address);
    }
}

void CodeGen::genDeclStmt(const DeclStmt& stmt) {
    for (const auto& declarator : stmt.declarators)
        declareVariable(declarator.name, declarator.type, declarator.init.get());
}

} // namespace minic
