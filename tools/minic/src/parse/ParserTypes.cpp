#include "minic/parse/Parser.h"
#include "minic/ast/ASTUtils.h"
#include <stdexcept>
#include <utility>

namespace minic {

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
    return parseArraySuffix(parsePointerSuffix(parseBaseType()));
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
        return static_cast<int64_t>(sizeofExpr->expr ? 8 : sizeOfType(sizeofExpr->type));
    if (auto* alignofExpr = dynamic_cast<const AlignofExpr*>(&expr))
        return static_cast<int64_t>(alignOfType(alignofExpr->type));
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
    std::string name;
    if (current_.kind == TokenKind::Ident)
        name = consume(TokenKind::Ident).lexeme;
    params.push_back({parseParamArraySuffix(type), std::move(name)});

    while (match(TokenKind::Comma)) {
        CType nextType = parsePointerSuffix(parseBaseType());
        std::string nextName;
        if (current_.kind == TokenKind::Ident)
            nextName = consume(TokenKind::Ident).lexeme;
        params.push_back({parseParamArraySuffix(nextType), std::move(nextName)});
    }
    return params;
}

} // namespace minic
