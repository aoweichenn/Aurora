#include "minic/lex/Lexer.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace minic {

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
    int base = 10;
    if (source_[pos_] == '0' && pos_ + 1 < source_.size()) {
        const char prefix = source_[pos_ + 1];
        if (prefix == 'x' || prefix == 'X') {
            base = 16;
            pos_ += 2;
            while (pos_ < source_.size() && std::isxdigit(static_cast<unsigned char>(source_[pos_])))
                ++pos_;
        } else if (prefix == 'b' || prefix == 'B') {
            base = 2;
            pos_ += 2;
            while (pos_ < source_.size() && (source_[pos_] == '0' || source_[pos_] == '1'))
                ++pos_;
        } else {
            base = 8;
            ++pos_;
            while (pos_ < source_.size() && source_[pos_] >= '0' && source_[pos_] <= '7')
                ++pos_;
        }
    } else {
        while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_])))
            ++pos_;
    }
    while (pos_ < source_.size()) {
        const char suffix = source_[pos_];
        if (suffix == 'u' || suffix == 'U' || suffix == 'l' || suffix == 'L')
            ++pos_;
        else
            break;
    }
    Token tok;
    tok.kind = TokenKind::IntLit;
    tok.lexeme = std::string(source_.substr(start, pos_ - start));
    std::string digits = tok.lexeme;
    while (!digits.empty()) {
        const char suffix = digits.back();
        if (suffix == 'u' || suffix == 'U' || suffix == 'l' || suffix == 'L')
            digits.pop_back();
        else
            break;
    }
    if (base == 2 && digits.size() > 2)
        digits = digits.substr(2);
    tok.intValue = std::stoll(digits, nullptr, base);
    return tok;
}

char Lexer::readEscapedChar() {
    if (pos_ >= source_.size())
        return '\0';
    char c = source_[pos_++];
    if (c != '\\')
        return c;
    if (pos_ >= source_.size())
        return '\0';
    c = source_[pos_++];
    switch (c) {
    case '0': return '\0';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case '\\': return '\\';
    case '\'': return '\'';
    case '"': return '"';
    default: return c;
    }
}

Token Lexer::readCharLiteral() {
    size_t start = pos_;
    ++pos_;
    if (pos_ >= source_.size())
        return {TokenKind::Invalid, "'", 0};
    char value = readEscapedChar();
    if (pos_ >= source_.size() || source_[pos_] != '\'')
        return {TokenKind::Invalid, std::string(source_.substr(start, pos_ - start)), 0};
    ++pos_;
    return {TokenKind::CharLit, std::string(source_.substr(start, pos_ - start)), value};
}

Token Lexer::readStringLiteral() {
    size_t start = pos_;
    ++pos_;
    while (pos_ < source_.size() && source_[pos_] != '"') {
        if (source_[pos_] == '\\' && pos_ + 1 < source_.size()) {
            pos_ += 2;
        } else {
            ++pos_;
        }
    }
    if (pos_ >= source_.size())
        return {TokenKind::Invalid, std::string(source_.substr(start, pos_ - start)), 0};
    ++pos_;
    return {TokenKind::StringLit, std::string(source_.substr(start, pos_ - start)), 0};
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
    else if (lexeme == "do") tok.kind = TokenKind::Do;
    else if (lexeme == "for") tok.kind = TokenKind::For;
    else if (lexeme == "break") tok.kind = TokenKind::Break;
    else if (lexeme == "continue") tok.kind = TokenKind::Continue;
    else if (lexeme == "switch") tok.kind = TokenKind::Switch;
    else if (lexeme == "case") tok.kind = TokenKind::Case;
    else if (lexeme == "default") tok.kind = TokenKind::Default;
    else if (lexeme == "sizeof") tok.kind = TokenKind::Sizeof;
    else if (lexeme == "alignof" || lexeme == "_Alignof") tok.kind = TokenKind::Alignof;
    else if (lexeme == "typedef") tok.kind = TokenKind::Typedef;
    else if (lexeme == "enum") tok.kind = TokenKind::Enum;
    else if (lexeme == "struct") tok.kind = TokenKind::Struct;
    else if (lexeme == "union") tok.kind = TokenKind::Union;
    else if (lexeme == "static_assert" || lexeme == "_Static_assert") tok.kind = TokenKind::StaticAssert;
    else if (lexeme == "true") tok.kind = TokenKind::True;
    else if (lexeme == "false") tok.kind = TokenKind::False;
    else if (lexeme == "nullptr") tok.kind = TokenKind::Nullptr;
    else if (lexeme == "int") tok.kind = TokenKind::Int;
    else if (lexeme == "long") tok.kind = TokenKind::Long;
    else if (lexeme == "short") tok.kind = TokenKind::Short;
    else if (lexeme == "char") tok.kind = TokenKind::Char;
    else if (lexeme == "bool" || lexeme == "_Bool") tok.kind = TokenKind::Bool;
    else if (lexeme == "void") tok.kind = TokenKind::Void;
    else if (lexeme == "signed") tok.kind = TokenKind::Signed;
    else if (lexeme == "unsigned") tok.kind = TokenKind::Unsigned;
    else if (lexeme == "const") tok.kind = TokenKind::Const;
    else if (lexeme == "volatile") tok.kind = TokenKind::Volatile;
    else if (lexeme == "restrict") tok.kind = TokenKind::Restrict;
    else if (lexeme == "static") tok.kind = TokenKind::Static;
    else if (lexeme == "extern") tok.kind = TokenKind::Extern;
    else if (lexeme == "auto") tok.kind = TokenKind::Auto;
    else if (lexeme == "register") tok.kind = TokenKind::Register;
    else if (lexeme == "inline") tok.kind = TokenKind::Inline;
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
    if (c == '\'') return readCharLiteral();
    if (c == '"') return readStringLiteral();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return readIdent();

    ++pos_;
    switch (c) {
    case '(': return {TokenKind::LParen, "(", 0};
    case ')': return {TokenKind::RParen, ")", 0};
    case '{': return {TokenKind::LBrace, "{", 0};
    case '}': return {TokenKind::RBrace, "}", 0};
    case '[': return {TokenKind::LBracket, "[", 0};
    case ']': return {TokenKind::RBracket, "]", 0};
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
        if (pos_ < source_.size() && source_[pos_] == '>') { ++pos_; return {TokenKind::Arrow, "->", 0}; }
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
            if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::ShlAssign, "<<=", 0}; }
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
            if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::ShrAssign, ">>=", 0}; }
            return {TokenKind::Shr, ">>", 0};
        }
        return {TokenKind::Gt, ">", 0};
    case ',': return {TokenKind::Comma, ",", 0};
    case '&':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::AmpAssign, "&=", 0}; }
        if (pos_ < source_.size() && source_[pos_] == '&') { ++pos_; return {TokenKind::AmpAmp, "&&", 0}; }
        return {TokenKind::Amp, "&", 0};
    case '|':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::PipeAssign, "|=", 0}; }
        if (pos_ < source_.size() && source_[pos_] == '|') { ++pos_; return {TokenKind::PipePipe, "||", 0}; }
        return {TokenKind::Pipe, "|", 0};
    case '^':
        if (pos_ < source_.size() && source_[pos_] == '=') { ++pos_; return {TokenKind::CaretAssign, "^=", 0}; }
        return {TokenKind::Caret, "^", 0};
    case '~': return {TokenKind::Tilde, "~", 0};
    case '.': return {TokenKind::Dot, ".", 0};
    }
    return {TokenKind::Invalid, std::string(1, c), 0};
}

} // namespace minic
