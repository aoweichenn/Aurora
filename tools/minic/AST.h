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
    unsigned pointerDepth = 0;
    uint64_t arraySize = 0;

    [[nodiscard]] bool isVoid() const noexcept { return kind == CTypeKind::Void && pointerDepth == 0 && arraySize == 0; }
    [[nodiscard]] bool isPointerLike() const noexcept { return pointerDepth > 0 || arraySize > 0; }

    [[nodiscard]] CType decayArray() const noexcept {
        CType result = *this;
        if (result.arraySize > 0) {
            result.arraySize = 0;
            ++result.pointerDepth;
        }
        return result;
    }

    [[nodiscard]] CType pointee() const noexcept {
        CType result = *this;
        if (result.arraySize > 0) {
            result.arraySize = 0;
            return result;
        }
        if (result.pointerDepth > 0)
            --result.pointerDepth;
        return result;
    }
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
    enum Op { Plus, Neg, LogicalNot, BitNot, AddressOf, Deref };
    Op op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(Op o, std::unique_ptr<Expr> e) : op(o), operand(std::move(e)) {}
};

struct AssignExpr : Expr {
    enum Op {
        Assign, AddAssign, SubAssign, MulAssign, DivAssign, RemAssign,
        BitAndAssign, BitOrAssign, BitXorAssign, ShlAssign, ShrAssign
    };
    Op op;
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
    AssignExpr(Op o, std::unique_ptr<Expr> t, std::unique_ptr<Expr> v)
        : op(o), target(std::move(t)), value(std::move(v)) {}
};

struct IncDecExpr : Expr {
    std::unique_ptr<Expr> target;
    bool increment;
    bool prefix;
    IncDecExpr(std::unique_ptr<Expr> t, bool inc, bool pre)
        : target(std::move(t)), increment(inc), prefix(pre) {}
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> b, std::unique_ptr<Expr> i)
        : base(std::move(b)), index(std::move(i)) {}
};

struct SizeofExpr : Expr {
    CType type;
    std::unique_ptr<Expr> expr;
    explicit SizeofExpr(CType t) : type(t) {}
    explicit SizeofExpr(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

struct CommaExpr : Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    CommaExpr(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : lhs(std::move(l)), rhs(std::move(r)) {}
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
        CType type;
        std::string name;
        std::unique_ptr<Expr> init;
        Declarator(CType t, std::string n, std::unique_ptr<Expr> i)
            : type(t), name(std::move(n)), init(std::move(i)) {}
    };
    std::vector<Declarator> declarators;
    explicit DeclStmt(std::vector<Declarator> decls)
        : declarators(std::move(decls)) {}
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

struct DoWhileStmt : Stmt {
    std::unique_ptr<Stmt> body;
    std::unique_ptr<Expr> cond;
    DoWhileStmt(std::unique_ptr<Stmt> b, std::unique_ptr<Expr> c)
        : body(std::move(b)), cond(std::move(c)) {}
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

struct SwitchStmt : Stmt {
    struct Section {
        std::unique_ptr<Expr> value;
        std::vector<std::unique_ptr<Stmt>> statements;
        Section(std::unique_ptr<Expr> v, std::vector<std::unique_ptr<Stmt>> s)
            : value(std::move(v)), statements(std::move(s)) {}
    };
    std::unique_ptr<Expr> cond;
    std::vector<Section> sections;
    SwitchStmt(std::unique_ptr<Expr> c, std::vector<Section> s)
        : cond(std::move(c)), sections(std::move(s)) {}
};

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
