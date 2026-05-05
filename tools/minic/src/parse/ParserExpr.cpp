#include "minic/parse/Parser.h"
#include "minic/ast/ASTUtils.h"
#include <stdexcept>
#include <utility>

namespace minic {

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

    if (!isAssignableExpr(*left))
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
        if (!isAssignableExpr(*target))
            throw std::runtime_error("Prefix increment/decrement requires an assignable expression");
        return std::make_unique<IncDecExpr>(std::move(target), true, true);
    }
    if (match(TokenKind::MinusMinus)) {
        auto target = parseUnary();
        if (!isAssignableExpr(*target))
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
    if (match(TokenKind::Alignof)) {
        consume(TokenKind::LParen);
        CType type = parseType();
        consume(TokenKind::RParen);
        return std::make_unique<AlignofExpr>(type);
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
        if (!isAssignableExpr(*target))
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
        if (match(TokenKind::Dot)) {
            std::string field = consume(TokenKind::Ident).lexeme;
            expr = std::make_unique<MemberExpr>(std::move(expr), std::move(field), false);
            continue;
        }
        if (match(TokenKind::Arrow)) {
            std::string field = consume(TokenKind::Ident).lexeme;
            expr = std::make_unique<MemberExpr>(std::move(expr), std::move(field), true);
            continue;
        }
        if (current_.kind == TokenKind::PlusPlus || current_.kind == TokenKind::MinusMinus) {
            bool increment = current_.kind == TokenKind::PlusPlus;
            advance();
            if (!isAssignableExpr(*expr))
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
            if (current_.kind == TokenKind::LBrace) {
                auto init = parseInitializer();
                auto* initList = dynamic_cast<InitListExpr*>(init.get());
                if (!initList)
                    throw std::runtime_error("Compound literal requires a braced initializer");
                (void)init.release();
                return std::make_unique<CompoundLiteralExpr>(type, std::unique_ptr<InitListExpr>(initList));
            }
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
