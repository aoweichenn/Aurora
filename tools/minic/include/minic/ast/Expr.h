#pragma once

#include "minic/ast/Node.h"
#include "minic/ast/Type.h"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

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

struct CastExpr : Expr {
    CType targetType;
    std::unique_ptr<Expr> operand;
    CastExpr(CType t, std::unique_ptr<Expr> e)
        : targetType(t), operand(std::move(e)) {}
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

struct AlignofExpr : Expr {
    CType type;
    explicit AlignofExpr(CType t) : type(t) {}
};

struct InitListExpr : Expr {
    std::vector<std::unique_ptr<Expr>> values;
    explicit InitListExpr(std::vector<std::unique_ptr<Expr>> v)
        : values(std::move(v)) {}
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

} // namespace minic
