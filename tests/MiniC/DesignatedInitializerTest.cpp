#include <gtest/gtest.h>
#include "Aurora/Air/Constant.h"
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

TEST(MiniCDesignatedInitializerTest, SupportsGlobalArrayIndexDesignators) {
    auto module = generateMiniC(R"mini(
long values[5] = {[2] = 7, [0] = 3, 9};

long main() {
    return values[0] + values[1] + values[2] + values[3] + values[4];
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 5u);

    const int64_t expected[] = {3, 9, 7, 0, 0};
    for (unsigned index = 0; index < 5; ++index) {
        auto* element = dynamic_cast<ConstantInt*>(init->getElement(index));
        ASSERT_NE(element, nullptr);
        EXPECT_EQ(element->getSExtValue(), expected[index]);
    }
}

TEST(MiniCDesignatedInitializerTest, SupportsLocalArrayIndexDesignators) {
    auto module = generateMiniC(R"mini(
long main() {
    long values[4] = {[1] = 4, 5, [0] = 2};
    return values[0] + values[1] + values[2] + values[3];
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 4));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 5));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 2));
}

TEST(MiniCDesignatedInitializerTest, SupportsStructFieldDesignators) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

long main() {
    struct Pair pair = {.second = 9, .first = 4};
    return pair.first + pair.second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 9));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 4));
}

TEST(MiniCDesignatedInitializerTest, SupportsUnionFieldDesignators) {
    auto module = generateMiniC(R"mini(
union Slot {
    long first;
    long second;
};

long main() {
    union Slot slot = {.second = 11};
    return slot.second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 11));
}

TEST(MiniCDesignatedInitializerTest, RejectsInvalidDesignators) {
    EXPECT_THROW((void)generateMiniC("long main() { long value = {.x = 1}; return value; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long main() { long values[2] = {.x = 1}; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; long main() { struct Pair pair = {[0] = 1}; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("union Slot { long first; }; long main() { union Slot slot = {[0] = 1}; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; long main() { struct Pair pair = {.missing = 1}; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("union Slot { long first; }; long main() { union Slot slot = {.missing = 1}; return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long values[1] = {[2] = 1}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)parseMiniC("long main() { long values[1] = {[-1] = 1}; return 0; }"), std::runtime_error);
}
