#pragma once

#include <cstdint>
#include <string>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/Type.h"

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
    Call,

    // Constant
    ConstantInt,

    // Switch
    Switch,

    // Struct
    ExtractValue,
    InsertValue
};

enum class ICmpCond : uint8_t {
    EQ, NE, UGT, UGE, ULT, ULE, SGT, SGE, SLT, SLE
};

class AIRInstruction {
public:
    [[nodiscard]] static AIRInstruction* createRet(unsigned valVReg = ~0U);
    [[nodiscard]] static AIRInstruction* createBr(BasicBlock* target);
    [[nodiscard]] static AIRInstruction* createCondBr(unsigned cond, BasicBlock* trueBB, BasicBlock* falseBB);
    [[nodiscard]] static AIRInstruction* createUnreachable();
    [[nodiscard]] static AIRInstruction* createAdd(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createSub(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createMul(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createUDiv(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createSDiv(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createURem(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createSRem(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createAnd(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createOr (Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createXor(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createShl (Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createLShr(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createAShr(Type* ty, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createICmp(Type* ty, ICmpCond cond, unsigned lhs, unsigned rhs);
    [[nodiscard]] static AIRInstruction* createAlloca(Type* allocaTy);
    [[nodiscard]] static AIRInstruction* createLoad(Type* ty, unsigned ptr);
    [[nodiscard]] static AIRInstruction* createStore(unsigned val, unsigned ptr);
    [[nodiscard]] static AIRInstruction* createGEP(Type* ty, unsigned ptr, const SmallVector<unsigned, 4>& indices);
    [[nodiscard]] static AIRInstruction* createSExt(Type* dstTy, unsigned src);
    [[nodiscard]] static AIRInstruction* createZExt(Type* dstTy, unsigned src);
    [[nodiscard]] static AIRInstruction* createTrunc(Type* dstTy, unsigned src);
    [[nodiscard]] static AIRInstruction* createPhi(Type* ty, const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& incomings);
    [[nodiscard]] static AIRInstruction* createSelect(Type* ty, unsigned cond, unsigned tVal, unsigned fVal);
    [[nodiscard]] static AIRInstruction* createCall(Type* retTy, Function* callee, const SmallVector<unsigned, 8>& args);
    [[nodiscard]] static AIRInstruction* createConstantInt(Type* ty, int64_t val);
    [[nodiscard]] static AIRInstruction* createSwitch(Type* ty, unsigned cond, BasicBlock* defaultBB, const SmallVector<std::pair<int64_t, BasicBlock*>, 8>& cases);
    [[nodiscard]] static AIRInstruction* createExtractValue(Type* ty, unsigned agg, const SmallVector<unsigned, 4>& indices);
    [[nodiscard]] static AIRInstruction* createInsertValue(Type* ty, unsigned agg, unsigned val, const SmallVector<unsigned, 4>& indices);

    [[nodiscard]] AIROpcode getOpcode() const noexcept { return opcode_; }
    [[nodiscard]] Type* getType() const noexcept { return type_; }

    [[nodiscard]] unsigned getDestVReg() const noexcept { return destVReg_; }
    [[nodiscard]] bool hasResult() const noexcept;

    [[nodiscard]] unsigned getOperand(unsigned idx) const;
    [[nodiscard]] unsigned getNumOperands() const noexcept;
    [[nodiscard]] BasicBlock* getOperandBlock(unsigned idx) const;
    [[nodiscard]] Function* getCalledFunction() const noexcept;
    [[nodiscard]] ICmpCond getICmpCondition() const noexcept;
    [[nodiscard]] int64_t getConstantValue() const noexcept { return constantVal_; }
    [[nodiscard]] const SmallVector<unsigned, 4>& getStructIndices() const { return gepIndices_; }
    [[nodiscard]] BasicBlock* getSwitchDefault() const noexcept;
    [[nodiscard]] const SmallVector<std::pair<int64_t, BasicBlock*>, 8>& getSwitchCases() const;

    [[nodiscard]] const SmallVector<unsigned, 4>& getIndices() const;
    [[nodiscard]] const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& getPhiIncomings() const;

    [[nodiscard]] BasicBlock* getParent() const noexcept { return parent_; }
    void setParent(BasicBlock* bb) noexcept { parent_ = bb; }

    [[nodiscard]] AIRInstruction* getNext() const noexcept { return next_; }
    [[nodiscard]] AIRInstruction* getPrev() const noexcept { return prev_; }
    void setNext(AIRInstruction* n) noexcept { next_ = n; }
    void setPrev(AIRInstruction* p) noexcept { prev_ = p; }

    void setDestVReg(const unsigned vreg) noexcept { destVReg_ = vreg; }
    void replaceUse(unsigned oldVReg, unsigned newVReg);

    [[nodiscard]] std::string toString() const;

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
    SmallVector<std::pair<int64_t, BasicBlock*>, 8> switchCases_;
    int64_t constantVal_ = 0;
};

[[nodiscard]] const char* opcodeName(AIROpcode op);
[[nodiscard]] const char* icmpCondName(ICmpCond c);

} // namespace aurora

