#include <gtest/gtest.h>
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Module.h"
#include "minic/codegen/CodeGen.h"
#include "minic/lex/Lexer.h"
#include "minic/parse/Parser.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace aurora;
using namespace minic;

namespace {

Program parseMiniC(std::string_view source) {
    Lexer lexer(source);
    Parser parser(lexer);
    return parser.parseProgram();
}

std::unique_ptr<Module> generateMiniC(std::string_view source) {
    Program program = parseMiniC(source);
    CodeGen codegen;
    return codegen.generate(program);
}

bool hasOpcode(const aurora::Function& function, AIROpcode opcode) {
    for (const auto& block : function.getBlocks()) {
        for (const AIRInstruction* inst = block->getFirst(); inst; inst = inst->getNext()) {
            if (inst->getOpcode() == opcode)
                return true;
        }
    }
    return false;
}

std::vector<TokenKind> lexKinds(std::string_view source) {
    Lexer lexer(source);
    std::vector<TokenKind> kinds;
    while (true) {
        Token token = lexer.next();
        kinds.push_back(token.kind);
        if (token.kind == TokenKind::Eof || token.kind == TokenKind::Invalid)
            return kinds;
    }
}

} // namespace

TEST(MiniCFeatureCoverageTest, TokenNameCoversEveryTokenKind) {
    constexpr std::array<TokenKind, 72> kinds = {
        TokenKind::Eof, TokenKind::Ident, TokenKind::IntLit, TokenKind::CharLit,
        TokenKind::StringLit, TokenKind::Invalid, TokenKind::Fn, TokenKind::If,
        TokenKind::Then, TokenKind::Else, TokenKind::Return, TokenKind::While,
        TokenKind::Do, TokenKind::For, TokenKind::Break, TokenKind::Continue,
        TokenKind::Switch, TokenKind::Case, TokenKind::Default, TokenKind::Sizeof,
        TokenKind::Alignof, TokenKind::Alignas, TokenKind::Typedef, TokenKind::Enum,
        TokenKind::Struct, TokenKind::Union, TokenKind::StaticAssert, TokenKind::True,
        TokenKind::False, TokenKind::Nullptr, TokenKind::Int, TokenKind::Long,
        TokenKind::Short, TokenKind::Char, TokenKind::Bool, TokenKind::Void,
        TokenKind::Signed, TokenKind::Unsigned, TokenKind::Const, TokenKind::Volatile,
        TokenKind::Restrict, TokenKind::Static, TokenKind::Extern, TokenKind::Auto,
        TokenKind::Register, TokenKind::Inline, TokenKind::LParen, TokenKind::RParen,
        TokenKind::LBrace, TokenKind::RBrace, TokenKind::LBracket, TokenKind::RBracket,
        TokenKind::Dot, TokenKind::Arrow, TokenKind::Semicolon, TokenKind::Comma,
        TokenKind::Question, TokenKind::Colon, TokenKind::Assign, TokenKind::PlusAssign,
        TokenKind::MinusAssign, TokenKind::StarAssign, TokenKind::SlashAssign,
        TokenKind::PercentAssign, TokenKind::AmpAssign, TokenKind::PipeAssign,
        TokenKind::CaretAssign, TokenKind::ShlAssign, TokenKind::ShrAssign,
        TokenKind::Plus, TokenKind::Minus, TokenKind::Star
    };
    for (TokenKind kind : kinds)
        EXPECT_FALSE(std::string_view(tokenName(kind)).empty());

    constexpr std::array<TokenKind, 25> moreKinds = {
        TokenKind::Slash, TokenKind::Percent, TokenKind::PlusPlus, TokenKind::MinusMinus,
        TokenKind::EqEq, TokenKind::Neq, TokenKind::Lt, TokenKind::Le, TokenKind::Gt,
        TokenKind::Ge, TokenKind::Amp, TokenKind::Pipe, TokenKind::Caret, TokenKind::Tilde,
        TokenKind::Bang, TokenKind::AmpAmp, TokenKind::PipePipe, TokenKind::Shl,
        TokenKind::Shr, TokenKind::Comma, TokenKind::Eof, TokenKind::Ident,
        TokenKind::IntLit, TokenKind::CharLit, TokenKind::StringLit
    };
    for (TokenKind kind : moreKinds)
        EXPECT_FALSE(std::string_view(tokenName(kind)).empty());
}

