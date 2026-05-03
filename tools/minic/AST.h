#pragma once

#include <memory>
#include <string>
#include <vector>

namespace minic {

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct IntLitExpr : ASTNode {
    int64_t value;
    explicit IntLitExpr(int64_t v) : value(v) {}
};

struct VarExpr : ASTNode {
    std::string name;
    explicit VarExpr(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : ASTNode {
    enum Op { Add, Sub, Mul, Div, Eq, Ne, Lt, Le, Gt, Ge };
    Op op;
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
    BinaryExpr(Op o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct NegExpr : ASTNode {
    std::unique_ptr<ASTNode> operand;
    explicit NegExpr(std::unique_ptr<ASTNode> o) : operand(std::move(o)) {}
};

struct IfExpr : ASTNode {
    std::unique_ptr<ASTNode> cond;
    std::unique_ptr<ASTNode> trueExpr;
    std::unique_ptr<ASTNode> falseExpr;
    IfExpr(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> f)
        : cond(std::move(c)), trueExpr(std::move(t)), falseExpr(std::move(f)) {}
};

struct Function {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<ASTNode> body;
    Function(std::string n, std::vector<std::string> p, std::unique_ptr<ASTNode> b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};

} // namespace minic
