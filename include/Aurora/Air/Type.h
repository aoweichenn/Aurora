#ifndef AURORA_AIR_TYPE_H
#define AURORA_AIR_TYPE_H

#include "Aurora/ADT/SmallVector.h"
#include <cstdint>
#include <string>

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
    TypeKind getKind() const noexcept { return kind_; }
    unsigned getSizeInBits() const noexcept { return sizeInBits_; }
    unsigned getAlignInBits() const noexcept { return alignInBits_; }

    bool isInteger() const noexcept { return kind_ == TypeKind::Integer; }
    bool isFloat()   const noexcept { return kind_ == TypeKind::Float; }
    bool isPointer() const noexcept { return kind_ == TypeKind::Pointer; }
    bool isArray()   const noexcept { return kind_ == TypeKind::Array; }
    bool isStruct()  const noexcept { return kind_ == TypeKind::Struct; }
    bool isFunction()const noexcept { return kind_ == TypeKind::Function; }
    bool isVoid()    const noexcept { return kind_ == TypeKind::Void; }

    // Integer
    static Type* getInt1Ty();
    static Type* getInt8Ty();
    static Type* getInt16Ty();
    static Type* getInt32Ty();
    static Type* getInt64Ty();

    // Float
    static Type* getFloatTy();
    static Type* getDoubleTy();

    // Void
    static Type* getVoidTy();

    // Derived
    static Type* getPointerTy(Type* elemType);
    static Type* getArrayTy(Type* elemType, unsigned numElements);
    static Type* getStructTy(SmallVector<Type*, 8> members);
    static Type* getFunctionTy(Type* returnType, SmallVector<Type*, 8> paramTypes);

    // Accessor for derived types
    Type* getElementType() const noexcept { return elemType_; }
    unsigned getNumElements() const noexcept { return numElements_; }
    const SmallVector<Type*, 8>& getStructMembers() const;
    Type* getReturnType() const noexcept { return elemType_; }
    const SmallVector<Type*, 8>& getParamTypes() const;

    std::string toString() const;

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

#endif // AURORA_AIR_TYPE_H
