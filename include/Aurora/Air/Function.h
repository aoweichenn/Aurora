#ifndef AURORA_AIR_FUNCTION_H
#define AURORA_AIR_FUNCTION_H

#include "Aurora/Air/Constant.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/ADT/SmallVector.h"
#include <string>
#include <memory>

namespace aurora {

class Module;

class FunctionType {
public:
    FunctionType(Type* retTy, const SmallVector<Type*, 8>& params);
    Type* getReturnType() const noexcept { return retTy_; }
    const SmallVector<Type*, 8>& getParamTypes() const noexcept { return params_; }
    unsigned getNumParams() const noexcept { return static_cast<unsigned>(params_.size()); }
private:
    Type* retTy_;
    SmallVector<Type*, 8> params_;
};

class Function : public Constant {
public:
    Function(FunctionType* ty, const std::string& name, Module* parent = nullptr);
    ~Function();

    const std::string& getName() const noexcept { return name_; }
    Module* getParent() const noexcept { return module_; }
    FunctionType* getFunctionType() const noexcept { return fnType_; }

    BasicBlock* createBasicBlock(const std::string& name = "");
    const SmallVector<std::unique_ptr<BasicBlock>, 8>& getBlocks() const noexcept { return blocks_; }

    BasicBlock* getEntryBlock() const noexcept { return entryBlock_; }
    unsigned getNumArgs() const noexcept { return fnType_->getNumParams(); }

    unsigned nextVReg() noexcept { return nextVReg_++; }
    unsigned getNumVRegs() const noexcept { return nextVReg_; }
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

#endif // AURORA_AIR_FUNCTION_H
