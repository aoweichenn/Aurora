#include <gtest/gtest.h>
#include "Aurora/Air/Function.h"
#include "Aurora/Air/Instruction.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
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

const AIRInstruction* findFirstInstruction(const aurora::Function& function, AIROpcode opcode) {
    for (const auto& block : function.getBlocks()) {
        for (const AIRInstruction* inst = block->getFirst(); inst; inst = inst->getNext()) {
            if (inst->getOpcode() == opcode)
                return inst;
        }
    }
    return nullptr;
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

TEST(MiniCStructTest, AllocatesStructStorageAndAccessesFields) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

long main() {
    struct Pair pair;
    pair.first = 3;
    pair.second = 4;
    return pair.first + pair.second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    const AIRInstruction* alloca = findFirstInstruction(*module->getFunctions()[0], AIROpcode::Alloca);
    ASSERT_NE(alloca, nullptr);
    ASSERT_TRUE(alloca->getType()->isPointer());
    ASSERT_TRUE(alloca->getType()->getElementType()->isArray());
    EXPECT_EQ(alloca->getType()->getElementType()->getNumElements(), 2u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 8));
}

TEST(MiniCStructTest, SupportsPointerMemberAccessWithArrow) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

long main() {
    struct Pair pair;
    struct Pair *cursor = &pair;
    cursor->first = 5;
    cursor->second = 6;
    return cursor->first + cursor->second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 8));
}

TEST(MiniCStructTest, SupportsOrderedLocalStructInitializers) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

long main() {
    struct Pair pair = {2, 5};
    return pair.first + pair.second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 2));
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 5));
}

TEST(MiniCStructTest, EvaluatesSizeofAndAlignofForStructs) {
    Program program = parseMiniC(R"mini(
struct Mixed {
    char tag;
    long value;
};

static_assert(sizeof(struct Mixed) == 16, "struct size includes padding");
static_assert(alignof(struct Mixed) == 8, "struct alignment follows max field alignment");

long main() {
    return sizeof(struct Mixed) + alignof(struct Mixed);
}
)mini");

    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST(MiniCStructTest, RejectsDuplicateStructFields) {
    EXPECT_THROW((void)parseMiniC(R"mini(
struct Bad {
    long value;
    long value;
};

long main() {
    return 0;
}
)mini"), std::runtime_error);
}

TEST(MiniCStructTest, RejectsUnknownStructFields) {
    Program program = parseMiniC(R"mini(
struct Pair {
    long first;
};

long main() {
    struct Pair pair;
    return pair.second;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}
