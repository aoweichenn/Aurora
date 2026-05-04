#pragma once

#include "AST.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace minic {

class CodeGen {
public:
    explicit CodeGen();
    std::unique_ptr<aurora::Module> generate(const std::vector<Function>& functions);

private:
    struct Variable {
        CType type;
        unsigned pointerVReg;
    };

    struct LoopTargets {
        aurora::BasicBlock* breakTarget;
        aurora::BasicBlock* continueTarget;
    };

    std::unique_ptr<aurora::Module> module_;
    aurora::AIRBuilder* builder_;
    std::unordered_map<std::string, aurora::Function*> functionMap_;
    std::vector<std::unordered_map<std::string, Variable>> scopes_;
    std::vector<LoopTargets> loopStack_;
    CType currentReturnType_;
    unsigned ifCounter_ = 0;
    unsigned loopCounter_ = 0;
    unsigned conditionalCounter_ = 0;

    aurora::Type* toAirType(CType type, bool allowVoid = true) const;

    void genStmt(const Stmt& stmt);
    void genBlock(const BlockStmt& block, bool createScope = true);
    void genDeclStmt(const DeclStmt& stmt);
    void genReturnStmt(const ReturnStmt& stmt);
    void genIfStmt(const IfStmt& stmt);
    void genWhileStmt(const WhileStmt& stmt);
    void genForStmt(const ForStmt& stmt);
    void genExprStmt(const ExprStmt& stmt);
    void genBreakStmt();
    void genContinueStmt();

    unsigned genExpr(const Expr& node);

    unsigned genBinaryExpr(const BinaryExpr& be);
    unsigned genUnaryExpr(const UnaryExpr& ue);
    unsigned genAssignExpr(const AssignExpr& ae);
    unsigned genIncDecExpr(const IncDecExpr& ie);
    unsigned genCallExpr(const CallExpr& ce);
    unsigned genConditionalExpr(const ConditionalExpr& ce);
    unsigned genIntLitExpr(const IntLitExpr& ie) const;
    unsigned genVarExpr(const VarExpr& ve);
    unsigned genConditionValue(const Expr& expr);
    bool containsCall(const Expr& expr) const;

    Variable& findVariable(const std::string& name);
    void declareVariable(const std::string& name, CType type, const Expr* init);
    void pushScope();
    void popScope();
    bool currentBlockTerminated() const;
    void branchToIfOpen(aurora::BasicBlock* target);
};

} // namespace minic
