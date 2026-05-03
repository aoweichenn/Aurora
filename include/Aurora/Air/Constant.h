#ifndef AURORA_AIR_CONSTANT_H
#define AURORA_AIR_CONSTANT_H

#include "Aurora/Air/Type.h"
#include <cstdint>
#include <string>

namespace aurora {

class Function;

class Constant {
public:
    virtual ~Constant() = default;
    Type* getType() const noexcept { return type_; }
protected:
    explicit Constant(Type* t) : type_(t) {}
    Type* type_;
};

class ConstantInt : public Constant {
public:
    static ConstantInt* getInt1(bool val);
    static ConstantInt* getInt8(int8_t val);
    static ConstantInt* getInt16(int16_t val);
    static ConstantInt* getInt32(int32_t val);
    static ConstantInt* getInt64(int64_t val);
    static ConstantInt* getInt(Type* ty, uint64_t val);

    int64_t  getSExtValue() const noexcept { return static_cast<int64_t>(value_); }
    uint64_t getZExtValue() const noexcept { return value_; }
    unsigned getBitWidth() const noexcept { return type_->getSizeInBits(); }
    bool isZero() const noexcept { return value_ == 0; }
    bool isOne()  const noexcept { return value_ == 1; }

private:
    ConstantInt(Type* ty, uint64_t val) : Constant(ty), value_(val) {}
    uint64_t value_;
};

class ConstantFP : public Constant {
public:
    static ConstantFP* getFloat(float val);
    static ConstantFP* getDouble(double val);

    float  getFloatValue()  const noexcept;
    double getDoubleValue() const noexcept;
private:
    ConstantFP(Type* ty, double val);
    union { float f; double d; } value_;
};

class GlobalVariable : public Constant {
public:
    GlobalVariable(Type* ty, const std::string& name);
    const std::string& getName() const noexcept { return name_; }
private:
    std::string name_;
};

} // namespace aurora

#endif // AURORA_AIR_CONSTANT_H
