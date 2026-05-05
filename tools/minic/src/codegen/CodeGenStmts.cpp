#include "minic/codegen/CodeGen.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace minic {

void CodeGen::genStmt(const Stmt& stmt) {
    if (currentBlockTerminated())
        return;
    if (auto* block = dynamic_cast<const BlockStmt*>(&stmt)) {
        genBlock(*block);
    } else if (auto* decl = dynamic_cast<const DeclStmt*>(&stmt)) {
        genDeclStmt(*decl);
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt)) {
        genReturnStmt(*ret);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        genIfStmt(*ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        genWhileStmt(*whileStmt);
    } else if (auto* doWhileStmt = dynamic_cast<const DoWhileStmt*>(&stmt)) {
        genDoWhileStmt(*doWhileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(&stmt)) {
        genForStmt(*forStmt);
    } else if (auto* switchStmt = dynamic_cast<const SwitchStmt*>(&stmt)) {
        genSwitchStmt(*switchStmt);
    } else if (auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        genExprStmt(*exprStmt);
    } else if (dynamic_cast<const BreakStmt*>(&stmt)) {
        genBreakStmt();
    } else if (dynamic_cast<const ContinueStmt*>(&stmt)) {
        genContinueStmt();
    } else {
        throw std::runtime_error("Unknown statement node");
    }
}

void CodeGen::genBlock(const BlockStmt& block, bool createScope) {
    if (createScope)
        pushScope();
    for (const auto& statement : block.statements) {
        if (currentBlockTerminated())
            break;
        genStmt(*statement);
    }
    if (createScope)
        popScope();
}

void CodeGen::genReturnStmt(const ReturnStmt& stmt) {
    if (currentReturnType_.isVoid()) {
        builder_->createRetVoid();
        return;
    }
    unsigned value = stmt.value ? genExpr(*stmt.value) : builder_->createConstantInt(0);
    builder_->createRet(value);
}

void CodeGen::genIfStmt(const IfStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned ifId = ifCounter_++;
    auto* thenBlock = function->createBasicBlock("if.then." + std::to_string(ifId));
    auto* elseBlock = function->createBasicBlock("if.else." + std::to_string(ifId));
    auto* mergeBlock = function->createBasicBlock("if.end." + std::to_string(ifId));

    builder_->createCondBr(genConditionValue(*stmt.cond), thenBlock, elseBlock);

    builder_->setInsertPoint(thenBlock);
    genStmt(*stmt.thenStmt);
    branchToIfOpen(mergeBlock);

    builder_->setInsertPoint(elseBlock);
    if (stmt.elseStmt)
        genStmt(*stmt.elseStmt);
    branchToIfOpen(mergeBlock);

    builder_->setInsertPoint(mergeBlock);
}

void CodeGen::genWhileStmt(const WhileStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned loopId = loopCounter_++;
    auto* condBlock = function->createBasicBlock("while.cond." + std::to_string(loopId));
    auto* bodyBlock = function->createBasicBlock("while.body." + std::to_string(loopId));
    auto* endBlock = function->createBasicBlock("while.end." + std::to_string(loopId));

    branchToIfOpen(condBlock);
    builder_->setInsertPoint(condBlock);
    builder_->createCondBr(genConditionValue(*stmt.cond), bodyBlock, endBlock);

    builder_->setInsertPoint(bodyBlock);
    breakStack_.push_back(endBlock);
    continueStack_.push_back(condBlock);
    genStmt(*stmt.body);
    continueStack_.pop_back();
    breakStack_.pop_back();
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(endBlock);
}

void CodeGen::genDoWhileStmt(const DoWhileStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned loopId = loopCounter_++;
    auto* bodyBlock = function->createBasicBlock("do.body." + std::to_string(loopId));
    auto* condBlock = function->createBasicBlock("do.cond." + std::to_string(loopId));
    auto* endBlock = function->createBasicBlock("do.end." + std::to_string(loopId));

    branchToIfOpen(bodyBlock);

    builder_->setInsertPoint(bodyBlock);
    breakStack_.push_back(endBlock);
    continueStack_.push_back(condBlock);
    genStmt(*stmt.body);
    continueStack_.pop_back();
    breakStack_.pop_back();
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(condBlock);
    builder_->createCondBr(genConditionValue(*stmt.cond), bodyBlock, endBlock);

    builder_->setInsertPoint(endBlock);
}

void CodeGen::genForStmt(const ForStmt& stmt) {
    pushScope();
    if (stmt.init)
        genStmt(*stmt.init);

    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned loopId = loopCounter_++;
    auto* condBlock = function->createBasicBlock("for.cond." + std::to_string(loopId));
    auto* bodyBlock = function->createBasicBlock("for.body." + std::to_string(loopId));
    auto* stepBlock = function->createBasicBlock("for.step." + std::to_string(loopId));
    auto* endBlock = function->createBasicBlock("for.end." + std::to_string(loopId));

    branchToIfOpen(condBlock);

    builder_->setInsertPoint(condBlock);
    unsigned condValue = stmt.cond ? genConditionValue(*stmt.cond) : builder_->createConstantInt(1);
    builder_->createCondBr(condValue, bodyBlock, endBlock);

    builder_->setInsertPoint(bodyBlock);
    breakStack_.push_back(endBlock);
    continueStack_.push_back(stepBlock);
    genStmt(*stmt.body);
    continueStack_.pop_back();
    breakStack_.pop_back();
    branchToIfOpen(stepBlock);

    builder_->setInsertPoint(stepBlock);
    if (stmt.step)
        (void)genExpr(*stmt.step);
    branchToIfOpen(condBlock);

    builder_->setInsertPoint(endBlock);
    popScope();
}

void CodeGen::genSwitchStmt(const SwitchStmt& stmt) {
    auto* function = builder_->getInsertBlock()->getParent();
    const unsigned switchId = switchCounter_++;
    unsigned condValue = genExpr(*stmt.cond);
    auto* endBlock = function->createBasicBlock("switch.end." + std::to_string(switchId));

    if (stmt.sections.empty()) {
        branchToIfOpen(endBlock);
        builder_->setInsertPoint(endBlock);
        return;
    }

    std::vector<aurora::BasicBlock*> bodyBlocks;
    std::vector<size_t> caseSectionIndices;
    std::vector<aurora::BasicBlock*> caseTestBlocks;
    aurora::BasicBlock* defaultBlock = nullptr;

    for (size_t index = 0; index < stmt.sections.size(); ++index) {
        const bool isDefault = stmt.sections[index].value == nullptr;
        std::string name = isDefault ? "switch.default." : "switch.case.";
        name += std::to_string(switchId) + "." + std::to_string(index);
        auto* bodyBlock = function->createBasicBlock(name);
        bodyBlocks.push_back(bodyBlock);
        if (isDefault) {
            defaultBlock = bodyBlock;
        } else {
            caseSectionIndices.push_back(index);
            caseTestBlocks.push_back(function->createBasicBlock("switch.test." + std::to_string(switchId) + "." + std::to_string(index)));
        }
    }

    aurora::BasicBlock* firstDispatch = !caseTestBlocks.empty()
        ? caseTestBlocks.front()
        : (defaultBlock ? defaultBlock : endBlock);
    branchToIfOpen(firstDispatch);

    for (size_t caseIndex = 0; caseIndex < caseSectionIndices.size(); ++caseIndex) {
        const size_t sectionIndex = caseSectionIndices[caseIndex];
        builder_->setInsertPoint(caseTestBlocks[caseIndex]);
        unsigned caseValue = builder_->createConstantInt(evalConstantExpr(*stmt.sections[sectionIndex].value));
        unsigned cmp = builder_->createICmp(aurora::ICmpCond::EQ, condValue, caseValue);
        aurora::BasicBlock* falseTarget = caseIndex + 1 < caseTestBlocks.size()
            ? caseTestBlocks[caseIndex + 1]
            : (defaultBlock ? defaultBlock : endBlock);
        builder_->createCondBr(cmp, bodyBlocks[sectionIndex], falseTarget);
    }

    breakStack_.push_back(endBlock);
    for (size_t index = 0; index < stmt.sections.size(); ++index) {
        builder_->setInsertPoint(bodyBlocks[index]);
        for (const auto& statement : stmt.sections[index].statements) {
            if (currentBlockTerminated())
                break;
            genStmt(*statement);
        }
        aurora::BasicBlock* fallthroughTarget = index + 1 < bodyBlocks.size() ? bodyBlocks[index + 1] : endBlock;
        branchToIfOpen(fallthroughTarget);
    }
    breakStack_.pop_back();

    builder_->setInsertPoint(endBlock);
}

void CodeGen::genExprStmt(const ExprStmt& stmt) {
    if (stmt.expr)
        (void)genExpr(*stmt.expr);
}

void CodeGen::genBreakStmt() {
    if (breakStack_.empty())
        throw std::runtime_error("break used outside of a loop or switch");
    builder_->createBr(breakStack_.back());
}

void CodeGen::genContinueStmt() {
    if (continueStack_.empty())
        throw std::runtime_error("continue used outside of a loop");
    builder_->createBr(continueStack_.back());
}

} // namespace minic
