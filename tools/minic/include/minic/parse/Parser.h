#pragma once

#include "minic/lex/Lexer.h"
#include "minic/ast/AST.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

class Parser {
public:
    explicit Parser(Lexer& lexer);
    Program parseProgram();

private:
    Lexer& lexer_;
    Token current_;
    std::unordered_map<std::string, CType> typedefs_;
    std::unordered_map<std::string, int64_t> enumConstants_;
    std::unordered_map<std::string, std::shared_ptr<CStructInfo>> structs_;
    void advance();
    Token consume(TokenKind expected);
    bool match(TokenKind kind);

    Function parseFunction();
    Function parseFunctionRest(CType returnType);
    Function parseFunctionRest(CType returnType, std::string name);
    Function parseLegacyFunction();
    void parseTopLevelDecl(Program& program, CType baseType, bool isExtern);
    CType parseBaseType();
    CType parseType();
    CType parsePointerSuffix(CType type);
    CType parseArraySuffix(CType type);
    CType parseParamArraySuffix(CType type);
    bool isTypeToken(TokenKind kind) const;
    bool isTypeQualifier(TokenKind kind) const;
    void consumeTypeQualifiers();
    void parseTypedefDecl();
    void parseStaticAssertDecl();
    void parseEnumBody();
    CType parseStructSpecifier();
    std::vector<CField> parseStructFields();
    int64_t evalConstantExpr(const Expr& expr) const;

    std::vector<Param> parseParamList();
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseDeclStmt(bool consumeSemicolon = true);
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseDoWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseSwitchStmt();
    std::unique_ptr<Stmt> parseExprStmt();
    std::unique_ptr<Expr> parseInitializer();

    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseComma();
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
