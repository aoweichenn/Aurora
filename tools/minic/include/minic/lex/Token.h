#pragma once

#include <cstdint>
#include <string>

namespace minic {

enum class TokenKind {
    Eof, Ident, IntLit, CharLit, StringLit,
    Invalid,
    Fn, If, Then, Else, Return, While, Do, For, Break, Continue,
    Switch, Case, Default, Sizeof, Alignof, Typedef, Enum, Struct, Union, StaticAssert,
    True, False, Nullptr,
    Int, Long, Short, Char, Bool, Void,
    Signed, Unsigned, Const, Volatile, Restrict,
    Static, Extern, Auto, Register, Inline,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Dot, Arrow,
    Semicolon, Comma, Question, Colon,
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign, PercentAssign,
    AmpAssign, PipeAssign, CaretAssign, ShlAssign, ShrAssign,
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

} // namespace minic
