#pragma once

namespace minic {

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct Expr : ASTNode {};
struct Stmt : ASTNode {};

} // namespace minic
