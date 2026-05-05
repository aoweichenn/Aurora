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

TEST(MiniCCompoundLiteralTest, SupportsStructCompoundLiteralsWithDesignators) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

long main() {
    return ((struct Pair){.second = 9, .first = 4}).first + ((struct Pair){3, 5}).second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 9));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 4));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 3));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 5));
}

TEST(MiniCCompoundLiteralTest, SupportsUnionCompoundLiteralAddresses) {
    auto module = generateMiniC(R"mini(
union Slot {
    long first;
    long second;
};

long main() {
    union Slot *slot = &(union Slot){.second = 11};
    return slot->second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 11));
}

TEST(MiniCCompoundLiteralTest, SupportsArrayCompoundLiteralDesignators) {
    auto module = generateMiniC(R"mini(
long main() {
    long *values = (long[4]){[2] = 7, [0] = 3, 5};
    return values[0] + values[1] + values[2] + (long[3]){[2] = 13}[2];
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 7));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 3));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 5));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 13));
}

TEST(MiniCCompoundLiteralTest, SupportsRecordArrayCompoundLiterals) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

long main() {
    struct Pair *pairs = (struct Pair[2]){[1].second = 9, [0] = {.first = 2, .second = 3}};
    return pairs[0].second + pairs[1].second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 2));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 3));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 9));
}

TEST(MiniCCompoundLiteralTest, SupportsScalarCompoundLiteralLValues) {
    auto module = generateMiniC(R"mini(
long main() {
    long *value = &(long){9};
    return *value + ((long){1} = 4) + (long){3};
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 9));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 4));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 3));
}

TEST(MiniCCompoundLiteralTest, RejectsInvalidCompoundLiterals) {
    EXPECT_THROW((void)generateMiniC("long main() { return (void){0}; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { return (long){.x = 1}; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { return (long[2]){.x = 1}[0]; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; long main() { return ((struct Pair){[0] = 1}).first; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("union Slot { long first; }; long main() { return ((union Slot){1, 2}).first; }"), std::runtime_error);
}
