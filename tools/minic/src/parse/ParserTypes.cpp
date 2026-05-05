#include "minic/parse/Parser.h"
#include "minic/ast/ASTUtils.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace minic {

namespace {

uint64_t alignTo(uint64_t value, uint64_t alignment) noexcept {
    if (alignment <= 1)
        return value;
    return ((value + alignment - 1) / alignment) * alignment;
}

bool isCompleteObjectType(CType type) noexcept {
    if (type.pointerDepth > 0)
        return true;
    if (type.arraySize > 0) {
        type.arraySize = 0;
        return isCompleteObjectType(type);
    }
    return (type.kind != CTypeKind::Struct && type.kind != CTypeKind::Union) ||
           (type.structInfo && type.structInfo->complete);
}

bool isPowerOfTwo(uint64_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

void applyRequiredAlign(CType& type, uint64_t requiredAlign) noexcept {
    type.requiredAlign = std::max(type.requiredAlign, requiredAlign);
}

} // namespace

bool Parser::isTypeToken(TokenKind kind) const {
    return kind == TokenKind::Int || kind == TokenKind::Long ||
           kind == TokenKind::Short || kind == TokenKind::Char ||
           kind == TokenKind::Bool || kind == TokenKind::Void ||
           kind == TokenKind::Signed || kind == TokenKind::Unsigned ||
           kind == TokenKind::Enum || kind == TokenKind::Struct ||
           kind == TokenKind::Union ||
           isTypeQualifier(kind) ||
           (kind == TokenKind::Ident && typedefs_.find(current_.lexeme) != typedefs_.end());
}

bool Parser::isTypeQualifier(TokenKind kind) const {
    return kind == TokenKind::Const || kind == TokenKind::Volatile ||
           kind == TokenKind::Restrict || kind == TokenKind::Static ||
           kind == TokenKind::Extern || kind == TokenKind::Auto ||
           kind == TokenKind::Register || kind == TokenKind::Inline ||
           kind == TokenKind::Alignas;
}

uint64_t Parser::consumeTypeQualifiers() {
    uint64_t requiredAlign = 0;
    while (isTypeQualifier(current_.kind)) {
        if (current_.kind == TokenKind::Alignas) {
            requiredAlign = std::max(requiredAlign, parseAlignmentSpecifier());
        } else {
            advance();
        }
    }
    return requiredAlign;
}

uint64_t Parser::parseAlignmentSpecifier() {
    consume(TokenKind::Alignas);
    consume(TokenKind::LParen);
    uint64_t alignment = 0;
    if (isTypeToken(current_.kind)) {
        alignment = alignOfType(parseType());
    } else {
        int64_t value = evalConstantExpr(*parseAssignment());
        if (value < 0)
            throw std::runtime_error("alignas value cannot be negative");
        alignment = static_cast<uint64_t>(value);
    }
    consume(TokenKind::RParen);
    if (alignment != 0 && !isPowerOfTwo(alignment))
        throw std::runtime_error("alignas value must be zero or a power of two");
    return alignment;
}

CType Parser::parseBaseType() {
    CType type;
    uint64_t requiredAlign = consumeTypeQualifiers();

    if (match(TokenKind::Enum)) {
        if (current_.kind == TokenKind::Ident)
            advance();
        if (current_.kind == TokenKind::LBrace)
            parseEnumBody();
        type.kind = CTypeKind::Int;
        applyRequiredAlign(type, requiredAlign);
        return type;
    }

    if (current_.kind == TokenKind::Struct || current_.kind == TokenKind::Union) {
        type = parseRecordSpecifier(current_.kind == TokenKind::Union);
        applyRequiredAlign(type, requiredAlign);
        return type;
    }

    if (current_.kind == TokenKind::Ident) {
        auto typedefIt = typedefs_.find(current_.lexeme);
        if (typedefIt != typedefs_.end()) {
            type = typedefIt->second;
            advance();
            applyRequiredAlign(type, requiredAlign);
            return type;
        }
    }

    bool sawSignedness = false;
    while (current_.kind == TokenKind::Signed || current_.kind == TokenKind::Unsigned) {
        sawSignedness = true;
        if (current_.kind == TokenKind::Unsigned)
            type.isUnsigned = true;
        advance();
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
    }

    if (match(TokenKind::Bool)) {
        type.kind = CTypeKind::Bool;
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    if (match(TokenKind::Int)) {
        type.kind = CTypeKind::Int;
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    if (match(TokenKind::Short)) {
        type.kind = CTypeKind::Short;
        (void)match(TokenKind::Int);
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    if (match(TokenKind::Long)) {
        type.kind = CTypeKind::Long;
        (void)match(TokenKind::Int);
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    if (match(TokenKind::Char)) {
        type.kind = CTypeKind::Char;
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    if (match(TokenKind::Void)) {
        type.kind = CTypeKind::Void;
        requiredAlign = std::max(requiredAlign, consumeTypeQualifiers());
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    if (sawSignedness) {
        type.kind = CTypeKind::Int;
        applyRequiredAlign(type, requiredAlign);
        return type;
    }
    throw std::runtime_error("Expected type name");
}

CType Parser::parsePointerSuffix(CType type) {
    while (match(TokenKind::Star)) {
        ++type.pointerDepth;
        applyRequiredAlign(type, consumeTypeQualifiers());
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
    applyRequiredAlign(type, consumeTypeQualifiers());
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

CType Parser::parseRecordSpecifier(bool isUnion) {
    consume(isUnion ? TokenKind::Union : TokenKind::Struct);
    std::string tag;
    if (current_.kind == TokenKind::Ident)
        tag = consume(TokenKind::Ident).lexeme;

    std::shared_ptr<CStructInfo> info;
    if (!tag.empty()) {
        auto& slot = records_[tag];
        if (!slot) {
            slot = std::make_shared<CStructInfo>();
            slot->tag = tag;
            slot->isUnion = isUnion;
        } else if (slot->isUnion != isUnion) {
            throw std::runtime_error("Tag kind mismatch for: " + tag);
        }
        info = slot;
    } else if (current_.kind == TokenKind::LBrace) {
        info = std::make_shared<CStructInfo>();
        info->isUnion = isUnion;
    } else {
        throw std::runtime_error(isUnion ? "Expected union tag or definition" : "Expected struct tag or definition");
    }

    if (match(TokenKind::LBrace)) {
        if (info->complete)
            throw std::runtime_error("Redefinition of tag: " + (tag.empty() ? std::string("<anonymous>") : tag));
        info->fields = parseRecordFields();
        consume(TokenKind::RBrace);

        uint64_t offset = 0;
        uint64_t maxAlign = 1;
        uint64_t maxSize = 0;
        for (auto& field : info->fields) {
            uint64_t fieldSize = sizeOfType(field.type);
            uint64_t fieldAlign = alignOfType(field.type);
            if (fieldSize == 0 || !isCompleteObjectType(field.type))
                throw std::runtime_error("Record field has incomplete type: " + field.name);
            maxAlign = std::max(maxAlign, fieldAlign);
            maxSize = std::max(maxSize, fieldSize);
            if (isUnion) {
                field.offset = 0;
            } else {
                offset = alignTo(offset, fieldAlign);
                field.offset = offset;
                offset += fieldSize;
            }
        }
        info->align = maxAlign;
        info->size = alignTo(isUnion ? maxSize : offset, maxAlign);
        info->complete = true;
    }

    CType type;
    type.kind = isUnion ? CTypeKind::Union : CTypeKind::Struct;
    type.structInfo = info;
    return type;
}

std::vector<CField> Parser::parseRecordFields() {
    std::vector<CField> fields;
    while (current_.kind != TokenKind::RBrace && current_.kind != TokenKind::Eof) {
        CType baseType = parseBaseType();
        do {
            CType fieldType = parsePointerSuffix(baseType);
            std::string fieldName = consume(TokenKind::Ident).lexeme;
            fieldType = parseArraySuffix(fieldType);
            if (fieldType.isVoid())
                throw std::runtime_error("Record field cannot have void type");
            if (std::any_of(fields.begin(), fields.end(), [&](const CField& field) { return field.name == fieldName; }))
                throw std::runtime_error("Duplicate record field: " + fieldName);
            fields.emplace_back(fieldType, std::move(fieldName));
        } while (match(TokenKind::Comma));
        consume(TokenKind::Semicolon);
    }
    return fields;
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
