#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Function.h"

namespace aurora {

AIRBuilder::AIRBuilder() : insertBlock_(nullptr), insertPoint_(nullptr) {}
AIRBuilder::AIRBuilder(BasicBlock* insertBlock) : insertBlock_(insertBlock), insertPoint_(nullptr) {}

void AIRBuilder::setInsertPoint(BasicBlock* bb, AIRInstruction* before) {
    insertBlock_ = bb;
    insertPoint_ = before;
}

unsigned AIRBuilder::createAdd(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createAdd(ty, lhs, rhs), vreg);
    return vreg;
}

unsigned AIRBuilder::createSub(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createSub(ty, lhs, rhs), vreg);
    return vreg;
}

unsigned AIRBuilder::createMul(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createMul(ty, lhs, rhs), vreg);
    return vreg;
}

unsigned AIRBuilder::createUDiv(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createUDiv(ty, lhs, rhs), vreg);
    return vreg;
}

unsigned AIRBuilder::createSDiv(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createSDiv(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createAnd(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createAnd(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createOr(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createOr(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createXor(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createXor(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createShl(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createShl(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createLShr(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createLShr(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createAShr(Type* ty, const unsigned lhs, const unsigned rhs) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createAShr(ty, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createICmp(const ICmpCond cond, const unsigned lhs, const unsigned rhs) {
    auto* ty = Type::getInt1Ty();
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createICmp(ty, cond, lhs, rhs), vreg);
    return vreg;
}
unsigned AIRBuilder::createAlloca(Type* allocaTy) {
    Type* ptrTy = Type::getPointerTy(allocaTy);
    const unsigned vreg = allocateVReg(ptrTy);
    setResult(AIRInstruction::createAlloca(allocaTy), vreg);
    return vreg;
}
unsigned AIRBuilder::createLoad(Type* ty, const unsigned ptr) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createLoad(ty, ptr), vreg);
    return vreg;
}
void AIRBuilder::createStore(const unsigned val, const unsigned ptr) {
    insertInstruction(AIRInstruction::createStore(val, ptr));
}
unsigned AIRBuilder::createGEP(Type* resultTy, const unsigned ptr, const SmallVector<unsigned, 4>& indices) {
    const unsigned vreg = allocateVReg(resultTy);
    setResult(AIRInstruction::createGEP(resultTy, ptr, indices), vreg);
    return vreg;
}
unsigned AIRBuilder::createSExt(Type* dstTy, const unsigned src) {
    const unsigned vreg = allocateVReg(dstTy);
    setResult(AIRInstruction::createSExt(dstTy, src), vreg);
    return vreg;
}
unsigned AIRBuilder::createZExt(Type* dstTy, const unsigned src) {
    const unsigned vreg = allocateVReg(dstTy);
    setResult(AIRInstruction::createZExt(dstTy, src), vreg);
    return vreg;
}
unsigned AIRBuilder::createTrunc(Type* dstTy, const unsigned src) {
    const unsigned vreg = allocateVReg(dstTy);
    setResult(AIRInstruction::createTrunc(dstTy, src), vreg);
    return vreg;
}
unsigned AIRBuilder::createSelect(Type* ty, const unsigned cond, const unsigned tVal, const unsigned fVal) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createSelect(ty, cond, tVal, fVal), vreg);
    return vreg;
}
unsigned AIRBuilder::createCall(Function* callee, const SmallVector<unsigned, 8>& args) {
    Type* retTy = callee->getFunctionType()->getReturnType();
    const unsigned vreg = allocateVReg(retTy);
    setResult(AIRInstruction::createCall(retTy, callee, args), vreg);
    return vreg;
}
unsigned AIRBuilder::createPhi(Type* ty, const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& incomings) {
    const unsigned vreg = allocateVReg(ty);
    setResult(AIRInstruction::createPhi(ty, incomings), vreg);
    return vreg;
}
void AIRBuilder::createRet(const unsigned val) {
    insertInstruction(AIRInstruction::createRet(val));
}
void AIRBuilder::createRetVoid() {
    insertInstruction(AIRInstruction::createRet(~0U));
}
void AIRBuilder::createBr(BasicBlock* target) {
    insertInstruction(AIRInstruction::createBr(target));
}
void AIRBuilder::createCondBr(const unsigned cond, BasicBlock* trueBB, BasicBlock* falseBB) {
    insertInstruction(AIRInstruction::createCondBr(cond, trueBB, falseBB));
}
void AIRBuilder::createUnreachable() {
    insertInstruction(AIRInstruction::createUnreachable());
}
void AIRBuilder::setDestVReg(AIRInstruction* inst, const unsigned vreg) {
    inst->setDestVReg(vreg);
}
Function* AIRBuilder::getFunction() const {
    return insertBlock_ ? insertBlock_->getParent() : nullptr;
}
unsigned AIRBuilder::allocateVReg(Type* ty) {
    auto* fn = getFunction();
    const unsigned vreg = fn->nextVReg();
    fn->recordVRegType(vreg, ty);
    return vreg;
}
void AIRBuilder::insertInstruction(AIRInstruction* inst) {
    if (insertPoint_)
        insertBlock_->insertBefore(insertPoint_, inst);
    else
        insertBlock_->pushBack(inst);
}
void AIRBuilder::setResult(AIRInstruction* inst, const unsigned vreg) {
    inst->setDestVReg(vreg);
    insertInstruction(inst);
}

} // namespace aurora
