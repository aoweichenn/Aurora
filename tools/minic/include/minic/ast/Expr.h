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

struct MemberExpr : Expr {
    std::unique_ptr<Expr> base;
    std::string field;
    bool viaPointer;
    MemberExpr(std::unique_ptr<Expr> b, std::string f, bool ptr)
        : base(std::move(b)), field(std::move(f)), viaPointer(ptr) {}
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
    struct Designator {
        enum Kind { None, Index, Field } kind = None;
        struct Part {
            Kind kind = None;
            uint64_t index = 0;
            std::string field;

            explicit Part(uint64_t i) : kind(Index), index(i) {}
            explicit Part(std::string f) : kind(Field), field(std::move(f)) {}
        };

        uint64_t index = 0;
        std::string field;
        std::vector<Part> parts;

        Designator() = default;
        explicit Designator(uint64_t i) : kind(Index), index(i), parts{Part(i)} {}
        explicit Designator(std::string f) : kind(Field), field(f), parts{Part(std::move(f))} {}
        explicit Designator(std::vector<Part> p) : parts(std::move(p)) {
            if (!parts.empty()) {
                kind = parts.front().kind;
                index = parts.front().index;
                field = parts.front().field;
            }
        }

        [[nodiscard]] bool empty() const noexcept { return parts.empty(); }
        [[nodiscard]] const Part& first() const noexcept { return parts.front(); }
    };

    struct Entry {
        Designator designator;
        std::unique_ptr<Expr> value;
        Entry(Designator d, std::unique_ptr<Expr> v)
            : designator(std::move(d)), value(std::move(v)) {}
    };

    std::vector<Entry> entries;
    explicit InitListExpr(std::vector<Entry> e)
        : entries(std::move(e)) {}
};

struct CompoundLiteralExpr : Expr {
    CType type;
    std::unique_ptr<InitListExpr> init;
    CompoundLiteralExpr(CType t, std::unique_ptr<InitListExpr> i)
        : type(std::move(t)), init(std::move(i)) {}
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