TEST(MiniCFeatureCoverageTest, LexerRecognizesKeywordsOperatorsLiteralsAndInvalidInput) {
    auto kinds = lexKinds(R"mini(
// line comment
/* block comment */
fn if then else return while do for break continue switch case default sizeof alignof _Alignof alignas _Alignas
typedef enum struct union static_assert _Static_assert true false nullptr
int long short char bool _Bool void signed unsigned const volatile restrict static extern auto register inline
( ) { } [ ] . -> ; , ? : = += -= *= /= %= &= |= ^= <<= >>= + - * / % ++ -- == != < <= > >= & | ^ ~ ! && || << >>
123 0x10u 0b11 077L 'a' '\n' "text" @
)mini");

    EXPECT_EQ(kinds.back(), TokenKind::Invalid);
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), TokenKind::Struct), kinds.end());
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), TokenKind::Union), kinds.end());
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), TokenKind::Alignas), kinds.end());
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), TokenKind::Arrow), kinds.end());
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), TokenKind::StringLit), kinds.end());
}

TEST(MiniCFeatureCoverageTest, LexerReportsInvalidUnterminatedLiterals) {
    Lexer charLexer("'x");
    EXPECT_EQ(charLexer.next().kind, TokenKind::Invalid);

    Lexer stringLexer("\"unterminated");
    EXPECT_EQ(stringLexer.next().kind, TokenKind::Invalid);
}

TEST(MiniCFeatureCoverageTest, CodeGenCoversStatementsAndExpressions) {
    auto module = generateMiniC(R"mini(
extern long imported(long);
typedef unsigned const int flags_t, *flags_ptr;

struct Pair {
    long first;
    long second;
};

struct Node {
    struct Node *next;
    long value;
};

struct WithArray {
    long values[2];
};

struct Pair *get_pair(void);
long accepts(long values[static 3], int count, char tag);

long helper(long value) {
    return value + 1;
}

long main() {
    long values[3] = {1, 2};
    char letters[2] = {'a', 'b'};
    char *letter_cursor = letters;
    long total = 0;
    signed signed_value = -1;
    unsigned long unsigned_value = 9;
    bool ready = true;
    flags_t flags = 3;
    flags_ptr flags_pointer = &flags;
    struct WithArray holder;
    long *cursor = values;
    *cursor = *cursor + 1;
    cursor = cursor + 1;
    cursor = 1 + cursor;
    cursor = cursor - 1;
    cursor += 2;
    cursor -= 1;
    letter_cursor = letter_cursor + 1;
    values[2] = helper(values[0]);
    holder.values[0] = *flags_pointer + signed_value + ready;
    total = (total = 1) + 2;

    for (long i = 0; i < 3; i++) {
        if (i == 1) {
            continue;
        } else {
            total += values[i];
        }
    }

    long j = 0;
    while (j < 3) {
        if (j == 2) {
            break;
        }
        total += j;
        j++;
    }

    do {
        total--;
    } while (total > 100);

    switch (values[0]) {
    case 2:
        total += 3;
        break;
    default:
        total += 4;
    }

    total = total ? total : 1;
    total = (helper(0), total);
    total = (long)(+total) + sizeof(values) + alignof(long) + (nullptr == 0);
    total = total + (long)helper(0);
    total = total + (total = helper(0));
    total = total + values[helper(0)];
    total = total + (helper(0), 1);
    total = total + (total ? helper(0) : 1);
    total = total + sizeof(helper(0));
    total = total + get_pair()->first;
    total += ~0;
    total += !0;
    total += (1 && helper(0)) || 0;
    total = (total << 1) + 1;
    total <<= 1;
    total >>= 1;
    total &= 255;
    total |= 1;
    total ^= 3;
    total *= 2;
    total /= 2;
    total %= 7;
    unsigned_value /= 3;
    unsigned_value %= 2;
    unsigned_value >>= 1;
    total += accepts(values, 3, letters[0]);
    total += holder.values[0];
    return total;
}
)mini");

    ASSERT_GE(module->getFunctions().size(), 3u);
    const auto& function = *module->getFunctions().back();
    EXPECT_TRUE(hasOpcode(function, AIROpcode::Alloca));
    EXPECT_TRUE(hasOpcode(function, AIROpcode::CondBr));
    EXPECT_TRUE(hasOpcode(function, AIROpcode::Br));
    EXPECT_TRUE(hasOpcode(function, AIROpcode::Phi));
    EXPECT_TRUE(hasOpcode(function, AIROpcode::Call));
    EXPECT_TRUE(hasOpcode(function, AIROpcode::Load));
    EXPECT_TRUE(hasOpcode(function, AIROpcode::Store));
}

