#include "minic/parse/Parser.h"
#include <stdexcept>
#include <utility>

namespace minic {

Parser::Parser(Lexer& lexer) : lexer_(lexer), current_(lexer.next()) {}

void Parser::advance() {
    current_ = lexer_.next();
}

Token Parser::consume(TokenKind expected) {
    if (current_.kind != expected) {
        throw std::runtime_error(
            std::string("Expected ") + tokenName(expected) +
            " but got " + tokenName(current_.kind) +
            " ('" + current_.lexeme + "')");
    }
    Token tok = current_;
    advance();
    return tok;
}

bool Parser::match(TokenKind kind) {
    if (current_.kind == kind) {
        advance();
        return true;
    }
    return false;
}

Program Parser::parseProgram() {
    Program program;
    while (current_.kind != TokenKind::Eof) {
        if (match(TokenKind::Typedef)) {
            parseTypedefDecl();
            continue;
        }
        if (current_.kind == TokenKind::StaticAssert) {
            parseStaticAssertDecl();
            continue;
        }
        if (current_.kind == TokenKind::Enum) {
            CType enumType = parseType();
            if (match(TokenKind::Semicolon))
                continue;
            parseTopLevelDecl(program, enumType, false);
            continue;
        }
        if (current_.kind == TokenKind::Fn) {
            program.functions.push_back(parseLegacyFunction());
            continue;
        }
        const bool isExtern = current_.kind == TokenKind::Extern;
        CType baseType = parseBaseType();
        parseTopLevelDecl(program, baseType, isExtern);
    }
    return program;
}

Function Parser::parseFunction() {
    if (current_.kind == TokenKind::Fn)
        return parseLegacyFunction();

    CType returnType = parseType();
    return parseFunctionRest(returnType);
}

Function Parser::parseFunctionRest(CType returnType) {
    std::string name = consume(TokenKind::Ident).lexeme;
    return parseFunctionRest(returnType, std::move(name));
}

Function Parser::parseFunctionRest(CType returnType, std::string name) {
    consume(TokenKind::LParen);
    std::vector<Param> params = parseParamList();
    consume(TokenKind::RParen);
    if (match(TokenKind::Semicolon))
        return Function(returnType, std::move(name), std::move(params), nullptr);
    auto body = parseBlock();
    return Function(returnType, std::move(name), std::move(params), std::move(body));
}

void Parser::parseTopLevelDecl(Program& program, CType baseType, bool isExtern) {
    do {
        CType type = parsePointerSuffix(baseType);
        std::string name = consume(TokenKind::Ident).lexeme;
        if (current_.kind == TokenKind::LParen) {
            program.functions.push_back(parseFunctionRest(type, std::move(name)));
            return;
        }

        type = parseArraySuffix(type);
        if (type.isVoid())
            throw std::runtime_error("Global variable cannot have void type: " + name);

        std::unique_ptr<Expr> init;
        if (match(TokenKind::Assign)) {
            if (isExtern)
                throw std::runtime_error("Extern global declaration cannot have an initializer: " + name);
            init = parseInitializer();
        }
        program.globals.emplace_back(type, std::move(name), std::move(init), isExtern);
    } while (match(TokenKind::Comma));

    consume(TokenKind::Semicolon);
}

Function Parser::parseLegacyFunction() {
    consume(TokenKind::Fn);
    std::string name = consume(TokenKind::Ident).lexeme;
    consume(TokenKind::LParen);

    std::vector<Param> params;
    if (current_.kind != TokenKind::RParen) {
        params.push_back({CType{CTypeKind::Long}, consume(TokenKind::Ident).lexeme});
        while (match(TokenKind::Comma))
            params.push_back({CType{CTypeKind::Long}, consume(TokenKind::Ident).lexeme});
    }
    consume(TokenKind::RParen);
    consume(TokenKind::Assign);

    auto block = std::make_unique<BlockStmt>();
    block->statements.push_back(std::make_unique<ReturnStmt>(parseExpr()));
    (void)match(TokenKind::Semicolon);
    return Function(CType{CTypeKind::Long}, std::move(name), std::move(params), std::move(block));
}

} // namespace minic
