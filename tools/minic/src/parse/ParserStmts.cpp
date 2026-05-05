#include "minic/parse/Parser.h"
#include <stdexcept>
#include <utility>

namespace minic {

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (current_.kind == TokenKind::LBrace)
        return parseBlock();
    if (isTypeToken(current_.kind))
        return parseDeclStmt();
    if (match(TokenKind::Typedef)) {
        parseTypedefDecl();
        return std::make_unique<ExprStmt>(nullptr);
    }
    if (current_.kind == TokenKind::StaticAssert) {
        parseStaticAssertDecl();
        return std::make_unique<ExprStmt>(nullptr);
    }
    if (current_.kind == TokenKind::Return)
        return parseReturnStmt();
    if (current_.kind == TokenKind::If)
        return parseIfStmt();
    if (current_.kind == TokenKind::While)
        return parseWhileStmt();
    if (current_.kind == TokenKind::Do)
        return parseDoWhileStmt();
    if (current_.kind == TokenKind::For)
        return parseForStmt();
    if (current_.kind == TokenKind::Switch)
        return parseSwitchStmt();
    if (current_.kind == TokenKind::Case || current_.kind == TokenKind::Default)
        throw std::runtime_error("case/default label used outside of switch");
    if (match(TokenKind::Break)) {
        consume(TokenKind::Semicolon);
        return std::make_unique<BreakStmt>();
    }
    if (match(TokenKind::Continue)) {
        consume(TokenKind::Semicolon);
        return std::make_unique<ContinueStmt>();
    }
    return parseExprStmt();
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    consume(TokenKind::LBrace);
    auto block = std::make_unique<BlockStmt>();
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof)
        block->statements.push_back(parseStmt());
    consume(TokenKind::RBrace);
    return block;
}

std::unique_ptr<Stmt> Parser::parseDeclStmt(bool consumeSemicolon) {
    CType baseType = parseBaseType();
    if (current_.kind == TokenKind::Semicolon) {
        if (consumeSemicolon)
            consume(TokenKind::Semicolon);
        return std::make_unique<DeclStmt>(std::vector<DeclStmt::Declarator>{});
    }

    std::vector<DeclStmt::Declarator> declarators;
    do {
        CType type = parsePointerSuffix(baseType);
        std::string name = consume(TokenKind::Ident).lexeme;
        type = parseArraySuffix(type);
        if (type.isVoid())
            throw std::runtime_error("Variables cannot have void type");
        std::unique_ptr<Expr> init;
        if (match(TokenKind::Assign))
            init = parseInitializer();
        declarators.emplace_back(type, std::move(name), std::move(init));
    } while (match(TokenKind::Comma));

    if (consumeSemicolon)
        consume(TokenKind::Semicolon);
    return std::make_unique<DeclStmt>(std::move(declarators));
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    consume(TokenKind::Return);
    std::unique_ptr<Expr> value;
    if (current_.kind != TokenKind::Semicolon)
        value = parseExpr();
    consume(TokenKind::Semicolon);
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
    consume(TokenKind::If);
    consume(TokenKind::LParen);
    auto cond = parseExpr();
    consume(TokenKind::RParen);
    auto thenStmt = parseStmt();
    std::unique_ptr<Stmt> elseStmt;
    if (match(TokenKind::Else))
        elseStmt = parseStmt();
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), std::move(elseStmt));
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    consume(TokenKind::While);
    consume(TokenKind::LParen);
    auto cond = parseExpr();
    consume(TokenKind::RParen);
    return std::make_unique<WhileStmt>(std::move(cond), parseStmt());
}

std::unique_ptr<Stmt> Parser::parseDoWhileStmt() {
    consume(TokenKind::Do);
    auto body = parseStmt();
    consume(TokenKind::While);
    consume(TokenKind::LParen);
    auto cond = parseExpr();
    consume(TokenKind::RParen);
    consume(TokenKind::Semicolon);
    return std::make_unique<DoWhileStmt>(std::move(body), std::move(cond));
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    consume(TokenKind::For);
    consume(TokenKind::LParen);

    std::unique_ptr<Stmt> init;
    if (match(TokenKind::Semicolon)) {
        init = nullptr;
    } else if (isTypeToken(current_.kind)) {
        init = parseDeclStmt();
    } else {
        auto expr = parseExpr();
        consume(TokenKind::Semicolon);
        init = std::make_unique<ExprStmt>(std::move(expr));
    }

    std::unique_ptr<Expr> cond;
    if (current_.kind != TokenKind::Semicolon)
        cond = parseExpr();
    consume(TokenKind::Semicolon);

    std::unique_ptr<Expr> step;
    if (current_.kind != TokenKind::RParen)
        step = parseExpr();
    consume(TokenKind::RParen);

    return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), parseStmt());
}

std::unique_ptr<Stmt> Parser::parseSwitchStmt() {
    consume(TokenKind::Switch);
    consume(TokenKind::LParen);
    auto cond = parseExpr();
    consume(TokenKind::RParen);
    consume(TokenKind::LBrace);

    bool sawDefault = false;
    std::vector<SwitchStmt::Section> sections;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        std::unique_ptr<Expr> value;
        if (match(TokenKind::Case)) {
            value = parseAssignment();
            consume(TokenKind::Colon);
        } else if (match(TokenKind::Default)) {
            if (sawDefault)
                throw std::runtime_error("Duplicate default label in switch");
            sawDefault = true;
            consume(TokenKind::Colon);
        } else {
            throw std::runtime_error("Expected case/default label in switch body");
        }

        std::vector<std::unique_ptr<Stmt>> statements;
        while (current_.kind != TokenKind::Case && current_.kind != TokenKind::Default &&
               current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
            statements.push_back(parseStmt());
        }
        sections.emplace_back(std::move(value), std::move(statements));
    }
    consume(TokenKind::RBrace);
    return std::make_unique<SwitchStmt>(std::move(cond), std::move(sections));
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    if (match(TokenKind::Semicolon))
        return std::make_unique<ExprStmt>(nullptr);
    auto expr = parseExpr();
    consume(TokenKind::Semicolon);
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::parseInitializer() {
    if (!match(TokenKind::LBrace))
        return parseAssignment();

    std::vector<InitListExpr::Entry> entries;
    if (current_.kind != TokenKind::RBrace) {
        do {
            if (current_.kind == TokenKind::RBrace)
                break;
            InitListExpr::Designator designator;
            if (match(TokenKind::Dot)) {
                designator = InitListExpr::Designator(consume(TokenKind::Ident).lexeme);
                consume(TokenKind::Assign);
            } else if (match(TokenKind::LBracket)) {
                int64_t index = evalConstantExpr(*parseAssignment());
                if (index < 0)
                    throw std::runtime_error("Initializer array designator cannot be negative");
                consume(TokenKind::RBracket);
                consume(TokenKind::Assign);
                designator = InitListExpr::Designator(static_cast<uint64_t>(index));
            }
            entries.emplace_back(std::move(designator), parseInitializer());
        } while (match(TokenKind::Comma));
    }
    consume(TokenKind::RBrace);
    return std::make_unique<InitListExpr>(std::move(entries));
}

} // namespace minic
