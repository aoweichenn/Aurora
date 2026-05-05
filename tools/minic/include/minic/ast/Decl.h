#pragma once

#include "minic/ast/Expr.h"
#include "minic/ast/Function.h"
#include "minic/ast/Type.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

struct GlobalDecl {
    CType type;
    std::string name;
    std::unique_ptr<Expr> init;
    bool isExtern = false;

    GlobalDecl(CType t, std::string n, std::unique_ptr<Expr> i, bool ext)
        : type(t), name(std::move(n)), init(std::move(i)), isExtern(ext) {}
};

struct Program {
    std::vector<GlobalDecl> globals;
    std::vector<Function> functions;
};

} // namespace minic
