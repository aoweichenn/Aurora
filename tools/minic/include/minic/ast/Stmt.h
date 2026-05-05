#pragma once

#include "minic/ast/Expr.h"
#include "minic/ast/Node.h"
#include "minic/ast/Type.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

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

} // namespace minic
