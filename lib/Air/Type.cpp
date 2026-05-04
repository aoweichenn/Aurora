#include "Aurora/Air/Type.h"
#include <unordered_map>
#include <mutex>
#include <sstream>

namespace aurora {

namespace {
std::mutex g_typeMutex;

Type* g_voidTy     = nullptr;
Type* g_int1Ty     = nullptr;
Type* g_int8Ty     = nullptr;
Type* g_int16Ty    = nullptr;
Type* g_int32Ty    = nullptr;
Type* g_int64Ty    = nullptr;
Type* g_floatTy    = nullptr;
Type* g_doubleTy   = nullptr;

struct TypeKey {
    TypeKind kind;
    Type* elem;  // for pointer, array
    unsigned num; // for array element count

    bool operator==(const TypeKey& o) const {
        return kind == o.kind && elem == o.elem && num == o.num;
    }
};

struct TypeKeyHash {
    size_t operator()(const TypeKey& k) const {
        return (static_cast<size_t>(k.kind) << 4) ^
               reinterpret_cast<size_t>(k.elem) ^
               (static_cast<size_t>(k.num) << 8);
    }
};

std::unordered_map<TypeKey, Type*, TypeKeyHash> g_ptrTypes;
std::unordered_map<TypeKey, Type*, TypeKeyHash> g_arrTypes;

SmallVector<Type*, 8> g_emptyMembers;

void initTypes() {
    if (g_voidTy) return;
    g_voidTy     = new Type(TypeKind::Void, 0, 0);
    g_int1Ty     = new Type(TypeKind::Integer, 1, 1);
    g_int8Ty     = new Type(TypeKind::Integer, 8, 8);
    g_int16Ty    = new Type(TypeKind::Integer, 16, 16);
    g_int32Ty    = new Type(TypeKind::Integer, 32, 32);
    g_int64Ty    = new Type(TypeKind::Integer, 64, 64);
    g_floatTy    = new Type(TypeKind::Float, 32, 32);
    g_doubleTy   = new Type(TypeKind::Float, 64, 64);
}
} // anonymous namespace

Type* Type::getInt1Ty () { initTypes(); return g_int1Ty; }

Type* Type::getInt8Ty () { initTypes(); return g_int8Ty; }

Type* Type::getInt16Ty() { initTypes(); return g_int16Ty; }

Type* Type::getInt32Ty() { initTypes(); return g_int32Ty; }

Type* Type::getInt64Ty() { initTypes(); return g_int64Ty; }
Type* Type::getFloatTy() { initTypes(); return g_floatTy; }
Type* Type::getDoubleTy(){ initTypes(); return g_doubleTy; }
Type* Type::getVoidTy() {
    initTypes();
    return g_voidTy;
}
Type* Type::getPointerTy(Type* elemType) {
    std::lock_guard<std::mutex> lock(g_typeMutex);
    const TypeKey key{TypeKind::Pointer, elemType, 0};
    const auto it = g_ptrTypes.find(key);
    if (it != g_ptrTypes.end()) return it->second;
    auto t = new Type(TypeKind::Pointer, elemType, 64, 64);
    g_ptrTypes[key] = t;
    return t;
}
Type* Type::getArrayTy(Type* elemType, const unsigned numElements) {
    std::lock_guard<std::mutex> lock(g_typeMutex);
    const TypeKey key{TypeKind::Array, elemType, numElements};
    const auto it = g_arrTypes.find(key);
    if (it != g_arrTypes.end()) return it->second;
    auto t = new Type(TypeKind::Array, elemType, numElements,
                      elemType->getSizeInBits() * numElements, elemType->getAlignInBits());
    g_arrTypes[key] = t;
    return t;
}
Type* Type::getStructTy(SmallVector<Type*, 8> members) {
    return new Type(TypeKind::Struct, std::move(members));
}
Type* Type::getFunctionTy(Type* returnType, SmallVector<Type*, 8> paramTypes) {
    auto t = new Type(TypeKind::Function, returnType, 0, 0);
    t->paramTypes_ = std::move(paramTypes);
    return t;
}
Type* Type::getFunctionTy(Type* returnType, SmallVector<Type*, 8> paramTypes, bool isVarArg) {
    auto t = new Type(TypeKind::Function, returnType, 0, 0);
    t->paramTypes_ = std::move(paramTypes);
    t->isVarArg_ = isVarArg;
    return t;
}

const SmallVector<Type*, 8>& Type::getStructMembers() const {
    if (kind_ == TypeKind::Struct) return members_;
    return g_emptyMembers;
}

unsigned Type::getMemberOffset(const unsigned idx) const {
    return (kind_ == TypeKind::Struct && idx < memberOffsets_.size()) ? memberOffsets_[idx] : 0;
}

const SmallVector<Type*, 8>& Type::getParamTypes() const {
    if (kind_ == TypeKind::Function) return paramTypes_;
    return g_emptyMembers;
}

std::string Type::toString() const {
    switch (kind_) {
        case TypeKind::Void:    return "void";
        case TypeKind::Integer: {
            switch (sizeInBits_) {
                case 1:  return "i1";
                case 8:  return "i8";
                case 16: return "i16";
                case 32: return "i32";
                case 64: return "i64";
                default: return "i" + std::to_string(sizeInBits_);
            }
        }
        case TypeKind::Float: {
            return (sizeInBits_ == 32) ? "float" : "double";
        }
        case TypeKind::Pointer:
            return elemType_ ? (elemType_->toString() + "*") : "ptr";
        case TypeKind::Array:
            return "[" + std::to_string(numElements_) + " x " + (elemType_ ? elemType_->toString() : "?") + "]";
        case TypeKind::Struct: {
            std::ostringstream os;
            os << "{";
            for (unsigned i = 0; i < members_.size(); ++i) {
                if (i) os << ", ";
                os << members_[i]->toString();
            }
            os << "}";
            return os.str();
        }
        case TypeKind::Function: {
            std::ostringstream os;
            os << (elemType_ ? elemType_->toString() : "void") << " (";
            for (unsigned i = 0; i < paramTypes_.size(); ++i) {
                if (i) os << ", ";
                os << paramTypes_[i]->toString();
            }
            os << ")";
            return os.str();
        }
    }
    return "unknown";
}

Type::Type(const TypeKind kind, const unsigned size, const unsigned align)
    : kind_(kind), sizeInBits_(size), alignInBits_(align),
      elemType_(nullptr), numElements_(0) {}

Type::Type(const TypeKind kind, Type* elem, const unsigned size, const unsigned align)
    : kind_(kind), sizeInBits_(size), alignInBits_(align),
      elemType_(elem), numElements_(0) {}

Type::Type(const TypeKind kind, Type* elem, const unsigned num, const unsigned size, const unsigned align)
    : kind_(kind), sizeInBits_(size), alignInBits_(align),
      elemType_(elem), numElements_(num) {}

Type::Type(const TypeKind kind, SmallVector<Type*, 8> members)
    : kind_(kind), elemType_(nullptr), numElements_(0), members_(std::move(members)) {
    sizeInBits_ = 0;
    alignInBits_ = 0;
    unsigned offset = 0;
    for (const auto* m : members_) {
        unsigned align = std::max(1u, m->getAlignInBits());
        offset = (offset + align - 1) / align * align; // align up
        memberOffsets_.push_back(offset);
        offset += m->getSizeInBits();
        if (m->getAlignInBits() > alignInBits_)
            alignInBits_ = m->getAlignInBits();
    }
    if (alignInBits_ > 0)
        offset = (offset + alignInBits_ - 1) / alignInBits_ * alignInBits_;
    sizeInBits_ = offset;
}

} // namespace aurora
