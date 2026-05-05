#include "Parser.h"
#include <stdexcept>

namespace minic {

namespace {

bool isAssignableExpr(const Expr* expr) {
    if (dynamic_cast<const VarExpr*>(expr))
        return true;
    if (dynamic_cast<const IndexExpr*>(expr))
        return true;
    if (auto* unary = dynamic_cast<const UnaryExpr*>(expr))
        return unary->op == UnaryExpr::Deref;
    return false;
}

uint64_t parserSizeOfType(CType type) {
    if (type.arraySize > 0) {
        CType elementType = type;
        elementType.arraySize = 0;
        return type.arraySize * parserSizeOfType(elementType);
    }
    if (type.pointerDepth > 0)
        return 8;
    switch (type.kind) {
    case CTypeKind::Bool: return 1;
    case CTypeKind::Char: return 1;
    case CTypeKind::Short: return 2;
    case CTypeKind::Int: return 4;
    case CTypeKind::Long: return 8;
    case CTypeKind::Void: return 1;
    }
    return 8;
}

} // namespace

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

bool Parser::isTypeToken(TokenKind kind) const {
    return kind == TokenKind::Int || kind == TokenKind::Long ||
           kind == TokenKind::Short || kind == TokenKind::Char ||
           kind == TokenKind::Bool || kind == TokenKind::Void ||
           kind == TokenKind::Signed || kind == TokenKind::Unsigned ||
           kind == TokenKind::Enum || isTypeQualifier(kind) ||
           (kind == TokenKind::Ident && typedefs_.find(current_.lexeme) != typedefs_.end());
}

bool Parser::isTypeQualifier(TokenKind kind) const {
    return kind == TokenKind::Const || kind == TokenKind::Volatile ||
           kind == TokenKind::Restrict || kind == TokenKind::Static ||
           kind == TokenKind::Extern || kind == TokenKind::Auto ||
           kind == TokenKind::Register || kind == TokenKind::Inline;
}

void Parser::consumeTypeQualifiers() {
    while (isTypeQualifier(current_.kind))
        advance();
}

CType Parser::parseBaseType() {
    CType type;
    consumeTypeQualifiers();

    if (match(TokenKind::Enum)) {
        if (current_.kind == TokenKind::Ident)
            advance();
        if (current_.kind == TokenKind::LBrace)
            parseEnumBody();
        type.kind = CTypeKind::Int;
        return type;
    }

    if (current_.kind == TokenKind::Ident) {
        auto typedefIt = typedefs_.find(current_.lexeme);
        if (typedefIt != typedefs_.end()) {
            type = typedefIt->second;
            advance();
            return type;
        }
    }

    bool sawSignedness = false;
    while (current_.kind == TokenKind::Signed || current_.kind == TokenKind::Unsigned) {
        sawSignedness = true;
        if (current_.kind == TokenKind::Unsigned)
            type.isUnsigned = true;
        advance();
        consumeTypeQualifiers();
    }

    if (match(TokenKind::Bool)) {
        type.kind = CTypeKind::Bool;
        consumeTypeQualifiers();
        return type;
    }
    if (match(TokenKind::Int)) {
        type.kind = CTypeKind::Int;
        consumeTypeQualifiers();
        return type;
    }
    if (match(TokenKind::Short)) {
        type.kind = CTypeKind::Short;
        (void)match(TokenKind::Int);
        consumeTypeQualifiers();
        return type;
    }
    if (match(TokenKind::Long)) {
        type.kind = CTypeKind::Long;
        (void)match(TokenKind::Int);
        consumeTypeQualifiers();
        return type;
    }
    if (match(TokenKind::Char)) {
        type.kind = CTypeKind::Char;
        consumeTypeQualifiers();
        return type;
    }
    if (match(TokenKind::Void)) {
        type.kind = CTypeKind::Void;
        consumeTypeQualifiers();
        return type;
    }
    if (sawSignedness) {
        type.kind = CTypeKind::Int;
        return type;
    }
    throw std::runtime_error("Expected type name");
}

CType Parser::parsePointerSuffix(CType type) {
    while (match(TokenKind::Star)) {
        ++type.pointerDepth;
        consumeTypeQualifiers();
    }
    return type;
}

CType Parser::parseArraySuffix(CType type) {
    if (!match(TokenKind::LBracket))
        return type;
    Token size = consume(TokenKind::IntLit);
    if (size.intValue <= 0)
        throw std::runtime_error("Array size must be positive");
    type.arraySize = static_cast<uint64_t>(size.intValue);
    consume(TokenKind::RBracket);
    return type;
}

CType Parser::parseParamArraySuffix(CType type) {
    if (!match(TokenKind::LBracket))
        return type;
    consumeTypeQualifiers();
    if (current_.kind != TokenKind::RBracket)
        (void)parseAssignment();
    consume(TokenKind::RBracket);
    ++type.pointerDepth;
    return type;
}

CType Parser::parseType() {
    return parsePointerSuffix(parseBaseType());
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

void Parser::parseTypedefDecl() {
    CType baseType = parseBaseType();
    do {
        CType type = parsePointerSuffix(baseType);
        std::string name = consume(TokenKind::Ident).lexeme;
        type = parseArraySuffix(type);
        typedefs_[name] = type;
    } while (match(TokenKind::Comma));
    consume(TokenKind::Semicolon);
}

void Parser::parseStaticAssertDecl() {
    consume(TokenKind::StaticAssert);
    consume(TokenKind::LParen);
    auto expr = parseAssignment();
    if (match(TokenKind::Comma)) {
        if (current_.kind == TokenKind::StringLit)
            advance();
        else
            (void)parseAssignment();
    }
    consume(TokenKind::RParen);
    consume(TokenKind::Semicolon);
    if (evalConstantExpr(*expr) == 0)
        throw std::runtime_error("static_assert failed");
}

void Parser::parseEnumBody() {
    consume(TokenKind::LBrace);
    int64_t nextValue = 0;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        std::string name = consume(TokenKind::Ident).lexeme;
        int64_t value = nextValue;
        if (match(TokenKind::Assign))
            value = evalConstantExpr(*parseAssignment());
        enumConstants_[name] = value;
        nextValue = value + 1;
        if (!match(TokenKind::Comma))
            break;
        if (current_.kind == TokenKind::RBrace)
            break;
    }
    consume(TokenKind::RBrace);
}

int64_t Parser::evalConstantExpr(const Expr& expr) const {
    if (auto* literal = dynamic_cast<const IntLitExpr*>(&expr))
        return literal->value;
    if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        int64_t value = evalConstantExpr(*unary->operand);
        switch (unary->op) {
        case UnaryExpr::Plus: return value;
        case UnaryExpr::Neg: return -value;
        case UnaryExpr::LogicalNot: return value == 0;
        case UnaryExpr::BitNot: return ~value;
        case UnaryExpr::AddressOf:
        case UnaryExpr::Deref:
            break;
        }
    }
    if (auto* cast = dynamic_cast<const CastExpr*>(&expr))
        return evalConstantExpr(*cast->operand);
    if (auto* binary = dynamic_cast<const BinaryExpr*>(&expr)) {
        int64_t lhs = evalConstantExpr(*binary->lhs);
        int64_t rhs = evalConstantExpr(*binary->rhs);
        switch (binary->op) {
        case BinaryExpr::Add: return lhs + rhs;
        case BinaryExpr::Sub: return lhs - rhs;
        case BinaryExpr::Mul: return lhs * rhs;
        case BinaryExpr::Div: return rhs == 0 ? 0 : lhs / rhs;
        case BinaryExpr::Rem: return rhs == 0 ? 0 : lhs % rhs;
        case BinaryExpr::Eq: return lhs == rhs;
        case BinaryExpr::Ne: return lhs != rhs;
        case BinaryExpr::Lt: return lhs < rhs;
        case BinaryExpr::Le: return lhs <= rhs;
        case BinaryExpr::Gt: return lhs > rhs;
        case BinaryExpr::Ge: return lhs >= rhs;
        case BinaryExpr::BitAnd: return lhs & rhs;
        case BinaryExpr::BitOr: return lhs | rhs;
        case BinaryExpr::BitXor: return lhs ^ rhs;
        case BinaryExpr::Shl: return lhs << rhs;
        case BinaryExpr::Shr: return lhs >> rhs;
        case BinaryExpr::LogAnd: return lhs && rhs;
        case BinaryExpr::LogOr: return lhs || rhs;
        }
    }
    if (auto* conditional = dynamic_cast<const ConditionalExpr*>(&expr))
        return evalConstantExpr(*(evalConstantExpr(*conditional->cond) ? conditional->trueExpr : conditional->falseExpr));
    if (auto* comma = dynamic_cast<const CommaExpr*>(&expr))
        return evalConstantExpr(*comma->rhs);
    if (auto* sizeofExpr = dynamic_cast<const SizeofExpr*>(&expr))
        return static_cast<int64_t>(sizeofExpr->expr ? 8 : parserSizeOfType(sizeofExpr->type));
    throw std::runtime_error("Expected integer constant expression");
}

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    if (current_.kind == TokenKind::RParen)
        return params;

    CType type = parseBaseType();
    if (type.isVoid() && current_.kind == TokenKind::RParen)
        return params;
    type = parsePointerSuffix(type);
    std::string name = consume(TokenKind::Ident).lexeme;
    params.push_back({parseParamArraySuffix(type), std::move(name)});

    while (match(TokenKind::Comma)) {
        CType nextType = parsePointerSuffix(parseBaseType());
        std::string nextName = consume(TokenKind::Ident).lexeme;
        params.push_back({parseParamArraySuffix(nextType), std::move(nextName)});
    }
    return params;
}

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

    std::vector<std::unique_ptr<Expr>> values;
    if (current_.kind != TokenKind::RBrace) {
        do {
            if (current_.kind == TokenKind::RBrace)
                break;
            values.push_back(parseAssignment());
        } while (match(TokenKind::Comma));
    }
    consume(TokenKind::RBrace);
    return std::make_unique<InitListExpr>(std::move(values));
}

