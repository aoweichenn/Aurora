#pragma once

#include "AST.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include <memory>
#include <unordered_map>

namespace minic {

class CodeGen {
public:
    explicit CodeGen();
    std::unique_ptr<aurora::Module> generate(const std::vector<Function>& functions);

private:
    std::unique_ptr<aurora::Module> module_;
    aurora::AIRBuilder* builder_;
    std::unordered_map<std::string, unsigned> varMap_;
    unsigned ifCounter_ = 0;

    unsigned genExpr(const ASTNode& node, aurora::BasicBlock* trueBB, aurora::BasicBlock* falseBB);
    unsigned genExpr(const ASTNode& node);

    unsigned genBinaryExpr(const BinaryExpr& be);
    unsigned genNegExpr(const NegExpr& ne);
    unsigned genIfExpr(const IfExpr& ie);
    unsigned genIntLitExpr(const IntLitExpr& ie) const;
    unsigned genVarExpr(const VarExpr& ve);
};

} // namespace minic
