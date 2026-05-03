#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace minic {

enum class TokenKind {
    Eof, Ident, IntLit,
    Fn, If, Then, Else,
    LParen, RParen, Assign,
    Plus, Minus, Star, Slash,
    EqEq, Neq, Lt, Le, Gt, Ge,
    Comma,
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    int64_t intValue;
};

const char* tokenName(TokenKind kind);

class Lexer {
public:
    explicit Lexer(std::string_view source);
    Token next();

private:
    std::string_view source_;
    size_t pos_;
    void skipWhitespace();
    Token readNumber();
    Token readIdent();
};

} // namespace minic
