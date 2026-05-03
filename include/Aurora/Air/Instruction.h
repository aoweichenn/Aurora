#ifndef AURORA_AIR_INSTRUCTION_H
#define AURORA_AIR_INSTRUCTION_H

#include "Aurora/Air/Type.h"
#include "Aurora/ADT/SmallVector.h"
#include <cstdint>
#include <string>

namespace aurora {

class BasicBlock;
class Function;

enum class AIROpcode : uint16_t {
    // Terminators
    Ret,
    Br,
    CondBr,
    Unreachable,

    // Binary arithmetic
    Add, Sub, Mul,
    UDiv, SDiv,
    URem, SRem,

    // Bitwise
    And, Or, Xor,
    Shl, LShr, AShr,

    // Comparison
    ICmp,

    // Memory
    Alloca,
    Load,
    Store,
    GetElementPtr,

    // Conversion
    SExt, ZExt, Trunc,
    FpToSi, SiToFp,
    BitCast,

    // Control
    Phi,
    Select,

    // Call
    Call
};

enum class ICmpCond : uint8_t {
    EQ, NE, UGT, UGE, ULT, ULE, SGT, SGE, SLT, SLE
};

class AIRInstruction {
public:
    static AIRInstruction* createRet(unsigned valVReg = ~0U);
    static AIRInstruction* createBr(BasicBlock* target);
    static AIRInstruction* createCondBr(unsigned cond, BasicBlock* trueBB, BasicBlock* falseBB);
    static AIRInstruction* createUnreachable();
    static AIRInstruction* createAdd(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createSub(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createMul(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createUDiv(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createSDiv(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createURem(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createSRem(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createAnd(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createOr (Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createXor(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createShl (Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createLShr(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createAShr(Type* ty, unsigned lhs, unsigned rhs);
    static AIRInstruction* createICmp(Type* ty, ICmpCond cond, unsigned lhs, unsigned rhs);
    static AIRInstruction* createAlloca(Type* allocaTy);
    static AIRInstruction* createLoad(Type* ty, unsigned ptr);
    static AIRInstruction* createStore(unsigned val, unsigned ptr);
    static AIRInstruction* createGEP(Type* ty, unsigned ptr, const SmallVector<unsigned, 4>& indices);
    static AIRInstruction* createSExt(Type* dstTy, unsigned src);
    static AIRInstruction* createZExt(Type* dstTy, unsigned src);
    static AIRInstruction* createTrunc(Type* dstTy, unsigned src);
    static AIRInstruction* createPhi(Type* ty, const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& incomings);
    static AIRInstruction* createSelect(Type* ty, unsigned cond, unsigned tVal, unsigned fVal);
    static AIRInstruction* createCall(Type* retTy, Function* callee, const SmallVector<unsigned, 8>& args);

    AIROpcode getOpcode() const noexcept { return opcode_; }
    Type* getType() const noexcept { return type_; }

    unsigned getDestVReg() const noexcept { return destVReg_; }
    bool hasResult() const noexcept;

    unsigned getOperand(unsigned idx) const;
    unsigned getNumOperands() const noexcept;
    BasicBlock* getOperandBlock(unsigned idx) const;
    Function* getCalledFunction() const noexcept;
    ICmpCond getICmpCondition() const noexcept;

    const SmallVector<unsigned, 4>& getIndices() const;
    const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& getPhiIncomings() const;

    BasicBlock* getParent() const noexcept { return parent_; }
    void setParent(BasicBlock* bb) noexcept { parent_ = bb; }

    AIRInstruction* getNext() const noexcept { return next_; }
    AIRInstruction* getPrev() const noexcept { return prev_; }
    void setNext(AIRInstruction* n) noexcept { next_ = n; }
    void setPrev(AIRInstruction* p) noexcept { prev_ = p; }

    void setDestVReg(const unsigned vreg) noexcept { destVReg_ = vreg; }
    void replaceUse(unsigned oldVReg, unsigned newVReg);

    std::string toString() const;

private:
    AIRInstruction(AIROpcode op, Type* ty = nullptr);

    AIROpcode opcode_;
    Type* type_;
    unsigned destVReg_;
    BasicBlock* parent_;
    AIRInstruction* next_;
    AIRInstruction* prev_;

    // Operands storage
    SmallVector<unsigned, 4> operands_;
    SmallVector<BasicBlock*, 2> blockOperands_;
    ICmpCond cond_;
    Function* callee_;
    SmallVector<unsigned, 4> gepIndices_;
    SmallVector<std::pair<BasicBlock*, unsigned>, 4> phiIncomings_;
};

const char* opcodeName(AIROpcode op);
const char* icmpCondName(ICmpCond c);

} // namespace aurora

#endif // AURORA_AIR_INSTRUCTION_H
