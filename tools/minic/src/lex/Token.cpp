#include "minic/lex/Token.h"

namespace minic {

const char* tokenName(TokenKind kind) {
    switch (kind) {
    case TokenKind::Eof:    return "EOF";
    case TokenKind::Ident:  return "Identifier";
    case TokenKind::IntLit: return "Integer";
    case TokenKind::CharLit:return "Character";
    case TokenKind::StringLit:return "String";
    case TokenKind::Invalid:return "Invalid";
    case TokenKind::Fn:     return "fn";
    case TokenKind::If:     return "if";
    case TokenKind::Then:   return "then";
    case TokenKind::Else:   return "else";
    case TokenKind::Return: return "return";
    case TokenKind::While:  return "while";
    case TokenKind::Do:     return "do";
    case TokenKind::For:    return "for";
    case TokenKind::Break:  return "break";
    case TokenKind::Continue:return "continue";
    case TokenKind::Switch: return "switch";
    case TokenKind::Case:   return "case";
    case TokenKind::Default:return "default";
    case TokenKind::Sizeof: return "sizeof";
    case TokenKind::Alignof:return "alignof";
    case TokenKind::Typedef:return "typedef";
    case TokenKind::Enum:   return "enum";
    case TokenKind::StaticAssert:return "static_assert";
    case TokenKind::True:   return "true";
    case TokenKind::False:  return "false";
    case TokenKind::Nullptr:return "nullptr";
    case TokenKind::Int:    return "int";
    case TokenKind::Long:   return "long";
    case TokenKind::Short:  return "short";
    case TokenKind::Char:   return "char";
    case TokenKind::Bool:   return "bool";
    case TokenKind::Void:   return "void";
    case TokenKind::Signed: return "signed";
    case TokenKind::Unsigned:return "unsigned";
    case TokenKind::Const:  return "const";
    case TokenKind::Volatile:return "volatile";
    case TokenKind::Restrict:return "restrict";
    case TokenKind::Static: return "static";
    case TokenKind::Extern: return "extern";
    case TokenKind::Auto:   return "auto";
    case TokenKind::Register:return "register";
    case TokenKind::Inline: return "inline";
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::LBrace: return "{";
    case TokenKind::RBrace: return "}";
    case TokenKind::LBracket:return "[";
    case TokenKind::RBracket:return "]";
    case TokenKind::Semicolon:return ";";
    case TokenKind::Question:return "?";
    case TokenKind::Colon:  return ":";
    case TokenKind::Assign: return "=";
    case TokenKind::PlusAssign:return "+=";
    case TokenKind::MinusAssign:return "-=";
    case TokenKind::StarAssign:return "*=";
    case TokenKind::SlashAssign:return "/=";
    case TokenKind::PercentAssign:return "%=";
    case TokenKind::AmpAssign:return "&=";
    case TokenKind::PipeAssign:return "|=";
    case TokenKind::CaretAssign:return "^=";
    case TokenKind::ShlAssign:return "<<=";
    case TokenKind::ShrAssign:return ">>=";
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

} // namespace minic
