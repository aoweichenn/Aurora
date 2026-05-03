#include "Parser.h"
#include <iostream>
#include <stdexcept>

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
    while (current_.kind == TokenKind::Fn) {
        functions.push_back(parseFunction());
    }
    consume(TokenKind::Eof);
    return functions;
}

Function Parser::parseFunction() {
    consume(TokenKind::Fn);
    std::string name = consume(TokenKind::Ident).lexeme;
    consume(TokenKind::LParen);

    std::vector<std::string> params;
    if (current_.kind != TokenKind::RParen) {
        params.push_back(consume(TokenKind::Ident).lexeme);
        while (current_.kind == TokenKind::Comma) {
            consume(TokenKind::Comma);
            params.push_back(consume(TokenKind::Ident).lexeme);
        }
    }
    consume(TokenKind::RParen);
    consume(TokenKind::Assign);
    auto body = parseExpr();

    return Function(name, std::move(params), std::move(body));
}

std::unique_ptr<ASTNode> Parser::parseExpr() {
    return parseCompare();
}

std::unique_ptr<ASTNode> Parser::parseCompare() {
    auto left = parseSum();
    while (true) {
        BinaryExpr::Op op;
        if (current_.kind == TokenKind::EqEq) { advance(); op = BinaryExpr::Eq; }
        else if (current_.kind == TokenKind::Neq) { advance(); op = BinaryExpr::Ne; }
        else if (current_.kind == TokenKind::Lt)  { advance(); op = BinaryExpr::Lt; }
        else if (current_.kind == TokenKind::Le)  { advance(); op = BinaryExpr::Le; }
        else if (current_.kind == TokenKind::Gt)  { advance(); op = BinaryExpr::Gt; }
        else if (current_.kind == TokenKind::Ge)  { advance(); op = BinaryExpr::Ge; }
        else break;
        auto right = parseSum();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseSum() {
    auto left = parseMul();
    while (true) {
        if (current_.kind == TokenKind::Plus) {
            advance();
            auto right = parseMul();
            left = std::make_unique<BinaryExpr>(BinaryExpr::Add, std::move(left), std::move(right));
        } else if (current_.kind == TokenKind::Minus) {
            advance();
            auto right = parseMul();
            left = std::make_unique<BinaryExpr>(BinaryExpr::Sub, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseMul() {
    auto left = parseUnary();
    while (true) {
        if (current_.kind == TokenKind::Star) {
            advance();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpr>(BinaryExpr::Mul, std::move(left), std::move(right));
        } else if (current_.kind == TokenKind::Slash) {
            advance();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpr>(BinaryExpr::Div, std::move(left), std::move(right));
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnary() {
    if (current_.kind == TokenKind::Minus) {
        advance();
        auto operand = parseUnary();
        return std::make_unique<NegExpr>(std::move(operand));
    }
    return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    if (current_.kind == TokenKind::IntLit) {
        int64_t value = current_.intValue;
        advance();
        return std::make_unique<IntLitExpr>(value);
    }
    if (current_.kind == TokenKind::Ident) {
        std::string name = current_.lexeme;
        advance();
        return std::make_unique<VarExpr>(name);
    }
    if (current_.kind == TokenKind::If) {
        consume(TokenKind::If);
        auto cond = parseExpr();
        consume(TokenKind::Then);
        auto trueExpr = parseExpr();
        consume(TokenKind::Else);
        auto falseExpr = parseExpr();
        return std::make_unique<IfExpr>(std::move(cond), std::move(trueExpr), std::move(falseExpr));
    }
    if (current_.kind == TokenKind::LParen) {
        consume(TokenKind::LParen);
        auto expr = parseExpr();
        consume(TokenKind::RParen);
        return expr;
    }
    throw std::runtime_error(
        std::string("Unexpected token: ") + tokenName(current_.kind) +
        " ('" + current_.lexeme + "')");
}

} // namespace minic
