#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <sstream>

namespace aurora {

// Factory methods
AIRInstruction* AIRInstruction::createRet(const unsigned valVReg) {
    auto* i = new AIRInstruction(AIROpcode::Ret);
    i->destVReg_ = valVReg;
    if (valVReg != ~0U) i->operands_.push_back(valVReg);
    return i;
}

AIRInstruction* AIRInstruction::createBr(BasicBlock* target) {
    auto* i = new AIRInstruction(AIROpcode::Br);
    i->blockOperands_.push_back(target);
    return i;
}

AIRInstruction* AIRInstruction::createCondBr(const unsigned cond, BasicBlock* trueBB, BasicBlock* falseBB) {
    auto* i = new AIRInstruction(AIROpcode::CondBr);
    i->operands_.push_back(cond);
    i->blockOperands_.push_back(trueBB);
    i->blockOperands_.push_back(falseBB);
    return i;
}

AIRInstruction* AIRInstruction::createUnreachable() {
    return new AIRInstruction(AIROpcode::Unreachable);
}

AIRInstruction* AIRInstruction::createAdd(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::Add, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createSub(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::Sub, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createMul(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::Mul, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createUDiv(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::UDiv, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createSDiv(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::SDiv, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createURem(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::URem, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createSRem(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::SRem, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createAnd(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::And, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createOr(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::Or, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}

AIRInstruction* AIRInstruction::createXor(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::Xor, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createShl(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::Shl, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createLShr(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::LShr, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createAShr(Type* ty, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::AShr, ty);
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createICmp(Type* ty, const ICmpCond cond, const unsigned lhs, const unsigned rhs) {
    auto* i = new AIRInstruction(AIROpcode::ICmp, ty);
    i->cond_ = cond;
    i->operands_.push_back(lhs); i->operands_.push_back(rhs);
    return i;
}
AIRInstruction* AIRInstruction::createAlloca(Type* allocaTy) {
    auto* i = new AIRInstruction(AIROpcode::Alloca, Type::getPointerTy(allocaTy));
    return i;
}
AIRInstruction* AIRInstruction::createLoad(Type* ty, const unsigned ptr) {
    auto* i = new AIRInstruction(AIROpcode::Load, ty);
    i->operands_.push_back(ptr);
    return i;
}
AIRInstruction* AIRInstruction::createStore(const unsigned val, const unsigned ptr) {
    auto* i = new AIRInstruction(AIROpcode::Store);
    i->operands_.push_back(val);
    i->operands_.push_back(ptr);
    return i;
}
AIRInstruction* AIRInstruction::createGEP(Type* ty, const unsigned ptr, const SmallVector<unsigned, 4>& indices) {
    auto* i = new AIRInstruction(AIROpcode::GetElementPtr, ty);
    i->operands_.push_back(ptr);
    i->gepIndices_ = indices;
    return i;
}
AIRInstruction* AIRInstruction::createSExt(Type* dstTy, const unsigned src) {
    auto* i = new AIRInstruction(AIROpcode::SExt, dstTy);
    i->operands_.push_back(src);
    return i;
}
AIRInstruction* AIRInstruction::createZExt(Type* dstTy, const unsigned src) {
    auto* i = new AIRInstruction(AIROpcode::ZExt, dstTy);
    i->operands_.push_back(src);
    return i;
}
AIRInstruction* AIRInstruction::createTrunc(Type* dstTy, const unsigned src) {
    auto* i = new AIRInstruction(AIROpcode::Trunc, dstTy);
    i->operands_.push_back(src);
    return i;
}
AIRInstruction* AIRInstruction::createPhi(Type* ty, const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& incomings) {
    auto* i = new AIRInstruction(AIROpcode::Phi, ty);
    i->phiIncomings_ = incomings;
    return i;
}
AIRInstruction* AIRInstruction::createSelect(Type* ty, const unsigned cond, const unsigned tVal, const unsigned fVal) {
    auto* i = new AIRInstruction(AIROpcode::Select, ty);
    i->operands_.push_back(cond);
    i->operands_.push_back(tVal);
    i->operands_.push_back(fVal);
    return i;
}
AIRInstruction* AIRInstruction::createCall(Type* retTy, Function* callee, const SmallVector<unsigned, 8>& args) {
    auto* i = new AIRInstruction(AIROpcode::Call, retTy);
    i->callee_ = callee;
    i->operands_.clear();
    for (const auto& arg : args) i->operands_.push_back(arg);
    return i;
}
AIRInstruction* AIRInstruction::createConstantInt(Type* ty, const int64_t val) {
    auto* i = new AIRInstruction(AIROpcode::ConstantInt, ty);
    i->constantVal_ = val;
    return i;
}
AIRInstruction* AIRInstruction::createGlobalAddress(Type* ty, const char* globalName) {
    auto* i = new AIRInstruction(AIROpcode::GlobalAddress, ty);
    i->globalName_ = globalName;
    return i;
}
AIRInstruction* AIRInstruction::createSwitch(Type* ty, const unsigned cond, BasicBlock* defaultBB,
                                              const SmallVector<std::pair<int64_t, BasicBlock*>, 8>& cases) {
    auto* i = new AIRInstruction(AIROpcode::Switch, ty);
    i->operands_.push_back(cond);
    i->blockOperands_.push_back(defaultBB);
    i->switchCases_ = cases;
    return i;
}
AIRInstruction* AIRInstruction::createExtractValue(Type* ty, const unsigned agg, const SmallVector<unsigned, 4>& indices) {
    auto* i = new AIRInstruction(AIROpcode::ExtractValue, ty);
    i->operands_.push_back(agg);
    i->gepIndices_ = indices;
    return i;
}
AIRInstruction* AIRInstruction::createInsertValue(Type* ty, const unsigned agg, const unsigned val, const SmallVector<unsigned, 4>& indices) {
    auto* i = new AIRInstruction(AIROpcode::InsertValue, ty);
    i->operands_.push_back(agg);
    i->operands_.push_back(val);
    i->gepIndices_ = indices;
    return i;
}
BasicBlock* AIRInstruction::getSwitchDefault() const noexcept {
    return blockOperands_.empty() ? nullptr : blockOperands_[0];
}
const SmallVector<std::pair<int64_t, BasicBlock*>, 8>& AIRInstruction::getSwitchCases() const {
    return switchCases_;
}
bool AIRInstruction::hasResult() const noexcept {
    switch (opcode_) {
        case AIROpcode::Ret: case AIROpcode::Br: case AIROpcode::CondBr:
        case AIROpcode::Unreachable: case AIROpcode::Store:
            return false;
        default:
            return true;
    }
}
unsigned AIRInstruction::getOperand(const unsigned idx) const {
    if (opcode_ == AIROpcode::Phi) {
        if (idx % 2 == 0) {
            return ~0U; // block operand
        }
        return phiIncomings_[idx / 2].second;
    }
    if (opcode_ == AIROpcode::GetElementPtr && idx > 0)
        return gepIndices_[idx - 1];
    return operands_[idx];
}
unsigned AIRInstruction::getNumOperands() const noexcept {
    switch (opcode_) {
        case AIROpcode::Ret:
            return destVReg_ != ~0U ? 1 : 0;
        case AIROpcode::Br:       return 0;
        case AIROpcode::CondBr:   return 1;
        case AIROpcode::ICmp:     return 2;
        case AIROpcode::Load:     return 1;
        case AIROpcode::Alloca:   return 0;
        case AIROpcode::GetElementPtr: return static_cast<unsigned>(1 + gepIndices_.size());
        case AIROpcode::ConstantInt:  return 0;
        case AIROpcode::GlobalAddress: return 0;
        case AIROpcode::Switch:   return 1;
        case AIROpcode::ExtractValue: return 1;
        case AIROpcode::InsertValue:  return 2;
        case AIROpcode::Phi:      return static_cast<unsigned>(phiIncomings_.size() * 2);
        case AIROpcode::Call:     return static_cast<unsigned>(operands_.size());
        case AIROpcode::Store:    return 2;
        default:                  return 2;
    }
}
BasicBlock* AIRInstruction::getOperandBlock(const unsigned idx) const {
    if (opcode_ == AIROpcode::CondBr) {
        return idx == 0 ? blockOperands_[0] : blockOperands_[1];
    }
    if (opcode_ == AIROpcode::Br) {
        return blockOperands_[0];
    }
    if (opcode_ == AIROpcode::Phi) {
        return phiIncomings_[idx / 2].first;
    }
    return nullptr;
}
Function* AIRInstruction::getCalledFunction() const noexcept { return callee_; }
ICmpCond AIRInstruction::getICmpCondition() const noexcept { return cond_; }
const SmallVector<unsigned, 4>& AIRInstruction::getIndices() const { return gepIndices_; }
const SmallVector<std::pair<BasicBlock*, unsigned>, 4>& AIRInstruction::getPhiIncomings() const {
    return phiIncomings_;
}
void AIRInstruction::replaceUse(const unsigned oldVReg, const unsigned newVReg) {
    for (auto& op : operands_) {
        if (op == oldVReg) op = newVReg;
    }
    if (opcode_ == AIROpcode::Phi) {
        for (auto& inc : phiIncomings_) {
            if (inc.second == oldVReg) inc.second = newVReg;
        }
    }
}
std::string AIRInstruction::toString() const {
    std::ostringstream os;
    if (hasResult()) {
        os << "%" << destVReg_ << " = ";
    }
    os << opcodeName(opcode_);

    switch (opcode_) {
        case AIROpcode::Ret:
            if (destVReg_ != ~0U) os << " %" << destVReg_;
            break;
        case AIROpcode::Br:
            if (!blockOperands_.empty()) os << " &" << blockOperands_[0]->getName();
            break;
        case AIROpcode::CondBr:
            os << " %" << operands_[0] << ", &" << blockOperands_[0]->getName()
                    << ", &" << blockOperands_[1]->getName();
            break;
        case AIROpcode::ICmp:
            os << " " << icmpCondName(cond_) << " %" << operands_[0] << ", %" << operands_[1];
            break;
        case AIROpcode::Add: case AIROpcode::Sub: case AIROpcode::Mul:
        case AIROpcode::UDiv: case AIROpcode::SDiv: case AIROpcode::URem: case AIROpcode::SRem:
        case AIROpcode::And: case AIROpcode::Or: case AIROpcode::Xor:
        case AIROpcode::Shl: case AIROpcode::LShr: case AIROpcode::AShr:
            if (type_) os << " " << type_->toString();
            os << " %" << operands_[0] << ", %" << operands_[1];
            break;
        case AIROpcode::Alloca:
            if (type_) os << " " << type_->toString();
            break;
        case AIROpcode::Load:
            if (type_) os << " " << type_->toString();
            os << " %" << operands_[0];
            break;
        case AIROpcode::Store:
            os << " %" << operands_[0] << ", %" << operands_[1];
            break;
        case AIROpcode::GetElementPtr:
            os << " %" << operands_[0];
            for (const auto idx : gepIndices_) os << ", %" << idx;
            break;
        case AIROpcode::Phi:
            for (unsigned i = 0; i < phiIncomings_.size(); ++i) {
                if (i) os << ", ";
                os << "[&" << phiIncomings_[i].first->getName()
                        << ", %" << phiIncomings_[i].second << "]";
            }
            break;
        case AIROpcode::Call:
            os << " " << (callee_ ? callee_->getName() : "?");
            for (auto& op : operands_) os << " %" << op;
            break;
        case AIROpcode::GlobalAddress:
            os << " @" << (globalName_ ? globalName_ : "?");
            break;
        default:
            break;
    }
    return os.str();
}
AIRInstruction::AIRInstruction(const AIROpcode op, Type* ty)
    : opcode_(op), type_(ty), destVReg_(~0U),
      parent_(nullptr), next_(nullptr), prev_(nullptr),
      cond_(ICmpCond::EQ), callee_(nullptr) {}
const char* opcodeName(const AIROpcode op) {
    switch (op) {
        case AIROpcode::Ret:          return "ret";
        case AIROpcode::Br:           return "br";
        case AIROpcode::CondBr:       return "condbr";
        case AIROpcode::Unreachable:  return "unreachable";
        case AIROpcode::Add:          return "add";
        case AIROpcode::Sub:          return "sub";
        case AIROpcode::Mul:          return "mul";
        case AIROpcode::UDiv:         return "udiv";
        case AIROpcode::SDiv:         return "sdiv";
        case AIROpcode::URem:         return "urem";
        case AIROpcode::SRem:         return "srem";
        case AIROpcode::And:          return "and";
        case AIROpcode::Or:           return "or";
        case AIROpcode::Xor:          return "xor";
        case AIROpcode::Shl:          return "shl";
        case AIROpcode::LShr:         return "lshr";
        case AIROpcode::AShr:         return "ashr";
        case AIROpcode::ICmp:         return "icmp";
        case AIROpcode::FCmp:         return "fcmp";
        case AIROpcode::Alloca:       return "alloca";
        case AIROpcode::Load:         return "load";
        case AIROpcode::Store:        return "store";
        case AIROpcode::GetElementPtr: return "gep";
        case AIROpcode::SExt:         return "sext";
        case AIROpcode::ZExt:         return "zext";
        case AIROpcode::Trunc:        return "trunc";
        case AIROpcode::FpToSi:       return "fptosi";
        case AIROpcode::SiToFp:       return "sitofp";
        case AIROpcode::BitCast:      return "bitcast";
        case AIROpcode::Phi:          return "phi";
        case AIROpcode::Select:       return "select";
        case AIROpcode::Call:         return "call";
        case AIROpcode::ConstantInt:  return "const";
        case AIROpcode::Switch:       return "switch";
        case AIROpcode::GlobalAddress: return "globaladdr";
        case AIROpcode::ExtractValue: return "extractvalue";
        case AIROpcode::InsertValue:  return "insertvalue";
    }
    return "unknown";
}
const char* icmpCondName(const ICmpCond c) {
    switch (c) {
        case ICmpCond::EQ:  return "eq";
        case ICmpCond::NE:  return "ne";
        case ICmpCond::UGT: return "ugt";
        case ICmpCond::UGE: return "uge";
        case ICmpCond::ULT: return "ult";
        case ICmpCond::ULE: return "ule";
        case ICmpCond::SGT: return "sgt";
        case ICmpCond::SGE: return "sge";
        case ICmpCond::SLT: return "slt";
        case ICmpCond::SLE: return "sle";
    }
    return "?";
}

} // namespace aurora
