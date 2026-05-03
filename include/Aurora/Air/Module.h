#ifndef AURORA_AIR_MODULE_H
#define AURORA_AIR_MODULE_H

#include "Aurora/Air/Function.h"
#include "Aurora/ADT/SmallVector.h"
#include <string>
#include <memory>

namespace aurora {

class DataLayout {
public:
    DataLayout() = default;
    bool isLittleEndian() const noexcept { return littleEndian_; }
    void setLittleEndian(const bool v) noexcept { littleEndian_ = v; }
    unsigned getPointerSize() const noexcept { return ptrSize_; }
    void setPointerSize(const unsigned v) noexcept { ptrSize_ = v; }
private:
    bool littleEndian_ = true;
    unsigned ptrSize_ = 64;
};

class Module {
public:
    explicit Module(const std::string& name = "");
    ~Module();

    const std::string& getName() const noexcept { return name_; }
    const DataLayout& getDataLayout() const noexcept { return dl_; }
    DataLayout& getDataLayout() noexcept { return dl_; }

    [[nodiscard]] Function* createFunction(FunctionType* ty, const std::string& name);
    [[nodiscard]] GlobalVariable* createGlobal(Type* ty, const std::string& name);

    const SmallVector<std::unique_ptr<Function>, 16>& getFunctions() const noexcept { return functions_; }
    const SmallVector<std::unique_ptr<GlobalVariable>, 8>& getGlobals() const noexcept { return globals_; }

private:
    std::string name_;
    DataLayout dl_;
    SmallVector<std::unique_ptr<Function>, 16> functions_;
    SmallVector<std::unique_ptr<GlobalVariable>, 8> globals_;
};

} // namespace aurora

#endif // AURORA_AIR_MODULE_H
