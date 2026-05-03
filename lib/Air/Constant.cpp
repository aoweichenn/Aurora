#include "Aurora/Air/Constant.h"

namespace aurora {

ConstantInt* ConstantInt::getInt1(bool val) {
    return new ConstantInt(Type::getInt1Ty(), val ? 1 : 0);
}
ConstantInt* ConstantInt::getInt8(int8_t val) {
    return new ConstantInt(Type::getInt8Ty(), static_cast<uint64_t>(val));
}
ConstantInt* ConstantInt::getInt16(int16_t val) {
    return new ConstantInt(Type::getInt16Ty(), static_cast<uint64_t>(val));
}
ConstantInt* ConstantInt::getInt32(int32_t val) {
    return new ConstantInt(Type::getInt32Ty(), static_cast<uint64_t>(static_cast<int64_t>(val)));
}
ConstantInt* ConstantInt::getInt64(int64_t val) {
    return new ConstantInt(Type::getInt64Ty(), static_cast<uint64_t>(val));
}
ConstantInt* ConstantInt::getInt(Type* ty, uint64_t val) {
    return new ConstantInt(ty, val);
}

ConstantFP::ConstantFP(Type* ty, double val) : Constant(ty) {
    if (ty->getSizeInBits() == 32) value_.f = static_cast<float>(val);
    else value_.d = val;
}
ConstantFP* ConstantFP::getFloat(float val) {
    return new ConstantFP(Type::getFloatTy(), static_cast<double>(val));
}
ConstantFP* ConstantFP::getDouble(double val) {
    return new ConstantFP(Type::getDoubleTy(), val);
}

float  ConstantFP::getFloatValue()  const noexcept { return type_->getSizeInBits() == 32 ? value_.f : static_cast<float>(value_.d); }
double ConstantFP::getDoubleValue() const noexcept { return type_->getSizeInBits() == 32 ? static_cast<double>(value_.f) : value_.d; }

GlobalVariable::GlobalVariable(Type* ty, const std::string& name)
    : Constant(ty), name_(name) {}

} // namespace aurora
