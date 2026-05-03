#pragma once

#include <cstdint>
#include <string>
#include "Aurora/Air/Type.h"

namespace aurora {

class Function;

class Constant {
public:
    virtual ~Constant() = default;
    [[nodiscard]] Type* getType() const noexcept { return type_; }
protected:
    explicit Constant(Type* t) : type_(t) {}
    Type* type_;
};

class ConstantInt : public Constant {
public:
    [[nodiscard]] static ConstantInt* getInt1(bool val);
    [[nodiscard]] static ConstantInt* getInt8(int8_t val);
    [[nodiscard]] static ConstantInt* getInt16(int16_t val);
    [[nodiscard]] static ConstantInt* getInt32(int32_t val);
    [[nodiscard]] static ConstantInt* getInt64(int64_t val);
    [[nodiscard]] static ConstantInt* getInt(Type* ty, uint64_t val);

    [[nodiscard]] int64_t  getSExtValue() const noexcept { return static_cast<int64_t>(value_); }
    [[nodiscard]] uint64_t getZExtValue() const noexcept { return value_; }
    [[nodiscard]] unsigned getBitWidth() const noexcept { return type_->getSizeInBits(); }
    [[nodiscard]] bool isZero() const noexcept { return value_ == 0; }
    [[nodiscard]] bool isOne()  const noexcept { return value_ == 1; }

private:
    ConstantInt(Type* ty, const uint64_t val) : Constant(ty), value_(val) {}
    uint64_t value_;
};

class ConstantFP : public Constant {
public:
    [[nodiscard]] static ConstantFP* getFloat(float val);
    [[nodiscard]] static ConstantFP* getDouble(double val);

    [[nodiscard]] float  getFloatValue()  const noexcept;
    [[nodiscard]] double getDoubleValue() const noexcept;
private:
    ConstantFP(Type* ty, double val);
    union { float f; double d; } value_;
};

class GlobalVariable : public Constant {
public:
    explicit GlobalVariable(Type* ty, const std::string& name);
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
private:
    std::string name_;
};

} // namespace aurora