std::unique_ptr<Expr> Parser::parseExpr() {
    return parseComma();
}

std::unique_ptr<Expr> Parser::parseComma() {
    auto left = parseAssignment();
    while (match(TokenKind::Comma))
        left = std::make_unique<CommaExpr>(std::move(left), parseAssignment());
    return left;
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
    case TokenKind::AmpAssign: op = AssignExpr::BitAndAssign; break;
    case TokenKind::PipeAssign: op = AssignExpr::BitOrAssign; break;
    case TokenKind::CaretAssign: op = AssignExpr::BitXorAssign; break;
    case TokenKind::ShlAssign: op = AssignExpr::ShlAssign; break;
    case TokenKind::ShrAssign: op = AssignExpr::ShrAssign; break;
    default: isAssign = false; op = AssignExpr::Assign; break;
    }
    if (!isAssign)
        return left;

    if (!isAssignableExpr(left.get()))
        throw std::runtime_error("Left side of assignment must be assignable");
    advance();
    return std::make_unique<AssignExpr>(op, std::move(left), parseAssignment());
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
    if (match(TokenKind::PlusPlus)) {
        auto target = parseUnary();
        if (!isAssignableExpr(target.get()))
            throw std::runtime_error("Prefix increment/decrement requires an assignable expression");
        return std::make_unique<IncDecExpr>(std::move(target), true, true);
    }
    if (match(TokenKind::MinusMinus)) {
        auto target = parseUnary();
        if (!isAssignableExpr(target.get()))
            throw std::runtime_error("Prefix increment/decrement requires an assignable expression");
        return std::make_unique<IncDecExpr>(std::move(target), false, true);
    }
    if (match(TokenKind::Sizeof)) {
        if (match(TokenKind::LParen)) {
            if (isTypeToken(current_.kind)) {
                CType type = parseType();
                consume(TokenKind::RParen);
                return std::make_unique<SizeofExpr>(type);
            }
            auto expr = parseExpr();
            consume(TokenKind::RParen);
            return std::make_unique<SizeofExpr>(std::move(expr));
        }
        return std::make_unique<SizeofExpr>(parseUnary());
    }
    if (match(TokenKind::Plus))
        return std::make_unique<UnaryExpr>(UnaryExpr::Plus, parseUnary());
    if (match(TokenKind::Minus))
        return std::make_unique<UnaryExpr>(UnaryExpr::Neg, parseUnary());
    if (match(TokenKind::Bang))
        return std::make_unique<UnaryExpr>(UnaryExpr::LogicalNot, parseUnary());
    if (match(TokenKind::Tilde))
        return std::make_unique<UnaryExpr>(UnaryExpr::BitNot, parseUnary());
    if (match(TokenKind::Amp)) {
        auto target = parseUnary();
        if (!isAssignableExpr(target.get()))
            throw std::runtime_error("Address-of requires an assignable expression");
        return std::make_unique<UnaryExpr>(UnaryExpr::AddressOf, std::move(target));
    }
    if (match(TokenKind::Star))
        return std::make_unique<UnaryExpr>(UnaryExpr::Deref, parseUnary());
    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (true) {
        if (match(TokenKind::LBracket)) {
            auto index = parseExpr();
            consume(TokenKind::RBracket);
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
            continue;
        }
        if (current_.kind == TokenKind::PlusPlus || current_.kind == TokenKind::MinusMinus) {
            bool increment = current_.kind == TokenKind::PlusPlus;
            advance();
            if (!isAssignableExpr(expr.get()))
                throw std::runtime_error("Postfix increment/decrement requires an assignable expression");
            expr = std::make_unique<IncDecExpr>(std::move(expr), increment, false);
            continue;
        }
        break;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (current_.kind == TokenKind::IntLit || current_.kind == TokenKind::CharLit) {
        int64_t value = current_.intValue;
        advance();
        return std::make_unique<IntLitExpr>(value);
    }

    if (match(TokenKind::True))
        return std::make_unique<IntLitExpr>(1);
    if (match(TokenKind::False) || match(TokenKind::Nullptr))
        return std::make_unique<IntLitExpr>(0);

    if (current_.kind == TokenKind::Ident) {
        std::string name = current_.lexeme;
        advance();
        auto enumConstantIt = enumConstants_.find(name);
        if (enumConstantIt != enumConstants_.end())
            return std::make_unique<IntLitExpr>(enumConstantIt->second);
        if (match(TokenKind::LParen)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (current_.kind != TokenKind::RParen) {
                args.push_back(parseAssignment());
                while (match(TokenKind::Comma))
                    args.push_back(parseAssignment());
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
        if (isTypeToken(current_.kind)) {
            CType type = parseType();
            consume(TokenKind::RParen);
            return std::make_unique<CastExpr>(type, parseUnary());
        }
        auto expr = parseExpr();
        consume(TokenKind::RParen);
        return expr;
    }

    throw std::runtime_error(
        std::string("Unexpected token: ") + tokenName(current_.kind) +
        " ('" + current_.lexeme + "')");
}

} // namespace minic
