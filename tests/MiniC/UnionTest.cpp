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

TEST(MiniCUnionTest, AllocatesUnionStorageUsingLargestMember) {
    auto module = generateMiniC(R"mini(
union Slot {
    char tag;
    long value;
};

long main() {
    union Slot slot;
    slot.value = 42;
    return slot.value;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    const AIRInstruction* alloca = findFirstInstruction(*module->getFunctions()[0], AIROpcode::Alloca);
    ASSERT_NE(alloca, nullptr);
    ASSERT_TRUE(alloca->getType()->isPointer());
    ASSERT_TRUE(alloca->getType()->getElementType()->isArray());
    EXPECT_EQ(alloca->getType()->getElementType()->getNumElements(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 42));
}

TEST(MiniCUnionTest, SupportsPointerMemberAccessWithArrow) {
    auto module = generateMiniC(R"mini(
union Slot {
    long first;
    long second;
};

long main() {
    union Slot slot;
    union Slot *cursor = &slot;
    cursor->first = 7;
    return cursor->second;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 7));
}

TEST(MiniCUnionTest, SupportsSingleValueLocalInitializers) {
    auto module = generateMiniC(R"mini(
union Slot {
    long first;
    long second;
};

long main() {
    union Slot slot = {9};
    return slot.first;
}
)mini");

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasConstantInt(*module->getFunctions()[0], 9));
}

TEST(MiniCUnionTest, EvaluatesSizeofAndAlignofForUnions) {
    Program program = parseMiniC(R"mini(
union Slot {
    char tag;
    long value;
};

static_assert(sizeof(union Slot) == 8, "union size uses largest member");
static_assert(alignof(union Slot) == 8, "union alignment uses max member alignment");

long main() {
    return sizeof(union Slot) + alignof(union Slot);
}
)mini");

    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST(MiniCUnionTest, RejectsMultipleInitializerValues) {
    Program program = parseMiniC(R"mini(
union Slot {
    long first;
    long second;
};

long main() {
    union Slot slot = {1, 2};
    return slot.first;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCUnionTest, RejectsTagKindMismatch) {
    EXPECT_THROW((void)parseMiniC(R"mini(
struct Slot;
union Slot;

long main() {
    return 0;
}
)mini"), std::runtime_error);
}
