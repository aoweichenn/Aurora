#include "Parser.h"
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
    while (current_.kind != TokenKind::Eof) {
        functions.push_back(parseFunction());
    }
    return functions;
}

bool Parser::isTypeToken(TokenKind kind) const {
    return kind == TokenKind::Int || kind == TokenKind::Long ||
           kind == TokenKind::Char || kind == TokenKind::Void;
}

CType Parser::parseType() {
    CType type;
    if (match(TokenKind::Int)) {
        type.kind = CTypeKind::Int;
        return type;
    }
    if (match(TokenKind::Long)) {
        type.kind = CTypeKind::Long;
        (void)match(TokenKind::Int);
        return type;
    }
    if (match(TokenKind::Char)) {
        type.kind = CTypeKind::Char;
        return type;
    }
    if (match(TokenKind::Void)) {
        type.kind = CTypeKind::Void;
        return type;
    }
    throw std::runtime_error("Expected type name");
}

Function Parser::parseFunction() {
    if (current_.kind == TokenKind::Fn)
        return parseLegacyFunction();

    CType returnType = parseType();
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

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    if (current_.kind == TokenKind::RParen)
        return params;

    if (current_.kind == TokenKind::Void) {
        CType voidType = parseType();
        if (current_.kind == TokenKind::RParen)
            return params;
        params.push_back({voidType, consume(TokenKind::Ident).lexeme});
    } else {
        CType type = parseType();
        params.push_back({type, consume(TokenKind::Ident).lexeme});
    }

    while (match(TokenKind::Comma)) {
        CType type = parseType();
        params.push_back({type, consume(TokenKind::Ident).lexeme});
    }
    return params;
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (current_.kind == TokenKind::LBrace)
        return parseBlock();
    if (isTypeToken(current_.kind))
        return parseDeclStmt();
    if (current_.kind == TokenKind::Return)
        return parseReturnStmt();
    if (current_.kind == TokenKind::If)
        return parseIfStmt();
    if (current_.kind == TokenKind::While)
        return parseWhileStmt();
    if (current_.kind == TokenKind::For)
        return parseForStmt();
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
    CType type = parseType();
    if (type.isVoid())
        throw std::runtime_error("Variables cannot have void type");

    std::vector<DeclStmt::Declarator> declarators;
    do {
        std::string name = consume(TokenKind::Ident).lexeme;
        std::unique_ptr<Expr> init;
        if (match(TokenKind::Assign))
            init = parseExpr();
        declarators.emplace_back(std::move(name), std::move(init));
    } while (match(TokenKind::Comma));

    if (consumeSemicolon)
        consume(TokenKind::Semicolon);
    return std::make_unique<DeclStmt>(type, std::move(declarators));
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

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    if (match(TokenKind::Semicolon))
        return std::make_unique<ExprStmt>(nullptr);
    auto expr = parseExpr();
    consume(TokenKind::Semicolon);
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::parseExpr() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto left = parseConditional();
    AssignExpr::Op op;
    bool isAssign = true;
    switch (current_.kind) {
    case TokenKind::Assign: op = AssignExpr::Assign; break;
    case TokenKind::PlusAssign: op = AssignExpr::AddAssign; break;
    case TokenKind::MinusAssign: op = AssignExpr::SubAssign; break;
    case TokenKind::StarAssign: op = AssignExpr::MulAssign; break;
    case TokenKind::SlashAssign: op = AssignExpr::DivAssign; break;
    case TokenKind::PercentAssign: op = AssignExpr::RemAssign; break;
    default: isAssign = false; op = AssignExpr::Assign; break;
    }
    if (!isAssign)
        return left;

    auto* var = dynamic_cast<VarExpr*>(left.get());
    if (!var)
        throw std::runtime_error("Left side of assignment must be a variable");
    std::string name = var->name;
    advance();
    return std::make_unique<AssignExpr>(op, std::move(name), parseAssignment());
}

std::unique_ptr<Expr> Parser::parseConditional() {
    auto cond = parseLogicalOr();
    if (!match(TokenKind::Question))
        return cond;
    auto trueExpr = parseExpr();
    consume(TokenKind::Colon);
    auto falseExpr = parseConditional();
    return std::make_unique<ConditionalExpr>(std::move(cond), std::move(trueExpr), std::move(falseExpr));
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (match(TokenKind::PipePipe))
        left = std::make_unique<BinaryExpr>(BinaryExpr::LogOr, std::move(left), parseLogicalAnd());
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseBitOr();
    while (match(TokenKind::AmpAmp))
        left = std::make_unique<BinaryExpr>(BinaryExpr::LogAnd, std::move(left), parseBitOr());
    return left;
}

std::unique_ptr<Expr> Parser::parseBitOr() {
    auto left = parseBitXor();
    while (match(TokenKind::Pipe))
        left = std::make_unique<BinaryExpr>(BinaryExpr::BitOr, std::move(left), parseBitXor());
    return left;
}

