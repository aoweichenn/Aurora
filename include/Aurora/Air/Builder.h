#ifndef AURORA_AIR_BUILDER_H
#define AURORA_AIR_BUILDER_H

#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/ADT/SmallVector.h"

namespace aurora {

class AIRBuilder {
public:
    AIRBuilder();
    explicit AIRBuilder(BasicBlock* insertBlock);

    void setInsertPoint(BasicBlock* bb, AIRInstruction* before = nullptr);
    BasicBlock* getInsertBlock() const noexcept { return insertBlock_; }

    unsigned createAdd(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createSub(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createMul(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createUDiv(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createSDiv(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createAnd(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createOr (Type* ty, unsigned lhs, unsigned rhs);
    unsigned createXor(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createShl (Type* ty, unsigned lhs, unsigned rhs);
    unsigned createLShr(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createAShr(Type* ty, unsigned lhs, unsigned rhs);
    unsigned createICmp(ICmpCond cond, unsigned lhs, unsigned rhs);
    unsigned createAlloca(Type* allocaTy);
    unsigned createLoad(Type* ty, unsigned ptr);
    void     createStore(unsigned val, unsigned ptr);
    unsigned createGEP(Type* resultTy, unsigned ptr, const SmallVector<unsigned, 4>& indices);
    unsigned createSExt(Type* dstTy, unsigned src);
    unsigned createZExt(Type* dstTy, unsigned src);
    unsigned createTrunc(Type* dstTy, unsigned src);
    unsigned createSelect(Type* ty, unsigned cond, unsigned tVal, unsigned fVal);
    unsigned createCall(Function* callee, const SmallVector<unsigned, 8>& args);
    unsigned createPhi(Type* ty, const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& incomings);

    void createRet(unsigned val);
    void createRetVoid();
    void createBr(BasicBlock* target);
    void createCondBr(unsigned cond, BasicBlock* trueBB, BasicBlock* falseBB);
    void createUnreachable();

    void setDestVReg(AIRInstruction* inst, unsigned vreg);

private:
    BasicBlock* insertBlock_;
    AIRInstruction* insertPoint_;
    Function* getFunction() const;
    unsigned allocateVReg(Type* ty);

    void insertInstruction(AIRInstruction* inst);
    void setResult(AIRInstruction* inst, unsigned vreg);
};

} // namespace aurora

#endif // AURORA_AIR_BUILDER_H
