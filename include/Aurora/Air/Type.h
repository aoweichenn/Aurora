#pragma once

#include <cstdint>
#include <string>
#include "Aurora/ADT/SmallVector.h"

namespace aurora {

enum class TypeKind : uint8_t {
    Void,
    Integer,
    Float,
    Pointer,
    Array,
    Struct,
    Function
};

class Type {
public:
    [[nodiscard]] TypeKind getKind() const noexcept { return kind_; }
    [[nodiscard]] unsigned getSizeInBits() const noexcept { return sizeInBits_; }
    [[nodiscard]] unsigned getAlignInBits() const noexcept { return alignInBits_; }

    [[nodiscard]] bool isInteger() const noexcept { return kind_ == TypeKind::Integer; }
    [[nodiscard]] bool isFloat()   const noexcept { return kind_ == TypeKind::Float; }
    [[nodiscard]] bool isPointer() const noexcept { return kind_ == TypeKind::Pointer; }
    [[nodiscard]] bool isArray()   const noexcept { return kind_ == TypeKind::Array; }
    [[nodiscard]] bool isStruct()  const noexcept { return kind_ == TypeKind::Struct; }
    [[nodiscard]] bool isFunction()const noexcept { return kind_ == TypeKind::Function; }
    [[nodiscard]] bool isVoid()    const noexcept { return kind_ == TypeKind::Void; }

    // Integer
    [[nodiscard]] static Type* getInt1Ty();
    [[nodiscard]] static Type* getInt8Ty();
    [[nodiscard]] static Type* getInt16Ty();
    [[nodiscard]] static Type* getInt32Ty();
    [[nodiscard]] static Type* getInt64Ty();

    // Float
    [[nodiscard]] static Type* getFloatTy();
    [[nodiscard]] static Type* getDoubleTy();

    // Void
    [[nodiscard]] static Type* getVoidTy();

    // Derived
    [[nodiscard]] static Type* getPointerTy(Type* elemType);
    [[nodiscard]] static Type* getArrayTy(Type* elemType, unsigned numElements);
    [[nodiscard]] static Type* getStructTy(SmallVector<Type*, 8> members);
    [[nodiscard]] static Type* getFunctionTy(Type* returnType, SmallVector<Type*, 8> paramTypes);

    // Accessor for derived types
    [[nodiscard]] Type* getElementType() const noexcept { return elemType_; }
    [[nodiscard]] unsigned getNumElements() const noexcept { return numElements_; }
    [[nodiscard]] const SmallVector<Type*, 8>& getStructMembers() const;
    [[nodiscard]] Type* getReturnType() const noexcept { return elemType_; }
    [[nodiscard]] const SmallVector<Type*, 8>& getParamTypes() const;

    [[nodiscard]] std::string toString() const;

    Type(TypeKind kind, unsigned size, unsigned align);
    Type(TypeKind kind, Type* elem, unsigned size, unsigned align);
    Type(TypeKind kind, Type* elem, unsigned num, unsigned size, unsigned align);
    Type(TypeKind kind, SmallVector<Type*, 8> members);

    TypeKind kind_;
    unsigned sizeInBits_;
    unsigned alignInBits_;
    Type* elemType_;
    unsigned numElements_;
    SmallVector<Type*, 8> members_;
    SmallVector<Type*, 8> paramTypes_;
};

} // namespace aurora

