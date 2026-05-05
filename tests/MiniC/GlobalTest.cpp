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

bool hasGlobalAddress(const aurora::Function& function, std::string_view name) {
    for (const auto& block : function.getBlocks()) {
        for (const AIRInstruction* inst = block->getFirst(); inst; inst = inst->getNext()) {
            if (inst->getOpcode() == AIROpcode::GlobalAddress && inst->getGlobalName() &&
                std::string_view(inst->getGlobalName()) == name)
                return true;
        }
    }
    return false;
}

} // namespace

TEST(MiniCGlobalTest, ParsesTopLevelScalarAndExternGlobals) {
    Program program = parseMiniC(R"mini(
extern long imported;
long counter = 7;

long main() {
    return counter;
}
)mini");

    ASSERT_EQ(program.globals.size(), 2u);
    EXPECT_EQ(program.globals[0].name, "imported");
    EXPECT_TRUE(program.globals[0].isExtern);
    EXPECT_EQ(program.globals[0].init, nullptr);

    EXPECT_EQ(program.globals[1].name, "counter");
    EXPECT_FALSE(program.globals[1].isExtern);
    ASSERT_NE(program.globals[1].init, nullptr);
    auto* init = dynamic_cast<IntLitExpr*>(program.globals[1].init.get());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->value, 7);

    ASSERT_EQ(program.functions.size(), 1u);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST(MiniCGlobalTest, CodeGenEmitsDefinedGlobalsAndReferencesExterns) {
    auto module = generateMiniC(R"mini(
extern long imported;
long counter = 7;

long read() {
    return counter + imported;
}

long write() {
    counter = 4;
    return counter;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    const auto& global = module->getGlobals()[0];
    EXPECT_EQ(global->getName(), "counter");
    auto* init = dynamic_cast<ConstantInt*>(global->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 7);

    ASSERT_EQ(module->getFunctions().size(), 2u);
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[0], "counter"));
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[0], "imported"));
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[1], "counter"));
}

TEST(MiniCGlobalTest, CodeGenEmitsGlobalArrayInitializersAndReferencesElements) {
    auto module = generateMiniC(R"mini(
long values[3] = {1, 2};

long read() {
    return values[0] + values[1] + values[2];
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    const auto& global = module->getGlobals()[0];
    EXPECT_EQ(global->getName(), "values");
    ASSERT_TRUE(global->getType()->isArray());
    EXPECT_EQ(global->getType()->getNumElements(), 3u);

    auto* init = dynamic_cast<ConstantArray*>(global->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 3u);
    auto* first = dynamic_cast<ConstantInt*>(init->getElement(0));
    auto* second = dynamic_cast<ConstantInt*>(init->getElement(1));
    auto* third = dynamic_cast<ConstantInt*>(init->getElement(2));
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(third, nullptr);
    EXPECT_EQ(first->getSExtValue(), 1);
    EXPECT_EQ(second->getSExtValue(), 2);
    EXPECT_EQ(third->getSExtValue(), 0);

    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[0], "values"));
}

TEST(MiniCGlobalTest, ExternGlobalArrayCanBeReferenced) {
    auto module = generateMiniC(R"mini(
extern long values[2];

long read() {
    return values[1];
}
)mini");

    EXPECT_TRUE(module->getGlobals().empty());
    ASSERT_EQ(module->getFunctions().size(), 1u);
    EXPECT_TRUE(hasGlobalAddress(*module->getFunctions()[0], "values"));
}

