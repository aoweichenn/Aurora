#include "Lexer.h"
#include <cctype>
#include <cstdlib>

namespace minic {

const char* tokenName(TokenKind kind) {
    switch (kind) {
    case TokenKind::Eof:    return "EOF";
    case TokenKind::Ident:  return "Identifier";
    case TokenKind::IntLit: return "Integer";
    case TokenKind::Invalid:return "Invalid";
    case TokenKind::Fn:     return "fn";
    case TokenKind::If:     return "if";
    case TokenKind::Then:   return "then";
    case TokenKind::Else:   return "else";
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::Assign: return "=";
    case TokenKind::Plus:   return "+";
    case TokenKind::Minus:  return "-";
    case TokenKind::Star:   return "*";
    case TokenKind::Slash:  return "/";
    case TokenKind::EqEq:   return "==";
    case TokenKind::Neq:    return "!=";
    case TokenKind::Lt:     return "<";
    case TokenKind::Le:     return "<=";
    case TokenKind::Gt:     return ">";
    case TokenKind::Ge:     return ">=";
    case TokenKind::Comma:  return ",";
    }
    return "?";
}

Lexer::Lexer(std::string_view source) : source_(source), pos_(0) {}

void Lexer::skipWhitespace() {
    while (pos_ < source_.size() && std::isspace(source_[pos_]))
        ++pos_;
}

Token Lexer::readNumber() {
    size_t start = pos_;
    while (pos_ < source_.size() && std::isdigit(source_[pos_]))
        ++pos_;
    Token tok;
    tok.kind = TokenKind::IntLit;
    tok.lexeme = std::string(source_.substr(start, pos_ - start));
    tok.intValue = std::stoll(tok.lexeme);
    return tok;
}

Token Lexer::readIdent() {
    size_t start = pos_;
    while (pos_ < source_.size() && (std::isalnum(source_[pos_]) || source_[pos_] == '_'))
        ++pos_;
    std::string lexeme(source_.substr(start, pos_ - start));
    Token tok;
    tok.lexeme = lexeme;

    if (lexeme == "fn")   tok.kind = TokenKind::Fn;
    else if (lexeme == "if")    tok.kind = TokenKind::If;
    else if (lexeme == "then")  tok.kind = TokenKind::Then;
    else if (lexeme == "else")  tok.kind = TokenKind::Else;
    else {
        tok.kind = TokenKind::Ident;
        tok.intValue = 0;
    }
    return tok;
}

Token Lexer::next() {
    skipWhitespace();
    if (pos_ >= source_.size()) return {TokenKind::Eof, "", 0};

    char c = source_[pos_];

    if (std::isdigit(c)) return readNumber();
    if (std::isalpha(c) || c == '_') return readIdent();

    ++pos_;
    switch (c) {
    case '(': return {TokenKind::LParen, "(", 0};
    case ')': return {TokenKind::RParen, ")", 0};
    case '=':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::EqEq, "==", 0};
        }
        return {TokenKind::Assign, "=", 0};
    case '+': return {TokenKind::Plus, "+", 0};
    case '-': return {TokenKind::Minus, "-", 0};
    case '*': return {TokenKind::Star, "*", 0};
    case '/': return {TokenKind::Slash, "/", 0};
    case '!':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::Neq, "!=", 0};
        }
        break;
    case '<':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::Le, "<=", 0};
        }
        return {TokenKind::Lt, "<", 0};
    case '>':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::Ge, ">=", 0};
        }
        return {TokenKind::Gt, ">", 0};
    case ',': return {TokenKind::Comma, ",", 0};
    }
    return {TokenKind::Invalid, std::string(1, c), 0};
}

} // namespace minic
