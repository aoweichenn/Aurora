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

std::vector<Function> Parser::parseProgram() {
    std::vector<Function> functions;
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
            functions.push_back(parseFunctionRest(enumType));
            continue;
        }
        functions.push_back(parseFunction());
    }
    return functions;
}

Function Parser::parseFunction() {
    if (current_.kind == TokenKind::Fn)
        return parseLegacyFunction();

    CType returnType = parseType();
    return parseFunctionRest(returnType);
}

Function Parser::parseFunctionRest(CType returnType) {
    std::string name = consume(TokenKind::Ident).lexeme;
    consume(TokenKind::LParen);
    std::vector<Param> params = parseParamList();
    consume(TokenKind::RParen);
    auto body = parseBlock();
    return Function(returnType, std::move(name), std::move(params), std::move(body));
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