TEST(MiniCGlobalTest, ExternGlobalCanBeDefinedLater) {
    auto module = generateMiniC(R"mini(
extern long value;
long value = 42;

long main() {
    return value;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    EXPECT_EQ(module->getGlobals()[0]->getName(), "value");
    auto* init = dynamic_cast<ConstantInt*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 42);
}

TEST(MiniCGlobalTest, ScalarGlobalAcceptsSingleValueBracedInitializer) {
    auto module = generateMiniC(R"mini(
long value = {42};

long main() {
    return value;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantInt*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 42);
}

TEST(MiniCGlobalTest, CodeGenEmitsGlobalStructInitializers) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

struct Pair pair = {.second = 9, .first = 4};

long main() {
    return pair.first + pair.second;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    const auto& global = module->getGlobals()[0];
    EXPECT_EQ(global->getName(), "pair");
    ASSERT_TRUE(global->getType()->isArray());
    EXPECT_EQ(global->getType()->getNumElements(), 2u);

    auto* init = dynamic_cast<ConstantArray*>(global->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 2u);
    auto* first = dynamic_cast<ConstantInt*>(init->getElement(0));
    auto* second = dynamic_cast<ConstantInt*>(init->getElement(1));
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first->getSExtValue(), 4);
    EXPECT_EQ(second->getSExtValue(), 9);
}

TEST(MiniCGlobalTest, CodeGenEmitsGlobalUnionInitializers) {
    auto module = generateMiniC(R"mini(
union Slot {
    long first;
    long second;
};

union Slot slot = {.second = 11};

long main() {
    return slot.second;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 1u);
    auto* value = dynamic_cast<ConstantInt*>(init->getElement(0));
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->getSExtValue(), 11);
}

TEST(MiniCGlobalTest, CodeGenEmitsAlignedGlobalRecordInitializers) {
    auto module = generateMiniC(R"mini(
struct Packet {
    char tag;
    alignas(16) long payload;
};

struct Packet packet = {.payload = 7, .tag = 1};

long main() {
    return packet.tag + packet.payload;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 4u);
    const int64_t expected[] = {1, 0, 7, 0};
    for (unsigned index = 0; index < 4; ++index) {
        auto* element = dynamic_cast<ConstantInt*>(init->getElement(index));
        ASSERT_NE(element, nullptr);
        EXPECT_EQ(element->getSExtValue(), expected[index]);
    }
}

TEST(MiniCGlobalTest, CodeGenEmitsGlobalStructArrayInitializers) {
    auto module = generateMiniC(R"mini(
struct Pair {
    long first;
    long second;
};

struct Pair pairs[3] = {{1, 2}, {.second = 5, .first = 4}, [2] = {.second = 9}};

long main() {
    return pairs[0].second + pairs[1].first + pairs[2].second;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 3u);

    const int64_t expected[3][2] = {{1, 2}, {4, 5}, {0, 9}};
    for (unsigned row = 0; row < 3; ++row) {
        auto* element = dynamic_cast<ConstantArray*>(init->getElement(row));
        ASSERT_NE(element, nullptr);
        ASSERT_EQ(element->getNumElements(), 2u);
        for (unsigned column = 0; column < 2; ++column) {
            auto* value = dynamic_cast<ConstantInt*>(element->getElement(column));
            ASSERT_NE(value, nullptr);
            EXPECT_EQ(value->getSExtValue(), expected[row][column]);
        }
    }
}

TEST(MiniCGlobalTest, CodeGenEmitsGlobalUnionArrayInitializers) {
    auto module = generateMiniC(R"mini(
union Slot {
    long first;
    long second;
};

union Slot slots[2] = {{.second = 7}, [1] = {3}};

long main() {
    return slots[0].second + slots[1].first;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 2u);
    const int64_t expected[] = {7, 3};
    for (unsigned index = 0; index < 2; ++index) {
        auto* element = dynamic_cast<ConstantArray*>(init->getElement(index));
        ASSERT_NE(element, nullptr);
        ASSERT_EQ(element->getNumElements(), 1u);
        auto* value = dynamic_cast<ConstantInt*>(element->getElement(0));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(value->getSExtValue(), expected[index]);
    }
}

TEST(MiniCGlobalTest, CodeGenEmitsNestedGlobalRecordInitializers) {
    auto module = generateMiniC(R"mini(
struct Inner {
    long x;
    long y;
};

union Slot {
    long first;
    long second;
};

struct Outer {
    long values[2];
    struct Inner inner;
    union Slot slot;
};

struct Outer outer = {{1, 2}, {.y = 5, .x = 4}, {.second = 7}};

long main() {
    return outer.values[1] + outer.inner.y + outer.slot.second;
}
)mini");

    ASSERT_EQ(module->getGlobals().size(), 1u);
    auto* init = dynamic_cast<ConstantArray*>(module->getGlobals()[0]->getInitializer());
    ASSERT_NE(init, nullptr);
    ASSERT_EQ(init->getNumElements(), 5u);
    const int64_t expected[] = {1, 2, 4, 5, 7};
    for (unsigned index = 0; index < 5; ++index) {
        auto* value = dynamic_cast<ConstantInt*>(init->getElement(index));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(value->getSExtValue(), expected[index]);
    }
}

TEST(MiniCGlobalTest, RejectsConflictingGlobalDeclarations) {
    Program program = parseMiniC(R"mini(
extern long value;
int value;

long main() {
    return 0;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsDuplicateGlobalDefinitions) {
    Program program = parseMiniC(R"mini(
long value;
long value;

long main() {
    return 0;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsNonConstantGlobalInitializers) {
    Program program = parseMiniC(R"mini(
long next() {
    return 1;
}

long value = next();

long main() {
    return value;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsGlobalArrayInitializerWithTooManyValues) {
    Program program = parseMiniC(R"mini(
long values[2] = {1, 2, 3};

long main() {
    return 0;
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsNonConstantGlobalArrayInitializer) {
    Program program = parseMiniC(R"mini(
long next() {
    return 1;
}

long values[2] = {1, next()};

long main() {
    return values[0];
}
)mini");

    CodeGen codegen;
    EXPECT_THROW((void)codegen.generate(program), std::runtime_error);
}

TEST(MiniCGlobalTest, RejectsInvalidGlobalRecordInitializers) {
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; struct Pair pair = {[0] = 1}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; struct Pair pair = {.missing = 1}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; struct Pair pair = {1, 2}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("union Slot { long first; }; union Slot slot = {1, 2}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("long next() { return 1; } struct Pair { long first; }; struct Pair pair = {next()}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct WithArray { long values[2]; }; struct WithArray bad = {1}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; struct Pair pairs[1] = {.first = 1}; long main() { return 0; }"), std::runtime_error);
    EXPECT_THROW((void)generateMiniC("struct Pair { long first; }; struct Pair pairs[1] = {1}; long main() { return 0; }"), std::runtime_error);
}
