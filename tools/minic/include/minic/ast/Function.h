#pragma once

#include "minic/ast/Stmt.h"
#include "minic/ast/Type.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

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