std::unique_ptr<Expr> Parser::parseBitXor() {
    auto left = parseBitAnd();
    while (match(TokenKind::Caret))
        left = std::make_unique<BinaryExpr>(BinaryExpr::BitXor, std::move(left), parseBitAnd());
    return left;
}

std::unique_ptr<Expr> Parser::parseBitAnd() {
    auto left = parseEquality();
    while (match(TokenKind::Amp))
        left = std::make_unique<BinaryExpr>(BinaryExpr::BitAnd, std::move(left), parseEquality());
    return left;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseRelational();
    while (true) {
        if (match(TokenKind::EqEq))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Eq, std::move(left), parseRelational());
        else if (match(TokenKind::Neq))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Ne, std::move(left), parseRelational());
        else
            return left;
    }
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto left = parseShift();
    while (true) {
        if (match(TokenKind::Lt))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Lt, std::move(left), parseShift());
        else if (match(TokenKind::Le))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Le, std::move(left), parseShift());
        else if (match(TokenKind::Gt))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Gt, std::move(left), parseShift());
        else if (match(TokenKind::Ge))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Ge, std::move(left), parseShift());
        else
            return left;
    }
}

std::unique_ptr<Expr> Parser::parseShift() {
    auto left = parseSum();
    while (true) {
        if (match(TokenKind::Shl))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Shl, std::move(left), parseSum());
        else if (match(TokenKind::Shr))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Shr, std::move(left), parseSum());
        else
            return left;
    }
}

std::unique_ptr<Expr> Parser::parseSum() {
    auto left = parseMul();
    while (true) {
        if (match(TokenKind::Plus))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Add, std::move(left), parseMul());
        else if (match(TokenKind::Minus))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Sub, std::move(left), parseMul());
        else
            return left;
    }
}

std::unique_ptr<Expr> Parser::parseMul() {
    auto left = parseUnary();
    while (true) {
        if (match(TokenKind::Star))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Mul, std::move(left), parseUnary());
        else if (match(TokenKind::Slash))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Div, std::move(left), parseUnary());
        else if (match(TokenKind::Percent))
            left = std::make_unique<BinaryExpr>(BinaryExpr::Rem, std::move(left), parseUnary());
        else
            return left;
    }
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenKind::PlusPlus))
        return std::make_unique<IncDecExpr>(consume(TokenKind::Ident).lexeme, true, true);
    if (match(TokenKind::MinusMinus))
        return std::make_unique<IncDecExpr>(consume(TokenKind::Ident).lexeme, false, true);
    if (match(TokenKind::Plus))
        return std::make_unique<UnaryExpr>(UnaryExpr::Plus, parseUnary());
    if (match(TokenKind::Minus))
        return std::make_unique<UnaryExpr>(UnaryExpr::Neg, parseUnary());
    if (match(TokenKind::Bang))
        return std::make_unique<UnaryExpr>(UnaryExpr::LogicalNot, parseUnary());
    if (match(TokenKind::Tilde))
        return std::make_unique<UnaryExpr>(UnaryExpr::BitNot, parseUnary());
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (current_.kind == TokenKind::PlusPlus || current_.kind == TokenKind::MinusMinus) {
        bool increment = current_.kind == TokenKind::PlusPlus;
        advance();
        auto* var = dynamic_cast<VarExpr*>(expr.get());
        if (!var)
            throw std::runtime_error("Postfix increment/decrement requires a variable");
        expr = std::make_unique<IncDecExpr>(var->name, increment, false);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (current_.kind == TokenKind::IntLit) {
        int64_t value = current_.intValue;
        advance();
        return std::make_unique<IntLitExpr>(value);
    }

    if (current_.kind == TokenKind::Ident) {
        std::string name = current_.lexeme;
        advance();
        if (match(TokenKind::LParen)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (current_.kind != TokenKind::RParen) {
                args.push_back(parseExpr());
                while (match(TokenKind::Comma))
                    args.push_back(parseExpr());
            }
            consume(TokenKind::RParen);
            return std::make_unique<CallExpr>(std::move(name), std::move(args));
        }
        return std::make_unique<VarExpr>(std::move(name));
    }

    if (current_.kind == TokenKind::If) {
        consume(TokenKind::If);
        auto cond = parseExpr();
        consume(TokenKind::Then);
        auto trueExpr = parseExpr();
        consume(TokenKind::Else);
        auto falseExpr = parseExpr();
        return std::make_unique<ConditionalExpr>(std::move(cond), std::move(trueExpr), std::move(falseExpr));
    }

    if (match(TokenKind::LParen)) {
        auto expr = parseExpr();
        consume(TokenKind::RParen);
        return expr;
    }

    throw std::runtime_error(
        std::string("Unexpected token: ") + tokenName(current_.kind) +
        " ('" + current_.lexeme + "')");
}

} // namespace minic
