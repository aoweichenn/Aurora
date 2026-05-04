#pragma once

#include <memory>
#include <string>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Constant.h"

namespace aurora {

class Module;

class FunctionType {
public:
    FunctionType(Type* retTy, const SmallVector<Type*, 8>& params, bool isVarArg = false);
    [[nodiscard]] Type* getReturnType() const noexcept { return retTy_; }
    [[nodiscard]] const SmallVector<Type*, 8>& getParamTypes() const noexcept { return params_; }
    [[nodiscard]] unsigned getNumParams() const noexcept { return static_cast<unsigned>(params_.size()); }
    [[nodiscard]] bool isVarArg() const noexcept { return isVarArg_; }
private:
    Type* retTy_;
    SmallVector<Type*, 8> params_;
    bool isVarArg_ = false;
};

class Function : public Constant {
public:
    Function(FunctionType* ty, const std::string& name, Module* parent = nullptr);
    ~Function();

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] Module* getParent() const noexcept { return module_; }
    [[nodiscard]] FunctionType* getFunctionType() const noexcept { return fnType_; }

    [[nodiscard]] BasicBlock* createBasicBlock(const std::string& name = "");
    [[nodiscard]] const SmallVector<std::unique_ptr<BasicBlock>, 8>& getBlocks() const noexcept { return blocks_; }

    [[nodiscard]] BasicBlock* getEntryBlock() const noexcept { return entryBlock_; }
    [[nodiscard]] unsigned getNumArgs() const noexcept { return fnType_->getNumParams(); }

    [[nodiscard]] unsigned nextVReg() noexcept { return nextVReg_++; }
    [[nodiscard]] unsigned getNumVRegs() const noexcept { return nextVReg_; }
    void recordVRegType(unsigned vreg, Type* ty);

private:
    FunctionType* fnType_;
    std::string name_;
    Module* module_;
    SmallVector<std::unique_ptr<BasicBlock>, 8> blocks_;
    BasicBlock* entryBlock_;
    unsigned nextVReg_;
    SmallVector<Type*, 64> vregTypes_;
};

} // namespace aurora

