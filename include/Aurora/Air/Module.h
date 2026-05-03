#pragma once

#include <memory>
#include <string>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/Function.h"

namespace aurora {

class DataLayout {
public:
    DataLayout() = default;
    [[nodiscard]] bool isLittleEndian() const noexcept { return littleEndian_; }
    void setLittleEndian(const bool v) noexcept { littleEndian_ = v; }
    [[nodiscard]] unsigned getPointerSize() const noexcept { return ptrSize_; }
    void setPointerSize(const unsigned v) noexcept { ptrSize_ = v; }
private:
    bool littleEndian_ = true;
    unsigned ptrSize_ = 64;
};

class Module {
public:
    explicit Module(const std::string& name = "");
    ~Module();

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] const DataLayout& getDataLayout() const noexcept { return dl_; }
    [[nodiscard]] DataLayout& getDataLayout() noexcept { return dl_; }

    [[nodiscard]] Function* createFunction(FunctionType* ty, const std::string& name);
    [[nodiscard]] GlobalVariable* createGlobal(Type* ty, const std::string& name);

    [[nodiscard]] const SmallVector<std::unique_ptr<Function>, 16>& getFunctions() const noexcept { return functions_; }
    [[nodiscard]] const SmallVector<std::unique_ptr<GlobalVariable>, 8>& getGlobals() const noexcept { return globals_; }

private:
    std::string name_;
    DataLayout dl_;
    SmallVector<std::unique_ptr<Function>, 16> functions_;
    SmallVector<std::unique_ptr<GlobalVariable>, 8> globals_;
};

} // namespace aurora

