#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace minic {

enum class TokenKind {
    Eof, Ident, IntLit,
    Invalid,
    Fn, If, Then, Else, Return, While, For, Break, Continue,
    Int, Long, Char, Void,
    LParen, RParen, LBrace, RBrace,
    Semicolon, Comma, Question, Colon,
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign, PercentAssign,
    Plus, Minus, Star, Slash, Percent,
    PlusPlus, MinusMinus,
    EqEq, Neq, Lt, Le, Gt, Ge,
    Amp, Pipe, Caret, Tilde, Bang,
    AmpAmp, PipePipe,
    Shl, Shr,
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
