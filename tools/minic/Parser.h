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
    Function parseLegacyFunction();
    CType parseType();
    bool isTypeToken(TokenKind kind) const;

    std::vector<Param> parseParamList();
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseDeclStmt(bool consumeSemicolon = true);
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseExprStmt();

    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseConditional();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseBitOr();
    std::unique_ptr<Expr> parseBitXor();
    std::unique_ptr<Expr> parseBitAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseShift();
    std::unique_ptr<Expr> parseSum();
    std::unique_ptr<Expr> parseMul();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
};

} // namespace minic
