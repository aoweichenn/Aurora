#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

enum class CTypeKind { Void, Int, Long, Char };

struct CType {
    CTypeKind kind = CTypeKind::Long;

    [[nodiscard]] bool isVoid() const noexcept { return kind == CTypeKind::Void; }
};

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct Expr : ASTNode {};
struct Stmt : ASTNode {};

struct IntLitExpr : Expr {
    int64_t value;
    explicit IntLitExpr(int64_t v) : value(v) {}
};

struct VarExpr : Expr {
    std::string name;
    explicit VarExpr(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : Expr {
    enum Op {
        Add, Sub, Mul, Div, Rem,
        Eq, Ne, Lt, Le, Gt, Ge,
        BitAnd, BitOr, BitXor, Shl, Shr,
        LogAnd, LogOr
    };
    Op op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    BinaryExpr(Op o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

struct UnaryExpr : Expr {
    enum Op { Plus, Neg, LogicalNot, BitNot };
    Op op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(Op o, std::unique_ptr<Expr> e) : op(o), operand(std::move(e)) {}
};

struct AssignExpr : Expr {
    enum Op { Assign, AddAssign, SubAssign, MulAssign, DivAssign, RemAssign };
    Op op;
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(Op o, std::string n, std::unique_ptr<Expr> v)
        : op(o), name(std::move(n)), value(std::move(v)) {}
};

struct IncDecExpr : Expr {
    std::string name;
    bool increment;
    bool prefix;
    IncDecExpr(std::string n, bool inc, bool pre)
        : name(std::move(n)), increment(inc), prefix(pre) {}
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(std::string c, std::vector<std::unique_ptr<Expr>> a)
        : callee(std::move(c)), args(std::move(a)) {}
};

struct ConditionalExpr : Expr {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> trueExpr;
    std::unique_ptr<Expr> falseExpr;
    ConditionalExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> t, std::unique_ptr<Expr> f)
        : cond(std::move(c)), trueExpr(std::move(t)), falseExpr(std::move(f)) {}
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    explicit ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    explicit ReturnStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
};

struct DeclStmt : Stmt {
    struct Declarator {
        std::string name;
        std::unique_ptr<Expr> init;
        Declarator(std::string n, std::unique_ptr<Expr> i)
            : name(std::move(n)), init(std::move(i)) {}
    };
    CType type;
    std::vector<Declarator> declarators;
    DeclStmt(CType t, std::vector<Declarator> decls)
        : type(t), declarators(std::move(decls)) {}
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt;
    IfStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> e)
        : cond(std::move(c)), thenStmt(std::move(t)), elseStmt(std::move(e)) {}
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> b)
        : cond(std::move(c)), body(std::move(b)) {}
};

struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init;
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> step;
    std::unique_ptr<Stmt> body;
    ForStmt(std::unique_ptr<Stmt> i, std::unique_ptr<Expr> c,
            std::unique_ptr<Expr> s, std::unique_ptr<Stmt> b)
        : init(std::move(i)), cond(std::move(c)), step(std::move(s)), body(std::move(b)) {}
};

struct BreakStmt : Stmt {};
struct ContinueStmt : Stmt {};

struct Param {
    CType type;
    std::string name;
};

struct Function {
    CType returnType;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Stmt> body;

    Function(CType ret, std::string n, std::vector<Param> p, std::unique_ptr<Stmt> b)
        : returnType(ret), name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};

} // namespace minic
