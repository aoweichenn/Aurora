#pragma once

#include "Lexer.h"
#include "AST.h"
#include <memory>
#include <string>
#include <vector>

namespace minic {

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::vector<Function> parseProgram();

private:
    Lexer& lexer_;
    Token current_;
    void advance();
    Token consume(TokenKind expected);
    bool match(TokenKind kind);

    Function parseFunction();
    std::unique_ptr<ASTNode> parseExpr();
    std::unique_ptr<ASTNode> parseCompare();
    std::unique_ptr<ASTNode> parseSum();
    std::unique_ptr<ASTNode> parseMul();
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parsePrimary();
};

} // namespace minic
