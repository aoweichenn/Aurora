#include <gtest/gtest.h>
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Module.h"
#include "minic/codegen/CodeGen.h"
#include "minic/lex/Lexer.h"
#include "minic/parse/Parser.h"
#include <memory>
#include <stdexcept>
#include <string_view>

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

bool hasConstantInt(const aurora::Function& function, int64_t value) {
    for (const auto& block : function.getBlocks()) {
        for (const AIRInstruction* inst = block->getFirst(); inst; inst = inst->getNext()) {
            if (inst->getOpcode() == AIROpcode::ConstantInt && inst->getConstantValue() == value)
                return true;
        }
    }
    return false;
}

} // namespace

TEST(MiniCAlignasTest, ParsesObjectAndMemberAlignmentSpecifiers) {
    Program program = parseMiniC(R"mini(
alignas(16) long global_value;
_Alignas(long) long typed_global;

struct Packet {
    char tag;
    alignas(16) long payload;
};

static_assert(alignof(struct Packet) == 16, "member alignment raises record alignment");
static_assert(sizeof(struct Packet) == 32, "member alignment changes record layout");

long main() {
    alignas(32) long local = 1;
    _Alignas(long) long other = 2;
    return local + other + sizeof(struct Packet);
}
)mini");

    ASSERT_EQ(program.globals.size(), 2u);
    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST(MiniCAlignasTest, CodeGenAcceptsAlignedLocalDeclarations) {
    auto module = generateMiniC(R"mini(
struct Packet {
    char tag;
    alignas(16) long payload;
};

long main() {
    alignas(16) long value = 7;
    struct Packet packet = {.payload = value, .tag = 1};
    return packet.payload + sizeof(struct Packet) + alignof(struct Packet);
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 7));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 16));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 32));
}

TEST(MiniCAlignasTest, SupportsTypedefAndZeroAlignmentSpecifiers) {
    Program program = parseMiniC(R"mini(
typedef long word;

struct Natural {
    alignas(word) long value;
};

alignas(0) long no_extra_alignment;

static_assert(alignof(struct Natural) == 8, "type-name alignment is accepted");

long main() {
    return no_extra_alignment;
}
)mini");

    ASSERT_EQ(program.globals.size(), 1u);
    ASSERT_EQ(program.functions.size(), 1u);
}

TEST(MiniCAlignasTest, RejectsInvalidAlignmentSpecifiers) {
    EXPECT_THROW((void)parseMiniC("alignas(3) long value;"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("alignas(-1) long value;"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long value; alignas(value) long other;"), std::runtime_error);
}
