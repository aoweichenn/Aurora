#include "Aurora/Air/Function.h"

namespace aurora {

FunctionType::FunctionType(Type* retTy, const SmallVector<Type*, 8>& params)
    : retTy_(retTy), params_(params) {}

Function::Function(FunctionType* ty, const std::string& name, Module* parent)
    : Constant(Type::getFunctionTy(ty->getReturnType(), ty->getParamTypes())),
      fnType_(ty), name_(name), module_(parent),
      entryBlock_(nullptr), nextVReg_(0) {
    // Create parameter vregs
    for (unsigned i = 0; i < ty->getNumParams(); ++i) {
        nextVReg_++;
        vregTypes_.push_back(ty->getParamTypes()[i]);
    }
    // Auto-create entry block
    entryBlock_ = createBasicBlock("entry");
}

Function::~Function() {
    blocks_.clear();
}

BasicBlock* Function::createBasicBlock(const std::string& name) {
    auto bb = std::make_unique<BasicBlock>(name.empty() ? ("L" + std::to_string(blocks_.size())) : name);
    BasicBlock* ptr = bb.get();
    bb->setParent(this);
    blocks_.push_back(std::move(bb));
    if (!entryBlock_) entryBlock_ = ptr;
    return ptr;
}

void Function::recordVRegType(const unsigned vreg, Type* ty) {
    while (vregTypes_.size() <= vreg) {
        vregTypes_.push_back(nullptr);
    }
    vregTypes_[vreg] = ty;
}

} // namespace aurora
