#include "CodeGen.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/BasicBlock.h"
#include <stdexcept>

namespace minic {

CodeGen::CodeGen() : builder_(nullptr) {}

std::unique_ptr<aurora::Module> CodeGen::generate(const std::vector<Function>& functions) {
    module_ = std::make_unique<aurora::Module>("minic");

    for (const auto& fn : functions) {
        // Build parameter types (all i64)
        aurora::SmallVector<aurora::Type*, 8> paramTypes;
        for (size_t i = 0; i < fn.params.size(); ++i)
            paramTypes.push_back(aurora::Type::getInt64Ty());

        auto* fnTy = new aurora::FunctionType(aurora::Type::getInt64Ty(), paramTypes);
        auto* airFn = module_->createFunction(fnTy, fn.name);
        auto* entryBB = airFn->getEntryBlock();

        // Set up variable mapping: parameter names -> vreg indices
        varMap_.clear();
        for (size_t i = 0; i < fn.params.size(); ++i)
            varMap_[fn.params[i]] = static_cast<unsigned>(i);

        // Build the body
        aurora::AIRBuilder builder(entryBB);
        builder_ = &builder;
        unsigned resultVReg = genExpr(*fn.body);
        builder_->createRet(resultVReg);
    }

    return std::move(module_);
}

unsigned CodeGen::genExpr(const ASTNode& node) {
    if (auto* be = dynamic_cast<const BinaryExpr*>(&node))
        return genBinaryExpr(*be);
    if (auto* ne = dynamic_cast<const NegExpr*>(&node))
        return genNegExpr(*ne);
    if (auto* ie = dynamic_cast<const IfExpr*>(&node))
        return genIfExpr(*ie);
    if (auto* il = dynamic_cast<const IntLitExpr*>(&node))
        return genIntLitExpr(*il);
    if (auto* ve = dynamic_cast<const VarExpr*>(&node))
        return genVarExpr(*ve);
    throw std::runtime_error("Unknown AST node type");
}

unsigned CodeGen::genIntLitExpr(const IntLitExpr& ie) {
    return builder_->createConstantInt(ie.value);
}

unsigned CodeGen::genVarExpr(const VarExpr& ve) {
    auto it = varMap_.find(ve.name);
    if (it != varMap_.end())
        return it->second;
    throw std::runtime_error("Undefined variable: " + ve.name);
}

unsigned CodeGen::genBinaryExpr(const BinaryExpr& be) {
    unsigned lhs = genExpr(*be.lhs);
    unsigned rhs = genExpr(*be.rhs);
    auto* i64 = aurora::Type::getInt64Ty();

    switch (be.op) {
    case BinaryExpr::Add: return builder_->createAdd(i64, lhs, rhs);
    case BinaryExpr::Sub: return builder_->createSub(i64, lhs, rhs);
    case BinaryExpr::Mul: return builder_->createMul(i64, lhs, rhs);
    case BinaryExpr::Div: {
        // For division, use SDiv
        // The x86 IDIV divides RDX:RAX by the operand
        // We need to sign-extend RAX into RDX:RAX first (CQO)
        return builder_->createSDiv(i64, lhs, rhs);
    }
    case BinaryExpr::Eq:  return builder_->createICmp(aurora::ICmpCond::EQ, lhs, rhs);
    case BinaryExpr::Ne:  return builder_->createICmp(aurora::ICmpCond::NE, lhs, rhs);
    case BinaryExpr::Lt:  return builder_->createICmp(aurora::ICmpCond::SLT, lhs, rhs);
    case BinaryExpr::Le:  return builder_->createICmp(aurora::ICmpCond::SLE, lhs, rhs);
    case BinaryExpr::Gt:  return builder_->createICmp(aurora::ICmpCond::SGT, lhs, rhs);
    case BinaryExpr::Ge:  return builder_->createICmp(aurora::ICmpCond::SGE, lhs, rhs);
    }
    throw std::runtime_error("Unknown binary op");
}

unsigned CodeGen::genNegExpr(const NegExpr& ne) {
    unsigned val = genExpr(*ne.operand);
    unsigned zero = builder_->createConstantInt(0);
    return builder_->createSub(aurora::Type::getInt64Ty(), zero, val);
}

unsigned CodeGen::genIfExpr(const IfExpr& ie) {
    // if cond then true_expr else false_expr
    // Strategy: Use basic blocks
    // 1. Evaluate cond in current block
    // 2. CondBr to true_bb or false_bb
    // 3. In true_bb: evaluate true_expr, br to merge_bb
    // 4. In false_bb: evaluate false_expr, br to merge_bb
    // 5. In merge_bb: phi of true_val and false_val, return phi result

    auto* fn = builder_->getInsertBlock()->getParent();
    auto* trueBB = fn->createBasicBlock("if.true");
    auto* falseBB = fn->createBasicBlock("if.false");
    auto* mergeBB = fn->createBasicBlock("if.merge");

    // 1. Evaluate condition and branch
    unsigned condVReg = genExpr(*ie.cond);
    builder_->createCondBr(condVReg, trueBB, falseBB);

    // 2. True block
    builder_->setInsertPoint(trueBB);
    unsigned trueVal = genExpr(*ie.trueExpr);
    builder_->createBr(mergeBB);

    // 3. False block
    builder_->setInsertPoint(falseBB);
    unsigned falseVal = genExpr(*ie.falseExpr);
    builder_->createBr(mergeBB);

    // 4. Merge block with PHI
    builder_->setInsertPoint(mergeBB);
    aurora::SmallVector<std::pair<aurora::BasicBlock*, unsigned>, 4> incomings;
    incomings.push_back({trueBB, trueVal});
    incomings.push_back({falseBB, falseVal});
    unsigned phi = builder_->createPhi(aurora::Type::getInt64Ty(), incomings);

    return phi;
}

} // namespace minic
