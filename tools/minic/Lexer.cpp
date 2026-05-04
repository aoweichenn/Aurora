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
    case TokenKind::Return: return "return";
    case TokenKind::While:  return "while";
    case TokenKind::For:    return "for";
    case TokenKind::Break:  return "break";
    case TokenKind::Continue:return "continue";
    case TokenKind::Int:    return "int";
    case TokenKind::Long:   return "long";
    case TokenKind::Char:   return "char";
    case TokenKind::Void:   return "void";
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::LBrace: return "{";
    case TokenKind::RBrace: return "}";
    case TokenKind::Semicolon:return ";";
    case TokenKind::Question:return "?";
    case TokenKind::Colon:  return ":";
    case TokenKind::Assign: return "=";
    case TokenKind::PlusAssign:return "+=";
    case TokenKind::MinusAssign:return "-=";
    case TokenKind::StarAssign:return "*=";
    case TokenKind::SlashAssign:return "/=";
    case TokenKind::PercentAssign:return "%=";
    case TokenKind::Plus:   return "+";
    case TokenKind::Minus:  return "-";
    case TokenKind::Star:   return "*";
    case TokenKind::Slash:  return "/";
    case TokenKind::Percent:return "%";
    case TokenKind::PlusPlus:return "++";
    case TokenKind::MinusMinus:return "--";
    case TokenKind::EqEq:   return "==";
    case TokenKind::Neq:    return "!=";
    case TokenKind::Lt:     return "<";
    case TokenKind::Le:     return "<=";
    case TokenKind::Gt:     return ">";
    case TokenKind::Ge:     return ">=";
    case TokenKind::Amp:    return "&";
    case TokenKind::Pipe:   return "|";
    case TokenKind::Caret:  return "^";
    case TokenKind::Tilde:  return "~";
    case TokenKind::Bang:   return "!";
    case TokenKind::AmpAmp: return "&&";
    case TokenKind::PipePipe:return "||";
    case TokenKind::Shl:    return "<<";
    case TokenKind::Shr:    return ">>";
    case TokenKind::Comma:  return ",";
    }
    return "?";
}

Lexer::Lexer(std::string_view source) : source_(source), pos_(0) {}

void Lexer::skipWhitespace() {
    while (pos_ < source_.size()) {
        if (std::isspace(static_cast<unsigned char>(source_[pos_]))) {
            ++pos_;
            continue;
        }
        if (source_[pos_] == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            pos_ += 2;
            while (pos_ < source_.size() && source_[pos_] != '\n')
                ++pos_;
            continue;
        }
        if (source_[pos_] == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            pos_ += 2;
            while (pos_ + 1 < source_.size() && !(source_[pos_] == '*' && source_[pos_ + 1] == '/'))
                ++pos_;
            if (pos_ + 1 < source_.size())
                pos_ += 2;
            continue;
        }
        break;
    }
}

Token Lexer::readNumber() {
    size_t start = pos_;
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_])))
        ++pos_;
    Token tok;
    tok.kind = TokenKind::IntLit;
    tok.lexeme = std::string(source_.substr(start, pos_ - start));
    tok.intValue = std::stoll(tok.lexeme);
    return tok;
}

Token Lexer::readIdent() {
    size_t start = pos_;
    while (pos_ < source_.size() && (std::isalnum(static_cast<unsigned char>(source_[pos_])) || source_[pos_] == '_'))
        ++pos_;
    std::string lexeme(source_.substr(start, pos_ - start));
    Token tok;
    tok.lexeme = lexeme;

    if (lexeme == "fn")   tok.kind = TokenKind::Fn;
    else if (lexeme == "if")    tok.kind = TokenKind::If;
    else if (lexeme == "then")  tok.kind = TokenKind::Then;
    else if (lexeme == "else")  tok.kind = TokenKind::Else;
    else if (lexeme == "return") tok.kind = TokenKind::Return;
    else if (lexeme == "while") tok.kind = TokenKind::While;
    else if (lexeme == "for") tok.kind = TokenKind::For;
    else if (lexeme == "break") tok.kind = TokenKind::Break;
    else if (lexeme == "continue") tok.kind = TokenKind::Continue;
    else if (lexeme == "int") tok.kind = TokenKind::Int;
    else if (lexeme == "long") tok.kind = TokenKind::Long;
    else if (lexeme == "char") tok.kind = TokenKind::Char;
    else if (lexeme == "void") tok.kind = TokenKind::Void;
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

    if (std::isdigit(static_cast<unsigned char>(c))) return readNumber();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return readIdent();

    ++pos_;
    switch (c) {
    case '(': return {TokenKind::LParen, "(", 0};
    case ')': return {TokenKind::RParen, ")", 0};
    case '{': return {TokenKind::LBrace, "{", 0};
    case '}': return {TokenKind::RBrace, "}", 0};
    case ';': return {TokenKind::Semicolon, ";", 0};
    case '?': return {TokenKind::Question, "?", 0};
    case ':': return {TokenKind::Colon, ":", 0};
    case '=':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::EqEq, "==", 0};
        }
        return {TokenKind::Assign, "=", 0};
    case '+':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::PlusAssign, "+=", 0}; }
        if (pos_ < source_.size() && source_[pos_] == '+') { ++pos_; return {TokenKind::PlusPlus, "++", 0}; }
        return {TokenKind::Plus, "+", 0};
    case '-':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::MinusAssign, "-=", 0}; }
        if (pos_ < source_.size() && source_[pos_] == '-') { ++pos_; return {TokenKind::MinusMinus, "--", 0}; }
        return {TokenKind::Minus, "-", 0};
    case '*':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::StarAssign, "*=", 0}; }
        return {TokenKind::Star, "*", 0};
    case '/':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::SlashAssign, "/=", 0}; }
        return {TokenKind::Slash, "/", 0};
    case '%':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::PercentAssign, "%=", 0}; }
        return {TokenKind::Percent, "%", 0};
    case '!':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::Neq, "!=", 0};
        }
        return {TokenKind::Bang, "!", 0};
    case '<':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::Le, "<=", 0};
        }
        if (pos_ < source_.size() && source_[pos_] == '<') {
            ++pos_;
            return {TokenKind::Shl, "<<", 0};
        }
        return {TokenKind::Lt, "<", 0};
    case '>':
        if (pos_ < source_.size() && source_[pos_] == '=') {
            ++pos_;
            return {TokenKind::Ge, ">=", 0};
        }
        if (pos_ < source_.size() && source_[pos_] == '>') {
            ++pos_;
            return {TokenKind::Shr, ">>", 0};
        }
        return {TokenKind::Gt, ">", 0};
    case ',': return {TokenKind::Comma, ",", 0};
    case '&':
        if (pos_ < source_.size() && source_[pos_] == '&') { ++pos_; return {TokenKind::AmpAmp, "&&", 0}; }
        return {TokenKind::Amp, "&", 0};
    case '|':
        if (pos_ < source_.size() && source_[pos_] == '|') { ++pos_; return {TokenKind::PipePipe, "||", 0}; }
        return {TokenKind::Pipe, "|", 0};
    case '^': return {TokenKind::Caret, "^", 0};
    case '~': return {TokenKind::Tilde, "~", 0};
    }
    return {TokenKind::Invalid, std::string(1, c), 0};
}

} // namespace minic
