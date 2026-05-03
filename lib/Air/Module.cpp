#include "Aurora/Air/Module.h"

namespace aurora {

Module::Module(const std::string& name) : name_(name) {}
Module::~Module() = default;

Function* Module::createFunction(FunctionType* ty, const std::string& name) {
    auto fn = std::make_unique<Function>(ty, name, this);
    Function* ptr = fn.get();
    functions_.push_back(std::move(fn));
    return ptr;
}

GlobalVariable* Module::createGlobal(Type* ty, const std::string& name) {
    auto gv = std::make_unique<GlobalVariable>(ty, name);
    GlobalVariable* ptr = gv.get();
    globals_.push_back(std::move(gv));
    return ptr;
}

} // namespace aurora
