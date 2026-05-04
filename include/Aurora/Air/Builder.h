#pragma once

#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Instruction.h"

namespace aurora {

class AIRBuilder {
public:
    AIRBuilder();
    explicit AIRBuilder(BasicBlock* insertBlock);

    void setInsertPoint(BasicBlock* bb, AIRInstruction* before = nullptr);
    [[nodiscard]] BasicBlock* getInsertBlock() const noexcept { return insertBlock_; }

    [[nodiscard]] unsigned createAdd(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createSub(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createMul(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createUDiv(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createSDiv(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createAnd(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createOr (Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createXor(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createShl (Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createLShr(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createAShr(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createICmp(ICmpCond cond, unsigned lhs, unsigned rhs);
    [[nodiscard]] unsigned createAlloca(Type* allocaTy);
    [[nodiscard]] unsigned createLoad(Type* ty, unsigned ptr);
    void     createStore(unsigned val, unsigned ptr);
    [[nodiscard]] unsigned createGEP(Type* resultTy, unsigned ptr, const SmallVector<unsigned, 4>& indices);
    [[nodiscard]] unsigned createSExt(Type* dstTy, unsigned src);
    [[nodiscard]] unsigned createZExt(Type* dstTy, unsigned src);
    [[nodiscard]] unsigned createTrunc(Type* dstTy, unsigned src);
    [[nodiscard]] unsigned createSelect(Type* ty, unsigned cond, unsigned tVal, unsigned fVal);
    [[nodiscard]] unsigned createCall(Function* callee, const SmallVector<unsigned, 8>& args);
    [[nodiscard]]     unsigned createPhi(Type* ty, const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& incomings);
    unsigned createConstantInt(int64_t val);
    [[nodiscard]] unsigned createGlobalAddress(Type* ty, const char* globalName);

    void createRet(unsigned val);
    void createRetVoid();
    void createBr(BasicBlock* target);
    void createCondBr(unsigned cond, BasicBlock* trueBB, BasicBlock* falseBB);
    void createUnreachable();

    void setDestVReg(AIRInstruction* inst, unsigned vreg) const;

private:
    BasicBlock* insertBlock_;
    AIRInstruction* insertPoint_;
    [[nodiscard]] Function* getFunction() const;
    [[nodiscard]] unsigned allocateVReg(Type* ty) const;

    void insertInstruction(AIRInstruction* inst) const;
    void setResult(AIRInstruction* inst, unsigned vreg);
};

} // namespace aurora

