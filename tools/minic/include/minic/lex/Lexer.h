#pragma once

#include "minic/lex/Token.h"

#include <cstddef>
#include <string_view>

namespace minic {

class Lexer {
public:
    explicit Lexer(std::string_view source);
    Token next();

private:
    std::string_view source_;
    std::size_t pos_;
    void skipWhitespace();
    Token readNumber();
    Token readCharLiteral();
    Token readStringLiteral();
    Token readIdent();
    char readEscapedChar();
};

} // namespace minic