TEST(MiniCFeatureCoverageTest, CodeGenCoversIntegerConstantEvaluation) {
    auto module = generateMiniC(R"mini(
long constants[27] = {
    +1,
    -2,
    !0,
    ~0,
    (long)3,
    4 + 5,
    6 - 1,
    2 * 3,
    8 / 2,
    8 % 3,
    4 / 0,
    4 % 0,
    1 == 1,
    1 != 2,
    1 < 2,
    1 <= 1,
    2 > 1,
    2 >= 2,
    6 & 3,
    4 | 1,
    5 ^ 3,
    1 << 3,
    8 >> 1,
    1 && 1,
    0 || 1,
    sizeof(long),
    alignof(long)
};

long main() {
    return constants[0];
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
}

TEST(MiniCFeatureCoverageTest, ParserCoversConstantEvaluationAndTypeForms) {
    Program program = parseMiniC(R"mini(
static_assert(+1 == 1, "plus");
static_assert(-1 + 2 == 1, "neg");
static_assert(!0 == 1, "not");
static_assert(~0 == -1, "bitnot");
static_assert((long)3 == 3, "cast");
static_assert((1 ? 2 : 3) == 2, "conditional");
static_assert((0, 4) == 4, "comma");
static_assert(6 / 3 == 2, "div");
static_assert(5 % 2 == 1, "rem");
static_assert(1 != 2, "neq");
static_assert(1 < 2, "lt");
static_assert(1 <= 1, "le");
static_assert(2 > 1, "gt");
static_assert(2 >= 2, "ge");
static_assert((6 & 3) == 2, "and");
static_assert((4 | 1) == 5, "or");
static_assert((5 ^ 3) == 6, "xor");
static_assert((1 << 3) == 8, "shl");
static_assert((8 >> 1) == 4, "shr");
static_assert(1 && 1, "land");
static_assert(0 || 1, "lor");

enum Tagged {
    TAG_ZERO,
    TAG_ONE = 1,
};

struct AnonymousHolder {
    struct { long inner; } nested;
};

void takes_void(void);
long takes_many(long values[static 2], int count, char tag);

long main() {
    return TAG_ONE;
}
)mini");

    ASSERT_EQ(program.functions.size(), 3u);
}

TEST(MiniCFeatureCoverageTest, ParsesLegacyFunctionsAndExpressionIf) {
    Program program = parseMiniC(R"mini(
fn choose(a, b) = if a then b else 3;
)mini");

    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "choose");
}

TEST(MiniCFeatureCoverageTest, CodeGenRejectsCommonInvalidPrograms) {
    EXPECT_THROW((void)generateMiniC("long main() { long value; long value; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { void value; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { break; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { continue; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long f(long a); long main() { return f(1, 2); }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { return missing(); }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { long a; long b; return (&a) + (&b); }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { long a; return 1 - (&a); }"), std::runtime_error);
}

TEST(MiniCFeatureCoverageTest, ParserRejectsInvalidDeclarationsAndStatements) {
    EXPECT_THROW((void)parseMiniC("long main() { case 1: return 0; }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long main() { switch (1) { default: return 0; default: return 1; } }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long main() { switch (1) { return 0; } }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("static_assert(0, \"no\"); long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long main() { ++1; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long main() { &(1); return 0; }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long main() { long values[0]; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { struct Missing value; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("struct Repeat { long value; }; struct Repeat { long value; };"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("struct Bad { void value; };"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("struct Bad { struct Missing value; };"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("struct;"), std::runtime_error);
}
