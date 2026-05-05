#include <gtest/gtest.h>
#include "Aurora/Air/Constant.h"
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Module.h"
#include "minic/codegen/CodeGen.h"
#include "minic/lex/Lexer.h"
#include "minic/parse/Parser.h"
#include <array>
#include <cstddef>
#include <memory>
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

TEST(MiniCAlignofTest, ParsesAlignofTypeNamesInConstantExpressions) {
    Program program = parseMiniC(R"mini(
typedef long word;

static_assert(alignof(char) == 1, "char alignment");
static_assert(_Alignof(short) == 2, "short alignment");
static_assert(alignof(int) == 4, "int alignment");
static_assert(alignof(long *) == 8, "pointer alignment");
static_assert(alignof(word[2]) == 8, "array alignment");

enum {
    char_align = alignof(char),
    int_align = _Alignof(int),
    array_align = alignof(word[2])
};

long main() {
    return char_align + int_align + array_align;
}
)mini");

    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST(MiniCAlignofTest, CodeGenEvaluatesAlignofInGlobalInitializers) {
    auto module = generateMiniC(R"mini(
long alignments[6] = {
    alignof(char),
    _Alignof(short),
    alignof(int),
    alignof(long),
    alignof(long *),
    alignof(long[3])
};

long read() {
    return alignments[0];
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 6u);

    constexpr std::array<int64_t, 6> expected = {1, 2, 4, 8, 8, 8};
    for (std::size_t index = 0; index < expected.size(); ++index) {
        auto* element = dynamic_cast<ConstantInt*>(init->getElement(index));
        ASSERT_NE(element, nullptr);
        EXPECT_EQ(element->getSExtValue(), expected[index]);
    }
}

TEST(MiniCAlignofTest, CodeGenLowersAlignofExpressionsToConstants) {
    auto module = generateMiniC(R"mini(
long main() {
    return alignof(int *) + _Alignof(char);
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 8));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 1));
}
